/* MapOutfitterPanel.cpp
Copyright (c) 2015 by Michael Zahniser

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#include "MapOutfitterPanel.h"

#include "CoreStartData.h"
#include "text/Format.h"
#include "GameData.h"
#include "text/Gettext.h"
#include "Outfit.h"
#include "Planet.h"
#include "PlayerInfo.h"
#include "Point.h"
#include "Screen.h"
#include "Sprite.h"
#include "StellarObject.h"
#include "System.h"
#include "UI.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <map>
#include <set>

using namespace std;
using namespace Gettext;

namespace {
	const T_ MINE = T_("Mine this here");
	const T_ LABEL[] = {T_("Has no outfitter"), T_("Has outfitter"), T_("Sells this outfit")};
}



MapOutfitterPanel::MapOutfitterPanel(PlayerInfo &player)
	: MapSalesPanel(player, true)
{
	Init();
}



MapOutfitterPanel::MapOutfitterPanel(const MapPanel &panel, bool onlyHere)
	: MapSalesPanel(panel, true)
{
	Init();
	onlyShowSoldHere = onlyHere;
	UpdateCache();
}



const Sprite *MapOutfitterPanel::SelectedSprite() const
{
	return selected ? selected->Thumbnail() : nullptr;
}



const Sprite *MapOutfitterPanel::CompareSprite() const
{
	return compare ? compare->Thumbnail() : nullptr;
}



const ItemInfoDisplay &MapOutfitterPanel::SelectedInfo() const
{
	return selectedInfo;
}



const ItemInfoDisplay &MapOutfitterPanel::CompareInfo() const
{
	return compareInfo;
}



const string &MapOutfitterPanel::KeyLabel(int index) const
{
	if(index == 2 && selected && selected->Get("installable") < 0)
		return MINE.Str();
	
	return LABEL[index].Str();
}



void MapOutfitterPanel::Select(int index)
{
	if(index < 0 || index >= static_cast<int>(list.size()))
		selected = nullptr;
	else
	{
		selected = list[index];
		selectedInfo.Update(*selected, player);
	}
	UpdateCache();
}



void MapOutfitterPanel::Compare(int index)
{
	if(index < 0 || index >= static_cast<int>(list.size()))
		compare = nullptr;
	else
	{
		compare = list[index];
		compareInfo.Update(*compare, player);
	}
}



double MapOutfitterPanel::SystemValue(const System *system) const
{
	if(!system || !player.HasVisited(*system))
		return numeric_limits<double>::quiet_NaN();
	
	auto it = player.Harvested().lower_bound(pair<const System *, const Outfit *>(system, nullptr));
	for( ; it != player.Harvested().end() && it->first == system; ++it)
		if(it->second == selected)
			return 1.;
	
	if(!system->IsInhabited(player.Flagship()))
		return numeric_limits<double>::quiet_NaN();
	
	// Visiting a system is sufficient to know what ports are available on its planets.
	double value = -.5;
	for(const StellarObject &object : system->Objects())
		if(object.GetPlanet())
		{
			const auto &outfitter = object.GetPlanet()->Outfitter();
			if(outfitter.Has(selected))
				return 1.;
			if(!outfitter.empty())
				value = 0.;
		}
	return value;
}



int MapOutfitterPanel::FindItem(const string &text) const
{
	int bestIndex = 9999;
	int bestItem = -1;
	for(unsigned i = 0; i < list.size(); ++i)
	{
		int index = Search(list[i]->Name(), text);
		if(index >= 0 && index < bestIndex)
		{
			bestIndex = index;
			bestItem = i;
			if(!index)
				return i;
		}
	}
	return bestItem;
}



void MapOutfitterPanel::DrawItems()
{
	if(GetUI()->IsTop(this) && player.GetPlanet() && player.GetDate() >= player.StartData().GetDate() + 12)
		DoHelp("map advanced shops");
	list.clear();
	Point corner = Screen::TopLeft() + Point(0, scroll);
	for(const string &category : categories)
	{
		auto it = catalog.find(category);
		if(it == catalog.end())
			continue;
		
		// Draw the header. If this category is collapsed, skip drawing the items.
		if(DrawHeader(corner, category))
			continue;
		
		// Translators may control order of outfits.
		// The translated keys may not be unique.
		multimap<string, const Outfit*> sortedOutfits;
		for(const Outfit *outfit : it->second)
			sortedOutfits.emplace(T(outfit->TrueName(), "sort key"), outfit);
		
		for(const auto &it : sortedOutfits)
		{
			const Outfit *outfit = it.second;
			string price = Format::Credits(outfit->Cost()) + T(" credits", "MapOUtfitterPanel");
			
			string info;
			if(outfit->Get("installable") < 0.)
				info = T("(Mined from asteroids)");
			else
			{
				double space = -outfit->Get("outfit space");
				string kind_of_outfit(T("outfit", "kind of outfit"));
				if(space && -outfit->Get("weapon capacity") == space)
					kind_of_outfit = T("weapon", "kind of outfit");
				else if(space && -outfit->Get("engine capacity") == space)
					kind_of_outfit = T("engine", "kind of outfit");
				info = Format::StringF(nT("%1% ton of %2% space", "%1% tons of %2% space", abs(space)),
					Format::Number(space), kind_of_outfit);
			}
			
			bool isForSale = true;
			if(player.HasVisited(*selectedSystem))
			{
				isForSale = false;
				for(const StellarObject &object : selectedSystem->Objects())
					if(object.GetPlanet() && object.GetPlanet()->Outfitter().Has(outfit))
					{
						isForSale = true;
						break;
					}
			}
			if(!isForSale && onlyShowSoldHere)
				continue;
			
			Draw(corner, outfit->Thumbnail(), isForSale, outfit == selected,
				outfit->Name(), price, info);
			list.push_back(outfit);
		}
	}
	maxScroll = corner.Y() - scroll - .5 * Screen::Height();
}



void MapOutfitterPanel::Init()
{
	catalog.clear();
	set<const Outfit *> seen;
	for(auto &&it : GameData::Planets())
		if(it.second.IsValid() && player.HasVisited(*it.second.GetSystem()))
			for(const Outfit *outfit : it.second.Outfitter())
				if(!seen.count(outfit))
				{
					catalog[outfit->Category()].push_back(outfit);
					seen.insert(outfit);
				}
	for(const auto &it : player.Harvested())
		if(!seen.count(it.second))
		{
			catalog[it.second->Category()].push_back(it.second);
			seen.insert(it.second);
		}
	
	for(auto &it : catalog)
		sort(it.second.begin(), it.second.end(),
			[](const Outfit *a, const Outfit *b) { return a->TrueName() < b->TrueName(); });
}
