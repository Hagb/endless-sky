/* FontSet.h
Copyright (c) 2014-2020 by Michael Zahniser

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#ifndef ES_TEXT_FONT_SET_H_
#define ES_TEXT_FONT_SET_H_

#include "Font.h"

#include <string>



// Class for getting the Font object for a given point size.
class FontSet {
public:
	static void Add(const std::string &fontsDir);
	static const Font &Get(int size);
	
	// Set the drawing settins.
	static void SetDrawingSettings(const Font::DrawingSettings &setting);
};



#endif
