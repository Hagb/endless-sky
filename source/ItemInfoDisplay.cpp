/* ItemInfoDisplay.cpp
Copyright (c) 2014 by Michael Zahniser

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#include "ItemInfoDisplay.h"

#include "text/alignment.hpp"
#include "Color.h"
#include "FillShader.h"
#include "text/FontSet.h"
#include "text/Format.h"
#include "GameData.h"
#include "text/layout.hpp"
#include "Rectangle.h"
#include "text/Gettext.h"
#include "Screen.h"
#include "text/Table.h"

#include <algorithm>
#include <cmath>
#include <functional>

using namespace std;
using namespace Gettext;

namespace {
	const int HOVER_TIME = 60;
	
	const T_ NOUN[2] = { T_("outfit", "ItemInfoDisplay NOUN"), T_("ship", "ItemInfoDisplay NOUN") };
	// TRANSLATORS: This "vowel" determines whether 'a' or 'an' is used.
	const T_ vowel = T_("aeiou", "ItemInfoDisplay");
	// TRANSLATORS: Indefinite article of a license.
	const T_ indefiniteArticle[2] = { T_("a", "ItemInfoDisplay"), T_("an", "ItemInfoDisplay") };
	// TRANSLATORS: %1%: indefinite article, %2%: license name.
	const T_ licenseFormat = T_("%1% %2%", "ItemInfoDisplay License");
	Format::ListOfWords listOfLicenses;

	// The Hook of translation.
	function<void()> updateCatalog([](){
		// TRANSLATORS: The Separators of licenses.
		listOfLicenses.SetSeparators(T(": and :, :, and ", "ItemInfoDisplay"));
	});
	// Set the hook.
	volatile bool hooked = AddHookUpdating(&updateCatalog);
	
	
	
	// Get the license name.
	string GetLicenseName(vector<string>::const_iterator it)
	{
		bool isVowel = false;
		for(const char &c : vowel.Str())
			if(*it->begin() == c || *it->begin() == toupper(c))
			{
				isVowel = true;
				break;
			}
		const string postfix(" License");
		return Format::StringF(licenseFormat.Str(), indefiniteArticle[isVowel].Str(),
			T((*it) + postfix, "license: "));
	}
}

const Layout ItemInfoDisplay::commonLayout = Layout(WIDTH - 20, Alignment::JUSTIFIED);



// Get the panel width.
int ItemInfoDisplay::PanelWidth()
{
	return WIDTH;
}



// Get the height of each of the three panels.
int ItemInfoDisplay::MaximumHeight() const
{
	return maximumHeight;
}



int ItemInfoDisplay::DescriptionHeight() const
{
	return descriptionHeight;
}



int ItemInfoDisplay::AttributesHeight() const
{
	return attributesHeight;
}



// Draw each of the panels.
void ItemInfoDisplay::DrawDescription(const Point &topLeft) const
{
	Rectangle hoverTarget = Rectangle::FromCorner(topLeft, Point(PanelWidth(), DescriptionHeight()));
	Color color = hoverTarget.Contains(hoverPoint) ? *GameData::Colors().Get("bright") : *GameData::Colors().Get("medium");
	const Font &font = FontSet::Get(14);
	font.Draw(description, topLeft + Point(10., 12.), color);
}



void ItemInfoDisplay::DrawAttributes(const Point &topLeft) const
{
	Draw(topLeft, attributeLabels, attributeValues);
}



void ItemInfoDisplay::DrawTooltips() const
{
	if(!hoverCount || hoverCount-- < HOVER_TIME || hoverText.GetText().empty())
		return;
	
	const Font &font = FontSet::Get(14);
	int hoverParagraphBreak = font.ParagraphBreak(commonLayout);
	Point boxSize = font.FormattedBounds(hoverText) + Point(20., 20. - hoverParagraphBreak);
	
	Point topLeft = hoverPoint;
	if(topLeft.X() + boxSize.X() > Screen::Right())
		topLeft.X() -= boxSize.X();
	if(topLeft.Y() + boxSize.Y() > Screen::Bottom())
		topLeft.Y() -= boxSize.Y();
	
	FillShader::Fill(topLeft + .5 * boxSize, boxSize, *GameData::Colors().Get("tooltip background"));
	font.Draw(hoverText, topLeft + Point(10., 10.), *GameData::Colors().Get("medium"));
}



// Update the location where the mouse is hovering.
void ItemInfoDisplay::Hover(const Point &point)
{
	hoverPoint = point;
	hasHover = true;
}



void ItemInfoDisplay::ClearHover()
{
	hasHover = false;
}



void ItemInfoDisplay::UpdateDescription(const string &text, const vector<string> &licenses, bool isShip)
{
	if(licenses.empty())
		description.SetText(text);
	else
	{
		string fullText = text;
		vector<string>::const_iterator it = licenses.begin();
		fullText += Format::StringF(T("\tTo purchase this %1% you must have %2%.\n"),
			NOUN[isShip].Str(),
			listOfLicenses.GetList(licenses.size(), [&it](){ return GetLicenseName(it++); }));
		description.SetText(fullText);
	}
	
	// Pad by 10 pixels on the top and bottom.
	const Font &font = FontSet::Get(14);
	descriptionHeight = font.FormattedHeight(description) + 20;
}



Point ItemInfoDisplay::Draw(Point point, const vector<string> &labels, const vector<string> &values) const
{
	// Add ten pixels of padding at the top.
	point.Y() += 10.;
	
	// Get standard colors to draw with.
	const Color &labelColor = *GameData::Colors().Get("medium");
	const Color &valueColor = *GameData::Colors().Get("bright");
	
	Table table;
	// Use 10-pixel margins on both sides.
	table.AddColumn(10, {WIDTH - 20});
	table.AddColumn(WIDTH - 10, {WIDTH - 20, Alignment::RIGHT});
	table.SetHighlight(0, WIDTH);
	table.DrawAt(point);
	
	for(unsigned i = 0; i < labels.size() && i < values.size(); ++i)
	{
		if(labels[i].empty())
		{
			table.DrawGap(10);
			continue;
		}
		
		CheckHover(table, labels[i]);
		table.Draw(T(labels[i], "Label of Attribute"), values[i].empty() ? valueColor : labelColor);
		table.Draw(T(values[i]), valueColor);
	}
	return table.GetPoint();
}



void ItemInfoDisplay::CheckHover(const Table &table, const string &label) const
{
	if(!hasHover)
		return;
	
	Point distance = hoverPoint - table.GetCenterPoint();
	Point radius = .5 * table.GetRowSize();
	if(abs(distance.X()) < radius.X() && abs(distance.Y()) < radius.Y())
	{
		hoverCount += 2 * (label == hover);
		hover = label;
		if(hoverCount >= HOVER_TIME)
		{
			hoverCount = HOVER_TIME;
			hoverText.SetText(GameData::Tooltip(label));
		}
	}
}
