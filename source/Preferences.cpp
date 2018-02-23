/* Preferences.cpp
Copyright (c) 2014 by Michael Zahniser

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#include "Preferences.h"

#include "Audio.h"
#include "DataFile.h"
#include "DataNode.h"
#include "DataWriter.h"
#include "Files.h"
#include "GameWindow.h"
#include "text/Gettext.h"
#include "Languages.h"
#include "Screen.h"

#include <algorithm>
#include <map>
#include <utility>

using namespace std;
using namespace Gettext;

namespace {
	map<string, bool> settings;
	int scrollSpeed = 60;
	
	// Strings for ammo expenditure:
	const string EXPEND_AMMO = "Escorts expend ammo";
	const string FRUGAL_ESCORTS = "Escorts use ammo frugally";
	
	const vector<double> ZOOMS = {.25, .35, .50, .70, 1.00, 1.40, 2.00};
	int zoomIndex = 4;
	constexpr double VOLUME_SCALE = .25;
	
	// Enable standard VSync by default.
	const vector<string> VSYNC_SETTINGS = {G("off", "vsync"), G("on", "vsync"), G("adaptive", "vsync")};
	int vsyncIndex = 1;
}



void Preferences::Load()
{
	// These settings should be on by default. There is no need to specify
	// values for settings that are off by default.
	settings["Automatic aiming"] = true;
	settings["Render motion blur"] = true;
	settings[FRUGAL_ESCORTS] = true;
	settings[EXPEND_AMMO] = true;
	settings["Damaged fighters retreat"] = true;
	settings["Warning siren"] = true;
	settings["Show escort systems on map"] = true;
	settings["Show mini-map"] = true;
	settings["Show planet labels"] = true;
	settings["Show hyperspace flash"] = true;
	settings["Draw background haze"] = true;
	settings["Draw starfield"] = true;
	settings["Hide unexplored map regions"] = true;
	settings["Turrets focus fire"] = true;
	settings["Ship outlines in shops"] = true;
	settings["Interrupt fast-forward"] = true;
	
	DataFile prefs(Files::Config() + "preferences.txt");
	for(const DataNode &node : prefs)
	{
		if(node.Token(0) == "window size" && node.Size() >= 3)
			Screen::SetRaw(node.Value(1), node.Value(2));
		else if(node.Token(0) == "zoom" && node.Size() >= 2)
			Screen::SetZoom(node.Value(1));
		else if(node.Token(0) == "volume" && node.Size() >= 2)
			Audio::SetVolume(node.Value(1) * VOLUME_SCALE);
		else if(node.Token(0) == "scroll speed" && node.Size() >= 2)
			scrollSpeed = node.Value(1);
		else if(node.Token(0) == "view zoom")
			zoomIndex = max<int>(0, min<int>(node.Value(1), ZOOMS.size() - 1));
		else if(node.Token(0) == "vsync")
			vsyncIndex = max<int>(0, min<int>(node.Value(1), VSYNC_SETTINGS.size() - 1));
		else if(node.Token(0) == "language" && node.Size() >= 2)
			Languages::SetLanguageID(node.Token(1));
		else if(node.Token(0) == "fullname format" && node.Size() >= 2)
			Languages::SetFullnameFormat(node.Token(1));
		else
			settings[node.Token(0)] = (node.Size() == 1 || node.Value(1));
	}
}



void Preferences::Save()
{
	DataWriter out(Files::Config() + "preferences.txt");
	
	out.Write("volume", Audio::Volume() / VOLUME_SCALE);
	out.Write("window size", Screen::RawWidth(), Screen::RawHeight());
	out.Write("zoom", Screen::UserZoom());
	out.Write("scroll speed", scrollSpeed);
	out.Write("view zoom", zoomIndex);
	out.Write("vsync", vsyncIndex);
	out.Write("language", Languages::GetLanguageID());
	out.Write("fullname format", Languages::GetFullnameFormat());
	
	for(const auto &it : settings)
		out.Write(it.first, it.second);
}



bool Preferences::Has(const string &name)
{
	auto it = settings.find(name);
	return (it != settings.end() && it->second);
}



void Preferences::Set(const string &name, bool on)
{
	settings[name] = on;
}



void Preferences::ToggleAmmoUsage()
{
	bool expend = Has(EXPEND_AMMO);
	bool frugal = Has(FRUGAL_ESCORTS);
	Preferences::Set(EXPEND_AMMO, !(expend && !frugal));
	Preferences::Set(FRUGAL_ESCORTS, !expend);
}



string Preferences::AmmoUsage()
{
	return Has(EXPEND_AMMO) ? Has(FRUGAL_ESCORTS) ? G("frugally") : G("always") : G("never");
}



// Scroll speed preference.
int Preferences::ScrollSpeed()
{
	return scrollSpeed;
}



void Preferences::SetScrollSpeed(int speed)
{
	scrollSpeed = speed;
}



// View zoom.
double Preferences::ViewZoom()
{
	return ZOOMS[zoomIndex];
}



bool Preferences::ZoomViewIn()
{
	if(zoomIndex == static_cast<int>(ZOOMS.size() - 1))
		return false;
	
	++zoomIndex;
	return true;
}



bool Preferences::ZoomViewOut()
{
	if(zoomIndex == 0)
		return false;
	
	--zoomIndex;
	return true;
}



bool Preferences::ToggleVSync()
{
	int targetIndex = vsyncIndex + 1;
	if(targetIndex == static_cast<int>(VSYNC_SETTINGS.size()))
		targetIndex = 0;
	if(!GameWindow::SetVSync(static_cast<VSync>(targetIndex)))
	{
		// Not all drivers support adaptive VSync. Increment desired VSync again.
		++targetIndex;
		if(targetIndex == static_cast<int>(VSYNC_SETTINGS.size()))
			targetIndex = 0;
		if(!GameWindow::SetVSync(static_cast<VSync>(targetIndex)))
		{
			// Restore original saved setting.
			Files::LogError("Unable to change VSync state");
			GameWindow::SetVSync(static_cast<VSync>(vsyncIndex));
			return false;
		}
	}
	vsyncIndex = targetIndex;
	return true;
}



// Return the current VSync setting
Preferences::VSync Preferences::VSyncState()
{
	return static_cast<VSync>(vsyncIndex);
}



const string &Preferences::VSyncSetting()
{
	return VSYNC_SETTINGS[vsyncIndex];
}



void Preferences::ToggleLanguage()
{
	const string &langID = Languages::GetLanguageID();
	const auto &knownLangIDs = Languages::GetKnownLanguageIDs();
	auto it = find(knownLangIDs.begin(), knownLangIDs.end(), langID);
	if(it == knownLangIDs.end())
		Languages::SetLanguageID(*knownLangIDs.begin());
	else
	{
		++it;
		if(it == knownLangIDs.end())
			Languages::SetLanguageID(*knownLangIDs.begin());
		else
			Languages::SetLanguageID(*it);
	}
}



string Preferences::GetLanguageName()
{
	return Languages::GetLanguageName();
}



void Preferences::ToggleFullnameFormat()
{
	const string &fmt = Languages::GetFullnameFormat();
	const auto &knownFmts = Languages::GetKnownFullnameFormats();
	auto it = find(knownFmts.begin(), knownFmts.end(), fmt);
	if(it == knownFmts.end())
		Languages::SetFullnameFormat(*knownFmts.begin());
	else
	{
		++it;
		if(it == knownFmts.end())
			Languages::SetFullnameFormat(*knownFmts.begin());
		else
			Languages::SetFullnameFormat(*it);
	}
}



string Preferences::GetFullnameFormat()
{
	return Languages::GetFullnameFormat();
}
