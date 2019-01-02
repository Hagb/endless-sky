/* OutfitterPanel.cpp
Copyright (c) 2014 by Michael Zahniser

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#include "OutfitterPanel.h"

#include "Color.h"
#include "Dialog.h"
#include "DistanceMap.h"
#include "FillShader.h"
#include "Font.h"
#include "FontSet.h"
#include "Format.h"
#include "GameData.h"
#include "Hardpoint.h"
#include "LocaleInfo.h"
#include "Outfit.h"
#include "Planet.h"
#include "PlayerInfo.h"
#include "Point.h"
#include "Screen.h"
#include "Ship.h"
#include "Sprite.h"
#include "SpriteSet.h"
#include "SpriteShader.h"
#include "UI.h"

#include <algorithm>
#include <limits>
#include <memory>

using namespace std;
using namespace Gettext;

namespace {
	string Tons(int tons)
	{
		return to_string(tons) + nT(" ton", " tons", tons);
	}
}



OutfitterPanel::OutfitterPanel(PlayerInfo &player)
	: ShopPanel(player, true)
{
	for(const pair<const string, Outfit> &it : GameData::Outfits())
		catalog[it.second.Category()].insert(it.first);
	
	// Add owned licenses
	const string PREFIX = "license: ";
	for(auto &it : player.Conditions())
		if(it.first.compare(0, PREFIX.length(), PREFIX) == 0 && it.second > 0)
		{
			const string name = it.first.substr(PREFIX.length()) + " License";
			const Outfit *outfit = GameData::Outfits().Get(name);
			if(outfit)
				catalog[outfit->Category()].insert(name);
		}
	
	if(player.GetPlanet())
		outfitter = player.GetPlanet()->Outfitter();
}


	
void OutfitterPanel::Step()
{
	CheckRefill();
	DoHelp("outfitter");
	ShopPanel::Step();
}



int OutfitterPanel::TileSize() const
{
	return OUTFIT_SIZE;
}



int OutfitterPanel::DrawPlayerShipInfo(const Point &point)
{
	shipInfo.Update(*playerShip, player.FleetDepreciation(), day);
	shipInfo.DrawAttributes(point);
	
	return shipInfo.AttributesHeight();
}



bool OutfitterPanel::HasItem(const string &name) const
{
	const Outfit *outfit = GameData::Outfits().Get(name);
	if((outfitter.Has(outfit) || player.Stock(outfit) > 0) && showForSale)
		return true;
	
	if(player.Cargo().Get(outfit) && (!playerShip || showForSale))
		return true;
	
	for(const Ship *ship : playerShips)
		if(ship->OutfitCount(outfit))
			return true;
	
	if(showForSale && HasLicense(name))
		return true;
	
	return false;
}



void OutfitterPanel::DrawItem(const string &name, const Point &point, int scrollY)
{
	const Outfit *outfit = GameData::Outfits().Get(name);
	zones.emplace_back(point, Point(OUTFIT_SIZE, OUTFIT_SIZE), outfit, scrollY);
	if(point.Y() + OUTFIT_SIZE / 2 < Screen::Top() || point.Y() - OUTFIT_SIZE / 2 > Screen::Bottom())
		return;
	
	bool isSelected = (outfit == selectedOutfit);
	bool isOwned = playerShip && playerShip->OutfitCount(outfit);
	DrawOutfit(*outfit, point, isSelected, isOwned);
	
	// Check if this outfit is a "license".
	bool isLicense = IsLicense(name);
	int mapSize = outfit->Get("map");
	
	const Font &font = FontSet::Get(14);
	const Color &bright = *GameData::Colors().Get("bright");
	if(playerShip || isLicense || mapSize)
	{
		int minCount = numeric_limits<int>::max();
		int maxCount = 0;
		if(isLicense)
			minCount = maxCount = player.GetCondition(LicenseName(name));
		else if(mapSize)
			minCount = maxCount = HasMapped(mapSize);
		else
		{
			for(const Ship *ship : playerShips)
			{
				int count = ship->OutfitCount(outfit);
				minCount = min(minCount, count);
				maxCount = max(maxCount, count);
			}
		}
		
		if(maxCount)
		{
			string label = Format::StringF({T("installed: %1%"), to_string(minCount)});
			if(maxCount > minCount)
				label += Format::StringF({T(" - %1%"), to_string(maxCount)});
			
			Point labelPos = point + Point(-OUTFIT_SIZE / 2 + 20, OUTFIT_SIZE / 2 - 38);
			font.Draw(label, labelPos, bright);
		}
	}
	// Don't show the "in stock" amount if the outfit has an unlimited stock or
	// if it is not something that you can buy.
	int stock = 0;
	if(!outfitter.Has(outfit) && outfit->Get("installable") >= 0.)
		stock = max(0, player.Stock(outfit));
	int cargo = player.Cargo().Get(outfit);
	
	string message;
	if(cargo && stock)
		message = Format::StringF({T("in cargo: %1%, in stock: %2%"), to_string(cargo), to_string(stock)});
	else if(cargo)
		message = Format::StringF({T("in cargo: %1%"), to_string(cargo)});
	else if(stock)
		message = Format::StringF({T("in stock: %1%"), to_string(stock)});
	else if(!outfitter.Has(outfit))
		message = T("(not sold here)");
	if(!message.empty())
	{
		Point pos = point + Point(
			OUTFIT_SIZE / 2 - 20 - font.Width(message),
			OUTFIT_SIZE / 2 - 24);
		font.Draw(message, pos, bright);
	}
}



int OutfitterPanel::DividerOffset() const
{
	return 80;
}



int OutfitterPanel::DetailWidth() const
{
	return 3 * outfitInfo.PanelWidth();
}



int OutfitterPanel::DrawDetails(const Point &center)
{
	if(!selectedOutfit)
		return 0;
	
	outfitInfo.Update(*selectedOutfit, player, CanSell());
	Point offset(outfitInfo.PanelWidth(), 0.);
	
	outfitInfo.DrawDescription(center - offset * 1.5 - Point(0., 10.));
	outfitInfo.DrawRequirements(center - offset * .5 - Point(0., 10.));
	outfitInfo.DrawAttributes(center + offset * .5 - Point(0., 10.));
	
	return outfitInfo.MaximumHeight();
}



bool OutfitterPanel::CanBuy() const
{
	if(!planet || !selectedOutfit)
		return false;
	
	bool isInCargo = player.Cargo().Get(selectedOutfit) && playerShip;
	if(!(outfitter.Has(selectedOutfit) || player.Stock(selectedOutfit) > 0 || isInCargo))
		return false;
	
	int mapSize = selectedOutfit->Get("map");
	if(mapSize > 0 && HasMapped(mapSize))
		return false;
	
	// Determine what you will have to pay to buy this outfit.
	int64_t cost = player.StockDepreciation().Value(selectedOutfit, day);
	// Check that the player has any necessary licenses.
	int64_t licenseCost = LicenseCost(selectedOutfit);
	if(licenseCost < 0)
		return false;
	cost += licenseCost;
	// If you have this in your cargo hold, installing it is free.
	if(cost > player.Accounts().Credits() && !isInCargo)
		return false;
	
	if(HasLicense(selectedOutfit->Identifier()))
		return false;
	
	if(!playerShip)
	{
		double mass = selectedOutfit->Mass();
		return (!mass || player.Cargo().Free() >= mass);
	}
	
	for(const Ship *ship : playerShips)
		if(ShipCanBuy(ship, selectedOutfit))
			return true;
	
	return false;
}



void OutfitterPanel::Buy(bool fromCargo)
{
	int64_t licenseCost = LicenseCost(selectedOutfit);
	if(licenseCost)
	{
		player.Accounts().AddCredits(-licenseCost);
		for(const string &licenseName : selectedOutfit->Licenses())
			if(!player.GetCondition("license: " + licenseName))
				player.Conditions()["license: " + licenseName] = true;
	}
	
	int modifier = Modifier();
	for(int i = 0; i < modifier && CanBuy(); ++i)
	{
		// Special case: maps.
		int mapSize = selectedOutfit->Get("map");
		if(mapSize > 0)
		{
			if(!HasMapped(mapSize))
			{
				DistanceMap distance(player.GetSystem(), mapSize);
				for(const System *system : distance.Systems())
					if(!player.HasVisited(system))
						player.Visit(system);
				int64_t price = player.StockDepreciation().Value(selectedOutfit, day);
				player.Accounts().AddCredits(-price);
			}
			return;
		}
		
		// Special case: licenses.
		if(IsLicense(selectedOutfit->Identifier()))
		{
			int &entry = player.Conditions()[LicenseName(selectedOutfit->Identifier())];
			if(entry <= 0)
			{
				entry = true;
				int64_t price = player.StockDepreciation().Value(selectedOutfit, day);
				player.Accounts().AddCredits(-price);
			}
			return;
		}
		
		if(!playerShip)
		{
			player.Cargo().Add(selectedOutfit);
			int64_t price = player.StockDepreciation().Value(selectedOutfit, day);
			player.Accounts().AddCredits(-price);
			player.AddStock(selectedOutfit, -1);
			continue;
		}
		
		// Find the ships with the fewest number of these outfits.
		const vector<Ship *> shipsToOutfit = GetShipsToOutfit(true);
		
		for(Ship *ship : shipsToOutfit)
		{
			if(!CanBuy())
				return;
		
			if(player.Cargo().Get(selectedOutfit))
				player.Cargo().Remove(selectedOutfit);
			else if(fromCargo || !(player.Stock(selectedOutfit) > 0 || outfitter.Has(selectedOutfit)))
				break;
			else
			{
				int64_t price = player.StockDepreciation().Value(selectedOutfit, day);
				player.Accounts().AddCredits(-price);
				player.AddStock(selectedOutfit, -1);
			}
			ship->AddOutfit(selectedOutfit, 1);
			int required = selectedOutfit->Get("required crew");
			if(required && ship->Crew() + required <= static_cast<int>(ship->Attributes().Get("bunks")))
				ship->AddCrew(required);
			ship->Recharge();
		}
	}
}



void OutfitterPanel::FailBuy() const
{
	if(!selectedOutfit)
		return;
	
	int64_t cost = player.StockDepreciation().Value(selectedOutfit, day);
	int64_t credits = player.Accounts().Credits();
	bool isInCargo = player.Cargo().Get(selectedOutfit);
	if(!isInCargo && cost > credits)
	{
		GetUI()->Push(new Dialog(Format::StringF({T("You cannot buy this outfit, because it costs %1% credits, "
			"and you only have %2%."), Format::Credits(cost), Format::Credits(credits)})));
		return;
	}
	// Check that the player has any necessary licenses.
	int64_t licenseCost = LicenseCost(selectedOutfit);
	if(licenseCost < 0)
	{
		GetUI()->Push(new Dialog(
			T("You cannot buy this outfit, because it requires a license that you don't have.")));
		return;
	}
	if(!isInCargo && cost + licenseCost > credits)
	{
		GetUI()->Push(new Dialog(
			Format::StringF({T("You don't have enough money to buy this outfit, because it will cost you an extra %1% "
				"credits to buy the necessary licenses."), Format::Credits(licenseCost)})));
		return;
	}
	
	if(!(outfitter.Has(selectedOutfit) || player.Stock(selectedOutfit) > 0 || isInCargo))
	{
		// TRANSLATORS: %1%: Planet's noun
		GetUI()->Push(new Dialog(Format::StringF({T("You cannot buy this outfit here. "
			"It is being shown in the list because you have one installed in your ship, "
			"but this %1% does not sell them."), planet->Noun()})));
		return;
	}
	
	if(selectedOutfit->Get("map"))
	{
		GetUI()->Push(new Dialog(T("You have already mapped all the systems shown by this map, "
			"so there is no reason to buy another.")));
		return;
	}
	
	if(HasLicense(selectedOutfit->Identifier()))
	{
		GetUI()->Push(new Dialog(T("You already have one of these licenses, "
			"so there is no reason to buy another.")));
		return;
	}
	
	if(!playerShip)
		return;
	
	double outfitNeeded = -selectedOutfit->Get("outfit space");
	double outfitSpace = playerShip->Attributes().Get("outfit space");
	if(outfitNeeded > outfitSpace)
	{
		GetUI()->Push(new Dialog(Format::StringF({T("You cannot install this outfit, "
			"because it takes up %1% of outfit space, and this ship has %2% free."),
			Tons(outfitNeeded), Tons(outfitSpace)})));
		return;
	}
	
	double weaponNeeded = -selectedOutfit->Get("weapon capacity");
	double weaponSpace = playerShip->Attributes().Get("weapon capacity");
	if(weaponNeeded > weaponSpace)
	{
		GetUI()->Push(new Dialog(Format::StringF({T("Only part of your ship's outfit capacity is usable for weapons. "
			"You cannot install this outfit, because it takes up %1% of weapon space, and this ship has %2% free."),
			Tons(weaponNeeded), Tons(weaponSpace)})));
		return;
	}
	
	double engineNeeded = -selectedOutfit->Get("engine capacity");
	double engineSpace = playerShip->Attributes().Get("engine capacity");
	if(engineNeeded > engineSpace)
	{
		GetUI()->Push(new Dialog(Format::StringF({T("Only part of your ship's outfit capacity is usable for engines. "
			"You cannot install this outfit, because it takes up %1% of engine space, and this ship has %2% free."),
			Tons(engineNeeded), Tons(engineSpace)})));
		return;
	}
	
	if(selectedOutfit->Category() == "Ammunition")
	{
		if(!playerShip->OutfitCount(selectedOutfit))
			GetUI()->Push(new Dialog(T("This outfit is ammunition for a weapon. "
				"You cannot install it without first installing the appropriate weapon.")));
		else
			GetUI()->Push(new Dialog(T("You already have the maximum amount of ammunition for this weapon. "
				"If you want to install more ammunition, you must first install another of these weapons.")));
		return;
	}
	
	int mountsNeeded = -selectedOutfit->Get("turret mounts");
	int mountsFree = playerShip->Attributes().Get("turret mounts");
	if(mountsNeeded && !mountsFree)
	{
		GetUI()->Push(new Dialog(T("This weapon is designed to be installed on a turret mount, "
			"but your ship does not have any unused turret mounts available.")));
		return;
	}
	
	int gunsNeeded = -selectedOutfit->Get("gun ports");
	int gunsFree = playerShip->Attributes().Get("gun ports");
	if(gunsNeeded && !gunsFree)
	{
		GetUI()->Push(new Dialog(T("This weapon is designed to be installed in a gun port, "
			"but your ship does not have any unused gun ports available.")));
		return;
	}
	
	if(selectedOutfit->Get("installable") < 0.)
	{
		GetUI()->Push(new Dialog(T("This item is not an outfit that can be installed in a ship.")));
		return;
	}
	
	if(!playerShip->Attributes().CanAdd(*selectedOutfit, 1))
	{
		GetUI()->Push(new Dialog(T("You cannot install this outfit in your ship, "
			"because it would reduce one of your ship's attributes to a negative amount. "
			"For example, it may use up more cargo space than you have left.")));
		return;
	}
}



bool OutfitterPanel::CanSell(bool toCargo) const
{
	if(!planet || !selectedOutfit)
		return false;
	
	if(!toCargo && player.Cargo().Get(selectedOutfit))
		return true;
	
	for(const Ship *ship : playerShips)
		if(ShipCanSell(ship, selectedOutfit))
			return true;
	
	return false;
}



void OutfitterPanel::Sell(bool toCargo)
{
	if(!toCargo && player.Cargo().Get(selectedOutfit))
	{
		player.Cargo().Remove(selectedOutfit);
		int64_t price = player.FleetDepreciation().Value(selectedOutfit, day);
		player.Accounts().AddCredits(price);
		player.AddStock(selectedOutfit, 1);
	}
	else
	{
		// Get the ships that have the most of this outfit installed.
		const vector<Ship *> shipsToOutfit = GetShipsToOutfit();
		
		for(Ship *ship : shipsToOutfit)
		{
			ship->AddOutfit(selectedOutfit, -1);
			if(selectedOutfit->Get("required crew"))
				ship->AddCrew(-selectedOutfit->Get("required crew"));
			ship->Recharge();
			if(toCargo && player.Cargo().Add(selectedOutfit))
			{
				// Transfer to cargo completed.
			}
			else
			{
				int64_t price = player.FleetDepreciation().Value(selectedOutfit, day);
				player.Accounts().AddCredits(price);
				player.AddStock(selectedOutfit, 1);
			}
			
			const Outfit *ammo = selectedOutfit->Ammo();
			if(ammo && ship->OutfitCount(ammo))
			{
				// Determine how many of this ammo I must sell to also sell the launcher.
				int mustSell = 0;
				for(const pair<const char *, double> &it : ship->Attributes().Attributes())
					if(it.second < 0.)
						mustSell = max<int>(mustSell, it.second / ammo->Get(it.first));
				
				if(mustSell)
				{
					ship->AddOutfit(ammo, -mustSell);
					if(toCargo)
						mustSell -= player.Cargo().Add(ammo, mustSell);
					if(mustSell)
					{
						int64_t price = player.FleetDepreciation().Value(ammo, day, mustSell);
						player.Accounts().AddCredits(price);
						player.AddStock(ammo, mustSell);
					}
				}
			}
		}
	}
}



void OutfitterPanel::FailSell(bool toCargo) const
{
	if(!planet || !selectedOutfit)
		return;
	else if(selectedOutfit->Get("map"))
	{
		if(toCargo)
			GetUI()->Push(new Dialog(T("You cannot uninstall maps. Once you obtain one, it is yours permanently.")));
		else
			GetUI()->Push(new Dialog(T("You cannot sell maps. Once you obtain one, it is yours permanently.")));
	}
	else if(HasLicense(selectedOutfit->Identifier()))
	{
		if(toCargo)
			GetUI()->Push(new Dialog(T("You cannot uninstall licenses. Once you obtain one, it is yours permanently.")));
		else
			GetUI()->Push(new Dialog(T("You cannot sell licenses. Once you obtain one, it is yours permanently.")));
	}
	else
	{
		bool hasOutfit = !toCargo && player.Cargo().Get(selectedOutfit);
		for(const Ship *ship : playerShips)
			if(ship->OutfitCount(selectedOutfit))
			{
				hasOutfit = true;
				break;
			}
		if(!hasOutfit)
		{
			if(toCargo)
				GetUI()->Push(new Dialog(T("You do not have any of these outfits to uninstall.")));
			else
				GetUI()->Push(new Dialog(T("You do not have any of these outfits to sell.")));
		}
		else
		{
			for(const Ship *ship : playerShips)
				for(const pair<const char *, double> &it : selectedOutfit->Attributes())
					if(ship->Attributes().Get(it.first) < it.second)
					{
						const string attrName = LocaleInfo::TranslateData(it.first, "Attribute");
						for(const auto &sit : ship->Outfits())
							if(sit.first->Get(it.first) < 0.)
							{
								if(toCargo)
									GetUI()->Push(new Dialog(Format::StringF({T("You cannot uninstall this outfit, "
										"because that would cause your ship's \"%1%\" value "
										"to be reduced to less than zero. "
										"To uninstall this outfit, you must uninstall the %2% outfit first."),
										attrName, sit.first->Name()})));
								else
									GetUI()->Push(new Dialog(Format::StringF({T("You cannot sell this outfit, "
										"because that would cause your ship's \"%1%\" value "
										"to be reduced to less than zero. "
										"To sell this outfit, you must sell the %2% outfit first."),
										attrName, sit.first->Name()})));
								return;
							}
						if(toCargo)
							GetUI()->Push(new Dialog(Format::StringF({T("You cannot uninstall this outfit, "
								"because that would cause your ship's \"%1%\" value "
								"to be reduced to less than zero."), attrName})));
						else
							GetUI()->Push(new Dialog(Format::StringF({T("You cannot sell this outfit, "
								"because that would cause your ship's \"%1%\" value "
								"to be reduced to less than zero."), attrName})));
						return;
					}
			if(toCargo)
				GetUI()->Push(new Dialog(T("You cannot uninstall this outfit, "
					"because something else in your ship depends on it.")));
			else
				GetUI()->Push(new Dialog(T("You cannot sell this outfit, "
					"because something else in your ship depends on it.")));
		}
	}
}



bool OutfitterPanel::ShouldHighlight(const Ship *ship)
{
	if(!selectedOutfit)
		return false;
	
	if(hoverButton == 'b')
		return CanBuy() && ShipCanBuy(ship, selectedOutfit);
	else if(hoverButton == 's')
		return CanSell() && ShipCanSell(ship, selectedOutfit);
	
	return false;
}



void OutfitterPanel::DrawKey()
{
	const Sprite *back = SpriteSet::Get("ui/outfitter key");
	SpriteShader::Draw(back, Screen::BottomLeft() + .5 * Point(back->Width(), -back->Height()));
	
	Font font = FontSet::Get(14);
	Color color[2] = {*GameData::Colors().Get("medium"), *GameData::Colors().Get("bright")};
	const Sprite *box[2] = {SpriteSet::Get("ui/unchecked"), SpriteSet::Get("ui/checked")};
	
	Point pos = Screen::BottomLeft() + Point(10., -30.);
	Point off = Point(10., -.5 * font.Height());
	SpriteShader::Draw(box[showForSale], pos);
	font.Draw(T("Show outfits for sale"), pos + off, color[showForSale]);
	AddZone(Rectangle(pos + Point(80., 0.), Point(180., 20.)), [this](){ ToggleForSale(); });
	
	bool showCargo = !playerShip;
	pos.Y() += 20.;
	SpriteShader::Draw(box[showCargo], pos);
	font.Draw(T("Show outfits in cargo"), pos + off, color[showCargo]);
	AddZone(Rectangle(pos + Point(80., 0.), Point(180., 20.)), [this](){ ToggleCargo(); });
}



void OutfitterPanel::ToggleForSale()
{
	showForSale = !showForSale;
	
	ShopPanel::ToggleForSale();
}



void OutfitterPanel::ToggleCargo()
{
	if(playerShip)
	{
		previousShip = playerShip;
		playerShip = nullptr;
		previousShips = playerShips;
		playerShips.clear();
	}
	else if(previousShip)
	{
		playerShip = previousShip;
		playerShips = previousShips;
	}
	else
	{
		playerShip = player.Flagship();
		if(playerShip)
			playerShips.insert(playerShip);
	}
	
	ShopPanel::ToggleCargo();
}



bool OutfitterPanel::ShipCanBuy(const Ship *ship, const Outfit *outfit)
{
	return (ship->Attributes().CanAdd(*outfit, 1) > 0);
}



bool OutfitterPanel::ShipCanSell(const Ship *ship, const Outfit *outfit)
{
	if(!ship->OutfitCount(outfit))
		return false;
	
	// If this outfit requires ammo, check if we could sell it if we sold all
	// the ammo for it first.
	const Outfit *ammo = outfit->Ammo();
	if(ammo && ship->OutfitCount(ammo))
	{
		Outfit attributes = ship->Attributes();
		attributes.Add(*ammo, -ship->OutfitCount(ammo));
		return attributes.CanAdd(*outfit, -1);
	}
	
	// Now, check whether this ship can sell this outfit.
	return ship->Attributes().CanAdd(*outfit, -1);
}



void OutfitterPanel::DrawOutfit(const Outfit &outfit, const Point &center, bool isSelected, bool isOwned)
{
	const Sprite *thumbnail = outfit.Thumbnail();
	const Sprite *back = SpriteSet::Get(
		isSelected ? "ui/outfitter selected" : "ui/outfitter unselected");
	SpriteShader::Draw(back, center);
	SpriteShader::Draw(thumbnail, center);
	
	// Draw the outfit name.
	const string &name = outfit.Name();
	const Font &font = FontSet::Get(14);
	Point offset(-.5f * font.Width(name), -.5f * OUTFIT_SIZE + 10.f);
	font.Draw(name, center + offset, Color((isSelected | isOwned) ? .8 : .5, 0.));
}



bool OutfitterPanel::HasMapped(int mapSize) const
{
	DistanceMap distance(player.GetSystem(), mapSize);
	for(const System *system : distance.Systems())
		if(!player.HasVisited(system))
			return false;
	
	return true;
}



bool OutfitterPanel::IsLicense(const string &name) const
{
	static const string &LICENSE = " License";
	if(name.length() < LICENSE.length())
		return false;
	if(name.compare(name.length() - LICENSE.length(), LICENSE.length(), LICENSE))
		return false;
	
	return true;
}



bool OutfitterPanel::HasLicense(const string &name) const
{
	return (IsLicense(name) && player.GetCondition(LicenseName(name)) > 0);
}



string OutfitterPanel::LicenseName(const string &name) const
{
	static const string &LICENSE = " License";
	return "license: " + name.substr(0, name.length() - LICENSE.length());
}



void OutfitterPanel::CheckRefill()
{
	if(checkedRefill)
		return;
	checkedRefill = true;
	
	int count = 0;
	map<const Outfit *, int> needed;
	for(const shared_ptr<Ship> &ship : player.Ships())
	{
		if(ship->GetSystem() != player.GetSystem() || ship->IsDisabled())
			continue;
		
		++count;
		set<const Outfit *> toRefill;
		for(const Hardpoint &it : ship->Weapons())
			if(it.GetOutfit() && it.GetOutfit()->Ammo())
				toRefill.insert(it.GetOutfit()->Ammo());
		
		for(const Outfit *outfit : toRefill)
		{
			int amount = ship->Attributes().CanAdd(*outfit, numeric_limits<int>::max());
			if(amount > 0 && (outfitter.Has(outfit) || player.Stock(outfit) > 0 || player.Cargo().Get(outfit)))
				needed[outfit] += amount;
		}
	}
	
	int64_t cost = 0;
	for(auto &it : needed)
	{
		// Don't count cost of anything installed from cargo.
		it.second = max(0, it.second - player.Cargo().Get(it.first));
		if(!outfitter.Has(it.first))
			it.second = min(it.second, max(0, player.Stock(it.first)));
		cost += player.StockDepreciation().Value(it.first, day, it.second);
	}
	if(!needed.empty() && cost < player.Accounts().Credits())
	{
		string message = T("Do you want to reload all the ammunition for your ");
		message += nT("ship?", "ships?", count);
		if(cost)
			message += Format::StringF({T(" It will cost %1% credits."), Format::Credits(cost)});
		GetUI()->Push(new Dialog(this, &OutfitterPanel::Refill, message));
	}
}



void OutfitterPanel::Refill()
{
	for(const shared_ptr<Ship> &ship : player.Ships())
	{
		if(ship->GetSystem() != player.GetSystem() || ship->IsDisabled())
			continue;
		
		set<const Outfit *> toRefill;
		for(const Hardpoint &it : ship->Weapons())
			if(it.GetOutfit() && it.GetOutfit()->Ammo())
				toRefill.insert(it.GetOutfit()->Ammo());
		
		for(const Outfit *outfit : toRefill)
		{
			int neededAmmo = ship->Attributes().CanAdd(*outfit, numeric_limits<int>::max());
			if(neededAmmo)
			{
				// Fill first from any stockpiles in cargo.
				int fromCargo = player.Cargo().Remove(outfit, neededAmmo);
				neededAmmo -= fromCargo;
				// Then, buy at reduced (or full) price.
				int available = outfitter.Has(outfit) ? neededAmmo : max<int>(0, player.Stock(outfit));
				if(neededAmmo && available > 0)
				{
					int64_t price = player.StockDepreciation().Value(outfit, day, available);
					player.Accounts().AddCredits(-price);
					player.AddStock(outfit, -available);
				}
				ship->AddOutfit(outfit, available + fromCargo);
			}
		}
	}
}



// Determine which ships of the selected ships should be referenced in this
// iteration of Buy / Sell.
const vector<Ship *> OutfitterPanel::GetShipsToOutfit(bool isBuy) const
{
	vector<Ship *> shipsToOutfit;
	int compareValue = isBuy ? numeric_limits<int>::max() : 0;
	int compareMod = 2 * isBuy - 1;
	for(Ship *ship : playerShips)
	{
		if((isBuy && !ShipCanBuy(ship, selectedOutfit))
				|| (!isBuy && !ShipCanSell(ship, selectedOutfit)))
			continue;
		
		int count = ship->OutfitCount(selectedOutfit);
		if(compareMod * count < compareMod * compareValue)
		{
			shipsToOutfit.clear();
			compareValue = count;
		}
		if(count == compareValue)
			shipsToOutfit.push_back(ship);
	}
	
	return shipsToOutfit;
}
