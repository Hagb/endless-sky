/* FontSet.cpp
Copyright (c) 2014-2020 by Michael Zahniser

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#include "FontSet.h"

#include "../Files.h"

#include <cstdlib>
#include <map>
#include <fontconfig/fontconfig.h>

using namespace std;

namespace {
	map<int, Font> fonts;
	
	Font::DrawingSettings drawingSettings;
	char envBackend[] = "PANGOCAIRO_BACKEND=fc";
}



void FontSet::Add(const string &fontsDir)
{
	const FcChar8* fontsDirFc8 = reinterpret_cast<const FcChar8*>(fontsDir.c_str());
	const string cfgFile = fontsDir + "fonts.conf";
	const FcChar8* cfgFileFc8 = reinterpret_cast<const FcChar8*>(cfgFile.c_str());
	FcConfig *fcConfig = FcConfigGetCurrent();
	if(!FcConfigAppFontAddDir(fcConfig, fontsDirFc8))
		// This is not a fatal error.
		Files::LogError("Warning: Fail to load some fonts from \"" + fontsDir + "\".");
	// This function outputs some errors and warnings to stderr when detecting some errors in parsing,
	// but the function doesn't return any error codes and the players may ignore them.
	FcConfigParseAndLoad(fcConfig, cfgFileFc8, FcFalse);
}



const Font &FontSet::Get(int size)
{
	if(!fonts.count(size))
	{
		putenv(envBackend);
		Font &font = fonts[size];
		font.SetDrawingSettings(drawingSettings);
		font.SetPixelSize(size);
	}
	return fonts[size];
}



void FontSet::SetDrawingSettings(const Font::DrawingSettings &settings)
{
	drawingSettings = settings;
	for(auto &it : fonts)
		it.second.SetDrawingSettings(drawingSettings);
}
