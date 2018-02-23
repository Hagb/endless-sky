/* TradingPanel.cpp
Copyright (c) 2014 by Michael Zahniser

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#include "TradingPanel.h"

#include "Color.h"
#include "Command.h"
#include "FillShader.h"
#include "text/Font.h"
#include "text/FontSet.h"
#include "text/Format.h"
#include "GameData.h"
#include "text/Gettext.h"
#include "Information.h"
#include "Interface.h"
#include "MapDetailPanel.h"
#include "Messages.h"
#include "Outfit.h"
#include "PlayerInfo.h"
#include "System.h"
#include "UI.h"

#include <algorithm>
#include <string>

using namespace std;
using namespace Gettext;

namespace {
	const T_ TRADE_LEVEL[] = {
		T_("(very low)"),
		T_("(low)"),
		T_("(medium)"),
		T_("(high)"),
		T_("(very high)")
	};
	
	const int MIN_X = -310;
	const int MAX_X = 190;
	
	const int NAME_X = -290;
	const int PRICE_X = -150;
	const int LEVEL_X = -110;
	const int BUY_X = 0;
	const int SELL_X = 60;
	const int HOLD_X = 120;
	
	const int FIRST_Y = 80;
}



TradingPanel::TradingPanel(PlayerInfo &player)
	: player(player), system(*player.GetSystem()), COMMODITY_COUNT(GameData::Commodities().size())
{
	SetTrapAllEvents(false);
}



TradingPanel::~TradingPanel()
{
	if(profit)
	{
		string message;
		if(profit < 0)
			message = Format::StringF(T("You sold %1% %2% of cargo at a loss of %3% credits."),
				to_string(tonsSold), nT("ton", "tons", "TradingPanel sold", tonsSold), Format::Credits(-profit));
		else
			message = Format::StringF(T("You sold %1% %2% of cargo for a total profit of %3% credits."),
				to_string(tonsSold), nT("ton", "tons", "TradingPanel sold", tonsSold), Format::Credits(profit));
		Messages::Add(message);
	}
}


	
void TradingPanel::Step()
{
	DoHelp("trading");
}



void TradingPanel::Draw()
{
	const Color &back = *GameData::Colors().Get("faint");
	int selectedRow = player.MapColoring();
	if(selectedRow >= 0 && selectedRow < COMMODITY_COUNT)
		FillShader::Fill(Point(-60., FIRST_Y + 20 * selectedRow + 33), Point(480., 20.), back);
	
	const Font &font = FontSet::Get(14);
	const Color &unselected = *GameData::Colors().Get("medium");
	const Color &selected = *GameData::Colors().Get("bright");
	
	int y = FIRST_Y;
	FillShader::Fill(Point(-60., y + 15.), Point(480., 1.), unselected);
	
	font.Draw(T("Commodity"), Point(NAME_X, y), selected);
	font.Draw(T("Price"), Point(PRICE_X, y), selected);
	
	string mod = "x " + to_string(Modifier());
	font.Draw(mod, Point(BUY_X, y), unselected);
	font.Draw(mod, Point(SELL_X, y), unselected);
	
	font.Draw(T("In Hold"), Point(HOLD_X, y), selected);
	
	y += 5;
	int lastY = y + 20 * COMMODITY_COUNT + 25;
	font.Draw(T("free:"), Point(SELL_X + 5, lastY), selected);
	font.Draw(to_string(player.Cargo().Free()), Point(HOLD_X, lastY), selected);
	
	int outfits = player.Cargo().OutfitsSize();
	int missionCargo = player.Cargo().MissionCargoSize();
	sellOutfits = false;
	if(player.Cargo().HasOutfits() || missionCargo)
	{
		bool hasOutfits = false;
		bool hasHarvested = false;
		for(const auto &it : player.Cargo().Outfits())
			if(it.second)
			{
				bool isHarvested = (it.first->Get("installable") < 0.);
				(isHarvested ? hasHarvested : hasOutfits) = true;
			}
		sellOutfits = (hasOutfits && !hasHarvested);
		
		string fmt;
		if(hasHarvested && missionCargo)
			fmt = T("%1% %2% of mission cargo and other items.");
		else if(hasOutfits && missionCargo)
			fmt = T("%1% %2% of outfits and mission cargo.");
		else if(hasOutfits && hasHarvested)
			fmt = T("%1% %2% of outfits and harvested materials.");
		else if(hasOutfits)
			fmt = T("%1% %2% of outfits.");
		else if(hasHarvested)
			fmt = T("%1% %2% of harvested materials.");
		else
			fmt = T("%1% %2% of mission cargo.");
		const string str = Format::StringF(fmt, to_string(outfits + missionCargo),
			nT("ton", "tons", "TradingPanel hold", outfits + missionCargo));
		font.Draw(str, Point(NAME_X, lastY), unselected);
	}
	
	int i = 0;
	bool canSell = false;
	bool canBuy = false;
	for(const Trade::Commodity &commodity : GameData::Commodities())
	{
		y += 20;
		int price = system.Trade(commodity.name.Original());
		int hold = player.Cargo().Get(commodity.name.Original());
		
		bool isSelected = (i++ == selectedRow);
		const Color &color = (isSelected ? selected : unselected);
		font.Draw(commodity.name.Str(), Point(NAME_X, y), color);
		
		if(price)
		{
			canBuy |= isSelected;
			font.Draw(to_string(price), Point(PRICE_X, y), color);
		
			int basis = player.GetBasis(commodity.name.Original());
			if(basis && basis != price && hold)
			{
				string profit = Format::StringF(T("(profit: %1%)"), to_string(price - basis));
				font.Draw(profit, Point(LEVEL_X, y), color);
			}
			else
			{
				int level = (price - commodity.low);
				if(level < 0)
					level = 0;
				else if(level >= (commodity.high - commodity.low))
					level = 4;
				else
					level = (5 * level) / (commodity.high - commodity.low);
				font.Draw(TRADE_LEVEL[level].Str(), Point(LEVEL_X, y), color);
			}
		
			font.Draw(T("[buy]"), Point(BUY_X, y), color);
			font.Draw(T("[sell]"), Point(SELL_X, y), color);
		}
		else
		{
			font.Draw(T("----", "TradingPanel"), Point(PRICE_X, y), color);
			font.Draw(T("(not for sale)"), Point(LEVEL_X, y), color);
		}
		
		if(hold)
		{
			sellOutfits = false;
			canSell |= (price != 0);
			font.Draw(to_string(hold), Point(HOLD_X, y), selected);
		}
	}
	
	const Interface *interface = GameData::Interfaces().Get("trade");
	Information info;
	if(sellOutfits)
		info.SetCondition("can sell outfits");
	else if(player.Cargo().HasOutfits() || canSell)
		info.SetCondition("can sell");
	if(player.Cargo().Free() > 0 && canBuy)
		info.SetCondition("can buy");
	interface->Draw(info, this);
}



// Only override the ones you need; the default action is to return false.
bool TradingPanel::KeyDown(SDL_Keycode key, Uint16 mod, const Command &command, bool isNewPress)
{
	if(key == SDLK_UP)
		player.SetMapColoring(max(0, player.MapColoring() - 1));
	else if(key == SDLK_DOWN)
		player.SetMapColoring(max(0, min(COMMODITY_COUNT - 1, player.MapColoring() + 1)));
	else if(key == SDLK_EQUALS || key == SDLK_KP_PLUS || key == SDLK_PLUS || key == SDLK_RETURN || key == SDLK_SPACE)
		Buy(1);
	else if(key == SDLK_MINUS || key == SDLK_KP_MINUS || key == SDLK_BACKSPACE || key == SDLK_DELETE)
		Buy(-1);
	else if(key == 'B' || (key == 'b' && (mod & KMOD_SHIFT)))
		Buy(1000000000);
	else if(key == 'S' || (key == 's' && (mod & KMOD_SHIFT)))
	{
		for(const auto &it : GameData::Commodities())
		{
			int64_t amount = player.Cargo().Get(it.name.Original());
			int64_t price = system.Trade(it.name.Original());
			if(!price || !amount)
				continue;
			
			int64_t basis = player.GetBasis(it.name.Original(), -amount);
			player.AdjustBasis(it.name.Original(), basis);
			profit += amount * price + basis;
			tonsSold += amount;
			
			player.Cargo().Remove(it.name.Original(), amount);
			player.Accounts().AddCredits(amount * price);
			GameData::AddPurchase(system, it.name.Original(), -amount);
		}
		int day = player.GetDate().DaysSinceEpoch();
		for(const auto &it : player.Cargo().Outfits())
		{
			if(it.first->Get("installable") >= 0. && !sellOutfits)
				continue;
			
			int64_t value = player.FleetDepreciation().Value(it.first, day, it.second);
			profit += value;
			tonsSold += static_cast<int>(it.second * it.first->Mass());
			
			player.AddStock(it.first, it.second);
			player.Accounts().AddCredits(value);
			player.Cargo().Remove(it.first, it.second);
		}
	}
	else if(command.Has(Command::MAP))
		GetUI()->Push(new MapDetailPanel(player));
	else
		return false;
	
	return true;
}



bool TradingPanel::Click(int x, int y, int clicks)
{
	int maxY = FIRST_Y + 25 + 20 * COMMODITY_COUNT;
	if(x >= MIN_X && x <= MAX_X && y >= FIRST_Y + 25 && y < maxY)
	{
		player.SetMapColoring((y - FIRST_Y - 25) / 20);
		if(x >= BUY_X && x < SELL_X)
			Buy(1);
		else if(x >= SELL_X && x < HOLD_X)
			Buy(-1);
	}
	else
		return false;
	
	return true;
}



void TradingPanel::Buy(int64_t amount)
{
	int selectedRow = player.MapColoring();
	if(selectedRow < 0 || selectedRow >= COMMODITY_COUNT)
		return;
	
	amount *= Modifier();
	const string &type = GameData::Commodities()[selectedRow].name.Original();
	int64_t price = system.Trade(type);
	if(!price)
		return;
	
	if(amount > 0)
	{
		amount = min(amount, min<int64_t>(player.Cargo().Free(), player.Accounts().Credits() / price));
		player.AdjustBasis(type, amount * price);
	}
	else
	{
		// Selling cargo:
		amount = max<int64_t>(amount, -player.Cargo().Get(type));
		
		int64_t basis = player.GetBasis(type, amount);
		player.AdjustBasis(type, basis);
		profit += -amount * price + basis;
		tonsSold += -amount;
	}
	amount = player.Cargo().Add(type, amount);
	player.Accounts().AddCredits(-amount * price);
	GameData::AddPurchase(system, type, amount);
}
