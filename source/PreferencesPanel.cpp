/* PreferencesPanel.cpp
Copyright (c) 2014 by Michael Zahniser

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#include "PreferencesPanel.h"

#include "text/alignment.hpp"
#include "Audio.h"
#include "Color.h"
#include "Dialog.h"
#include "Files.h"
#include "text/Font.h"
#include "text/FontSet.h"
#include "text/FontUtilities.h"
#include "GameData.h"
#include "text/Gettext.h"
#include "Information.h"
#include "Interface.h"
#include "text/layout.hpp"
#include "Preferences.h"
#include "Screen.h"
#include "Sprite.h"
#include "SpriteSet.h"
#include "SpriteShader.h"
#include "StarField.h"
#include "text/Table.h"
#include "text/truncate.hpp"
#include "UI.h"

#include "gl_header.h"
#include <SDL2/SDL.h>

#include <algorithm>

using namespace std;
using namespace Gettext;

namespace {
	// Settings that require special handling.
	const string ZOOM_FACTOR = G("Main zoom factor");
	const int ZOOM_FACTOR_MIN = 100;
	const int ZOOM_FACTOR_MAX = 200;
	const int ZOOM_FACTOR_INCREMENT = 10;
	const string VIEW_ZOOM_FACTOR = G("View zoom factor");
	const string VSYNC_SETTING = G("VSync");
	const string EXPEND_AMMO = G("Escorts expend ammo");
	const string TURRET_TRACKING = G("Turret tracking");
	const string FOCUS_PREFERENCE = "Turrets focus fire";
	const string FRUGAL_ESCORTS = "Escorts use ammo frugally";
	const string REACTIVATE_HELP = G("Reactivate first-time help");
	const string SCROLL_SPEED = G("Scroll speed");
	const string FIGHTER_REPAIR = G("Repair fighters in");
	const string SHIP_OUTLINES = G("Ship outlines in shops");
	const string LANGUAGE = G("Language");
	const string FULLNAME = G("Fullname format");
}



PreferencesPanel::PreferencesPanel()
	: editing(-1), selected(0), hover(-1)
{
	if(!GameData::PluginAboutText().empty())
		selectedPlugin = GameData::PluginAboutText().begin()->first;
	
	SetIsFullScreen(true);
}



// Draw this panel.
void PreferencesPanel::Draw()
{
	glClear(GL_COLOR_BUFFER_BIT);
	GameData::Background().Draw(Point(), Point());
	
	Information info;
	info.SetBar("volume", Audio::Volume());
	GameData::Interfaces().Get("menu background")->Draw(info, this);
	string pageName = (page == 'c' ? "controls" : page == 's' ? "settings" : "plugins");
	GameData::Interfaces().Get(pageName)->Draw(info, this);
	GameData::Interfaces().Get("preferences")->Draw(info, this);
	
	zones.clear();
	prefZones.clear();
	pluginZones.clear();
	if(page == 'c')
		DrawControls();
	else if(page == 's')
		DrawSettings();
	else if(page == 'p')
		DrawPlugins();
}



bool PreferencesPanel::KeyDown(SDL_Keycode key, Uint16 mod, const Command &command, bool isNewPress)
{
	if(static_cast<unsigned>(editing) < zones.size())
	{
		Command::SetKey(zones[editing].Value(), key);
		EndEditing();
		return true;
	}
	
	if(key == SDLK_DOWN && static_cast<unsigned>(selected + 1) < zones.size())
		++selected;
	else if(key == SDLK_UP && selected > 0)
		--selected;
	else if(key == SDLK_RETURN)
		editing = selected;
	else if(key == 'b' || command.Has(Command::MENU) || (key == 'w' && (mod & (KMOD_CTRL | KMOD_GUI))))
		Exit();
	else if(key == 'c' || key == 's' || key == 'p')
		page = key;
	else
		return false;
	
	return true;
}



bool PreferencesPanel::Click(int x, int y, int clicks)
{
	EndEditing();
	
	if(x >= 265 && x < 295 && y >= -220 && y < 70)
	{
		Audio::SetVolume((20 - y) / 200.);
		Audio::Play(Audio::Get("warder"));
		return true;
	}
	
	Point point(x, y);
	for(unsigned index = 0; index < zones.size(); ++index)
		if(zones[index].Contains(point))
			editing = selected = index;
	
	for(const auto &zone : prefZones)
		if(zone.Contains(point))
		{
			// For some settings, clicking the option does more than just toggle a
			// boolean state keyed by the option's name.
			if(zone.Value() == ZOOM_FACTOR)
			{
				int newZoom = Screen::UserZoom() + ZOOM_FACTOR_INCREMENT;
				Screen::SetZoom(newZoom);
				if(newZoom > ZOOM_FACTOR_MAX || Screen::Zoom() != newZoom)
				{
					// Notify the user why setting the zoom any higher isn't permitted.
					// Only show this if it's not possible to zoom the view at all, as
					// otherwise the dialog will show every time, which is annoying.
					if(newZoom == ZOOM_FACTOR_MIN + ZOOM_FACTOR_INCREMENT)
						GetUI()->Push(new Dialog(
							T("Your screen resolution is too low to support a zoom level above 100%.")));
					Screen::SetZoom(ZOOM_FACTOR_MIN);
				}
				// Convert to raw window coordinates, at the new zoom level.
				point *= Screen::Zoom() / 100.;
				point += .5 * Point(Screen::RawWidth(), Screen::RawHeight());
				SDL_WarpMouseInWindow(nullptr, point.X(), point.Y());
			}
			else if(zone.Value() == VIEW_ZOOM_FACTOR)
			{
				// Increase the zoom factor unless it is at the maximum. In that
				// case, cycle around to the lowest zoom factor.
				if(!Preferences::ZoomViewIn())
					while(Preferences::ZoomViewOut()) {}
			}
			else if(zone.Value() == VSYNC_SETTING)
			{
				if(!Preferences::ToggleVSync())
					GetUI()->Push(new Dialog(
						T("Unable to change VSync state. (Your system's graphics settings may be controlling it instead.)")));
			}
			else if(zone.Value() == EXPEND_AMMO)
				Preferences::ToggleAmmoUsage();
			else if(zone.Value() == TURRET_TRACKING)
				Preferences::Set(FOCUS_PREFERENCE, !Preferences::Has(FOCUS_PREFERENCE));
			else if(zone.Value() == REACTIVATE_HELP)
			{
				for(const auto &it : GameData::HelpTemplates())
					Preferences::Set("help: " + it.first, false);
			}
			else if(zone.Value() == SCROLL_SPEED)
			{
				// Toggle between three different speeds.
				int speed = Preferences::ScrollSpeed() + 20;
				if(speed > 60)
					speed = 20;
				Preferences::SetScrollSpeed(speed);
			}
			else if(zone.Value() == LANGUAGE)
				Preferences::ToggleLanguage();
			else if(zone.Value() == FULLNAME)
				Preferences::ToggleFullnameFormat();
			// All other options are handled by just toggling the boolean state.
			else
				Preferences::Set(zone.Value(), !Preferences::Has(zone.Value()));
			break;
		}
	
	for(const auto &zone : pluginZones)
		if(zone.Contains(point))
		{
			selectedPlugin = zone.Value();
			break;
		}
	
	return true;
}



bool PreferencesPanel::Hover(int x, int y)
{
	hoverPoint = Point(x, y);
	
	hover = -1;
	for(unsigned index = 0; index < zones.size(); ++index)
		if(zones[index].Contains(hoverPoint))
			hover = index;
	
	hoverPreference.clear();
	for(const auto &zone : prefZones)
		if(zone.Contains(hoverPoint))
			hoverPreference = zone.Value();
	
	hoverPlugin.clear();
	for(const auto &zone : pluginZones)
		if(zone.Contains(hoverPoint))
			hoverPlugin = zone.Value();
	
	return true;
}



// Change the value being hovered over in the direction of the scroll.
bool PreferencesPanel::Scroll(double dx, double dy)
{
	if(!dy || hoverPreference.empty())
		return false;
	
	if(hoverPreference == ZOOM_FACTOR)
	{
		int zoom = Screen::UserZoom();
		if(dy < 0. && zoom > ZOOM_FACTOR_MIN)
			zoom -= ZOOM_FACTOR_INCREMENT;
		if(dy > 0. && zoom < ZOOM_FACTOR_MAX)
			zoom += ZOOM_FACTOR_INCREMENT;
		
		Screen::SetZoom(zoom);
		if(Screen::Zoom() != zoom)
			Screen::SetZoom(Screen::Zoom());
		
		// Convert to raw window coordinates, at the new zoom level.
		Point point = hoverPoint * (Screen::Zoom() / 100.);
		point += .5 * Point(Screen::RawWidth(), Screen::RawHeight());
		SDL_WarpMouseInWindow(nullptr, point.X(), point.Y());
	}
	else if(hoverPreference == VIEW_ZOOM_FACTOR)
	{
		if(dy < 0.)
			Preferences::ZoomViewOut();
		else
			Preferences::ZoomViewIn();
	}
	else if(hoverPreference == SCROLL_SPEED)
	{
		int speed = Preferences::ScrollSpeed();
		if(dy < 0.)
			speed = max(20, speed - 20);
		else
			speed = min(60, speed + 20);
		Preferences::SetScrollSpeed(speed);
	}
	return true;
}



void PreferencesPanel::EndEditing()
{
	editing = -1;
}



void PreferencesPanel::DrawControls()
{
	const Color &back = *GameData::Colors().Get("faint");
	const Color &dim = *GameData::Colors().Get("dim");
	const Color &medium = *GameData::Colors().Get("medium");
	const Color &bright = *GameData::Colors().Get("bright");
	
	// Check for conflicts.
	const Color &warning = *GameData::Colors().Get("warning conflict");
	
	Table table;
	table.AddColumn(-115, {230, Alignment::LEFT});
	table.AddColumn(115, {230, Alignment::RIGHT});
	table.SetUnderline(-120, 120);
	
	int firstY = -248;
	table.DrawAt(Point(-130, firstY));
	
	static const string CATEGORIES[] = {
		G("Navigation"),
		G("Weapons"),
		G("Targeting"),
		G("Interface"),
		G("Fleet")
	};
	const string *category = CATEGORIES;
	static const Command COMMANDS[] = {
		Command::NONE,
		Command::FORWARD,
		Command::LEFT,
		Command::RIGHT,
		Command::BACK,
		Command::AFTERBURNER,
		Command::LAND,
		Command::JUMP,
		Command::NONE,
		Command::PRIMARY,
		Command::SELECT,
		Command::SECONDARY,
		Command::CLOAK,
		Command::NONE,
		Command::NEAREST,
		Command::TARGET,
		Command::HAIL,
		Command::BOARD,
		Command::SCAN,
		Command::NONE,
		Command::MENU,
		Command::MAP,
		Command::INFO,
		Command::FULLSCREEN,
		Command::FASTFORWARD,
		Command::NONE,
		Command::DEPLOY,
		Command::FIGHT,
		Command::GATHER,
		Command::HOLD,
		Command::AMMO
	};
	static const Command *BREAK = &COMMANDS[19];
	for(const Command &command : COMMANDS)
	{
		// The "BREAK" line is where to go to the next column.
		if(&command == BREAK)
			table.DrawAt(Point(130, firstY));
		
		if(!command)
		{
			table.DrawGap(10);
			table.DrawUnderline(medium);
			if(category != end(CATEGORIES))
				table.Draw(T(*category++), bright);
			else
				table.Advance();
			table.Draw(T("Key"), bright);
			table.DrawGap(5);
		}
		else
		{
			int index = zones.size();
			// Mark conflicts.
			bool isConflicted = command.HasConflict();
			bool isEditing = (index == editing);
			if(isConflicted || isEditing)
			{
				table.SetHighlight(56, 120);
				table.DrawHighlight(isEditing ? dim: warning);
			}
			
			// Mark the selected row.
			bool isHovering = (index == hover && !isEditing);
			if(!isHovering && index == selected)
			{
				table.SetHighlight(-120, 54);
				table.DrawHighlight(back);
			}
			
			// Highlight whichever row the mouse hovers over.
			table.SetHighlight(-120, 120);
			if(isHovering)
				table.DrawHighlight(back);
			
			zones.emplace_back(table.GetCenterPoint(), table.GetRowSize(), command);
			
			table.Draw(command.Description(), medium);
			table.Draw(command.KeyName(), isEditing ? bright : medium);
		}
	}
	
	Table shiftTable;
	shiftTable.AddColumn(125, {150, Alignment::RIGHT});
	shiftTable.SetUnderline(0, 130);
	shiftTable.DrawAt(Point(-400, 52));
	
	shiftTable.DrawUnderline(medium);
	shiftTable.Draw(T("With &lt;shift&gt; key"), bright);
	shiftTable.DrawGap(5);
	shiftTable.Draw(T("Select nearest ship"), medium);
	shiftTable.Draw(T("Select next escort"), medium);
	shiftTable.Draw(T("Talk to planet"), medium);
	shiftTable.Draw(T("Board disabled escort"), medium);
}



void PreferencesPanel::DrawSettings()
{
	const Color &back = *GameData::Colors().Get("faint");
	const Color &dim = *GameData::Colors().Get("dim");
	const Color &medium = *GameData::Colors().Get("medium");
	const Color &bright = *GameData::Colors().Get("bright");
	
	Table table;
	table.AddColumn(-115, {230, Alignment::LEFT});
	table.AddColumn(115, {230, Alignment::RIGHT});
	table.SetUnderline(-120, 120);
	
	int firstY = -248;
	table.DrawAt(Point(-130, firstY));
	
	static const string SETTINGS[] = {
		G("Display"),
		ZOOM_FACTOR,
		VIEW_ZOOM_FACTOR,
		VSYNC_SETTING,
		G("Show status overlays"),
		G("Highlight player's flagship"),
		G("Rotate flagship in HUD"),
		G("Show planet labels"),
		G("Show mini-map"),
		"",
		G("AI"),
		G("Automatic aiming"),
		G("Automatic firing"),
		EXPEND_AMMO,
		FIGHTER_REPAIR,
		TURRET_TRACKING,
		"\n",
		G("Performance"),
		G("Show CPU / GPU load"),
		G("Render motion blur"),
		G("Reduce large graphics"),
		G("Draw background haze"),
		G("Draw starfield"),
		G("Show hyperspace flash"),
		SHIP_OUTLINES,
		"",
		G("Other"),
		G("Clickable radar display"),
		G("Hide unexplored map regions"),
		REACTIVATE_HELP,
		G("Interrupt fast-forward"),
		G("Rehire extra crew when lost"),
		SCROLL_SPEED,
		G("Show escort systems on map"),
		G("System map sends move orders"),
		G("Warning siren"),
		LANGUAGE,
		FULLNAME
	};
	bool isCategory = true;
	for(const string &setting : SETTINGS)
	{
		// Check if this is a category break or column break.
		if(setting.empty() || setting == "\n")
		{
			isCategory = true;
			if(!setting.empty())
				table.DrawAt(Point(130, firstY));
			continue;
		}
		
		if(isCategory)
		{
			isCategory = false;
			table.DrawGap(10);
			table.DrawUnderline(medium);
			table.Draw(T(setting), bright);
			table.Advance();
			table.DrawGap(5);
			continue;
		}
		
		// Record where this setting is displayed, so the user can click on it.
		prefZones.emplace_back(table.GetCenterPoint(), table.GetRowSize(), setting);
		
		// Get the "on / off" text for this setting. Setting "isOn"
		// draws the setting "bright" (i.e. the setting is active).
		bool isOn = Preferences::Has(setting);
		string text;
		if(setting == ZOOM_FACTOR)
		{
			isOn = Screen::UserZoom() == Screen::Zoom();
			text = to_string(Screen::UserZoom());
		}
		else if(setting == VIEW_ZOOM_FACTOR)
		{
			isOn = true;
			text = to_string(static_cast<int>(100. * Preferences::ViewZoom()));
		}
		else if(setting == VSYNC_SETTING)
		{
			text = Preferences::VSyncSetting();
			isOn = text != "off";
			text = T(text, "vsync");
		}
		else if(setting == EXPEND_AMMO)
			text = T(Preferences::AmmoUsage());
		else if(setting == TURRET_TRACKING)
		{
			isOn = true;
			text = Preferences::Has(FOCUS_PREFERENCE) ? T("focused") : T("opportunistic");
		}
		else if(setting == FIGHTER_REPAIR)
		{
			isOn = true;
			text = Preferences::Has(FIGHTER_REPAIR) ? T("parallel") : T("series");
		}
		else if(setting == SHIP_OUTLINES)
		{
			isOn = true;
			text = Preferences::Has(SHIP_OUTLINES) ? T("fancy") : T("fast");
		}
		else if(setting == REACTIVATE_HELP)
		{
			// Check how many help messages have been displayed.
			const map<string, vector<T_>> &help = GameData::HelpTemplates();
			int shown = 0;
			int total = 0;
			for(const auto &it : help)
			{
				// Don't count certain special help messages that are always
				// active for new players.
				bool special = false;
				const string SPECIAL_HELP[] = {"basics", "lost"};
				for(const string &str : SPECIAL_HELP)
					if(it.first.find(str) == 0)
						special = true;
				
				if(!special)
				{
					++total;
					shown += Preferences::Has("help: " + it.first);
				}
			}
			
			if(shown)
				text = to_string(shown) + " / " + to_string(total);
			else
			{
				isOn = true;
				text = T("done");
			}
		}
		else if(setting == SCROLL_SPEED)
		{
			isOn = true;
			text = to_string(Preferences::ScrollSpeed());
		}
		else if(setting == LANGUAGE)
		{
			isOn = true;
			text = Preferences::GetLanguageName();
		}
		else if(setting == FULLNAME)
		{
			isOn = true;
			text = FontUtilities::Escape(T(Preferences::GetFullnameFormat(), "preferences"));
		}
		else
			text = isOn ? T("on") : T("off");
		
		if(setting == hoverPreference)
			table.DrawHighlight(back);
		table.Draw(T(setting), isOn ? medium : dim);
		table.Draw(text, isOn ? bright : medium);
	}
}



void PreferencesPanel::DrawPlugins()
{
	const Color &back = *GameData::Colors().Get("faint");
	const Color &medium = *GameData::Colors().Get("medium");
	const Color &bright = *GameData::Colors().Get("bright");
	
	const int MAX_TEXT_WIDTH = 230;
	Table table;
	table.AddColumn(-115, {MAX_TEXT_WIDTH, Truncate::MIDDLE});
	table.SetUnderline(-120, 120);
	
	int firstY = -238;
	table.DrawAt(Point(-130, firstY));
	table.DrawUnderline(medium);
	table.Draw(T("Installed plugins:"), bright);
	table.DrawGap(5);
	
	const Font &font = FontSet::Get(14);
	for(const auto &plugin : GameData::PluginAboutText())
	{
		pluginZones.emplace_back(table.GetCenterPoint(), table.GetRowSize(), plugin.first);
		
		bool isSelected = (plugin.first == selectedPlugin);
		if(isSelected || plugin.first == hoverPlugin)
			table.DrawHighlight(back);
		table.Draw(plugin.first, isSelected ? bright : medium);
		
		if(isSelected)
		{
			const Sprite *sprite = SpriteSet::Get(plugin.first);
			Point top(15., firstY);
			if(sprite)
			{
				Point center(130., top.Y() + .5 * sprite->Height());
				SpriteShader::Draw(sprite, center);
				top.Y() += sprite->Height() + 10.;
			}
			
			static const T_ EMPTY = T_("(No description given.)");
			const string &text = plugin.second.Str().empty() ? EMPTY.Str() : plugin.second.Str();
			font.Draw({text, {MAX_TEXT_WIDTH, Alignment::JUSTIFIED}}, top, medium);
		}
	}
}



void PreferencesPanel::Exit()
{
	Command::SaveSettings(Files::Config() + "keys.txt");
	
	GetUI()->Pop(this);
}
