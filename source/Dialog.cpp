/* Dialog.cpp
Copyright (c) 2014-2020 by Michael Zahniser

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#include "Dialog.h"

#include "text/alignment.hpp"
#include "Color.h"
#include "Command.h"
#include "Conversation.h"
#include "DataNode.h"
#include "text/DisplayText.h"
#include "FillShader.h"
#include "text/Font.h"
#include "text/FontSet.h"
#include "text/FontUtilities.h"
#include "GameData.h"
#include "MapDetailPanel.h"
#include "PlayerInfo.h"
#include "Point.h"
#include "Screen.h"
#include "Sprite.h"
#include "SpriteSet.h"
#include "SpriteShader.h"
#include "TextInputPanel.h"
#include "UI.h"

#include <cmath>

using namespace std;
using namespace Gettext;

namespace {
	const int WIDTH = 250;
	
	
	// Convert a raw input text to the valid text.
	string ValidateTextInput(const string &text)
	{
		return text;
	}
	
	string ValidateIntInput(const string &text)
	{
		string out;
		out.reserve(text.length());
		for(auto c : text)
		{
			// Integer input should not allow leading zeros.
			if(c == '0' && !out.empty())
				out += c;
			else if(c >= '1' && c <= '9')
				out += c;
		}
		return out;
	}
	
	
	// Return true if the event have to handle this dialog.
	bool IsFallThroughEvent(SDL_Keycode key, Uint16 mod, const Command &, bool)
	{
		return key == SDLK_ESCAPE
			|| (key == 'w' && (mod & (KMOD_CTRL | KMOD_GUI)))
			|| key == SDLK_TAB
			|| key == SDLK_LEFT
			|| key == SDLK_RIGHT
			|| key == SDLK_RETURN
			|| key == SDLK_KP_ENTER;
	}
}



// Dialog that has no callback (information only). In this form, there is
// only an "ok" button, not a "cancel" button.
Dialog::Dialog(const string &text, Truncate truncate)
{
	Init(text, truncate, "", false);
}



// Mission accept / decline dialog.
Dialog::Dialog(const string &text, PlayerInfo &player, const System *system, Truncate truncate)
	: intFun(bind(&PlayerInfo::MissionCallback, &player, placeholders::_1)),
	system(system), player(&player)
{
	Init(text, truncate, "", true, true);
}



Dialog::~Dialog()
{
	if(textInputPanel)
		GetUI()->Pop(textInputPanel.get());
}



// Draw this panel.
void Dialog::Draw()
{
	DrawBackdrop();
	
	const Sprite *top = SpriteSet::Get("ui/dialog top");
	const Sprite *middle = SpriteSet::Get("ui/dialog middle");
	const Sprite *bottom = SpriteSet::Get("ui/dialog bottom");
	const Sprite *cancel = SpriteSet::Get("ui/dialog cancel");
	
	// Get the position of the top of this dialog, and of the text and input.
	Point pos(0., topPosY);
	Point textPos(WIDTH * -.5 + 10., pos.Y() + 20.);
	
	// Draw the top section of the dialog box.
	pos.Y() += top->Height() * .5;
	SpriteShader::Draw(top, pos);
	pos.Y() += top->Height() * .5;
	
	// The middle section is duplicated depending on how long the text is.
	for(int i = 0; i < height; ++i)
	{
		pos.Y() += middle->Height() * .5;
		SpriteShader::Draw(middle, pos);
		pos.Y() += middle->Height() * .5;
	}
	
	// Draw the bottom section.
	pos.Y() += bottom->Height() * .5;
	SpriteShader::Draw(bottom, pos);
	pos.Y() += bottom->Height() * .5 - 25.;
	
	// Draw the buttons, including optionally the cancel button.
	const Font &font = FontSet::Get(14);
	const Color &bright = *GameData::Colors().Get("bright");
	const Color &dim = *GameData::Colors().Get("medium");
	if(canCancel)
	{
		string cancelText = isMission ? T("Decline") : T("Cancel");
		cancelPos = pos + Point(10., 0.);
		SpriteShader::Draw(cancel, cancelPos);
		Point labelPos(
			cancelPos.X() - .5 * font.Width(cancelText),
			cancelPos.Y() - .5 * font.Height());
		font.Draw(cancelText, labelPos, !okIsActive ? bright : dim);
	}
	string okText = isMission ? T("Accept") : T("OK");
	okPos = pos + Point(90., 0.);
	Point labelPos(
		okPos.X() - .5 * font.Width(okText),
		okPos.Y() - .5 * font.Height());
	font.Draw(okText, labelPos, okIsActive ? bright : dim);
	
	// Draw the text.
	font.Draw(dialogText, textPos, dim);
	
	// Draw the input, if any.
	if(!isMission && (intFun || stringFun))
	{
		Point inputPos = Point(0., -70. - topPosY);
		const Color &back = *GameData::Colors().Get("faint");
		FillShader::Fill(inputPos, Point(WIDTH - 20., 20.), back);
	}
}



// Format and add the text from the given node to the given string.
void Dialog::ParseTextNode(const DataNode &node, size_t startingIndex, std::vector<Gettext::T_> &text)
{
	for(int i = startingIndex; i < node.Size(); ++i)
	{
		if(!IsEmptyText(text))
			text.push_back(T_("\n\t", "dialog paragraph separator"));
		text.emplace_back(node.Token(i));
	}
	for(const DataNode &child : node)
		for(int i = 0; i < child.Size(); ++i)
		{
			if(!IsEmptyText(text))
				text.push_back(T_("\n\t", "dialog paragraph separator"));
			text.emplace_back(child.Token(i));
		}
}



bool Dialog::KeyDown(SDL_Keycode key, Uint16 mod, const Command &command, bool isNewPress)
{
	bool isCloseRequest = key == SDLK_ESCAPE || (key == 'w' && (mod & (KMOD_CTRL | KMOD_GUI)));
	if(key == SDLK_TAB && canCancel)
		okIsActive = !okIsActive;
	else if(key == SDLK_LEFT)
		okIsActive = !canCancel;
	else if(key == SDLK_RIGHT)
		okIsActive = true;
	else if(key == SDLK_RETURN || key == SDLK_KP_ENTER || isCloseRequest
			|| (isMission && (key == 'a' || key == 'd')))
	{
		// Shortcuts for "accept" and "decline."
		if(key == 'a' || (!canCancel && isCloseRequest))
			okIsActive = true;
		if(key == 'd' || (canCancel && isCloseRequest))
			okIsActive = false;
		if(okIsActive || isMission)
			DoCallback();
		
		if(textInputPanel)
		{
			GetUI()->Pop(textInputPanel.get());
			textInputPanel.reset();
		}
		GetUI()->Pop(this);
	}
	else if((key == 'm' || command.Has(Command::MAP)) && system && player)
		GetUI()->Push(new MapDetailPanel(*player, system));
	else
		return false;
	
	return true;
}



bool Dialog::Click(int x, int y, int clicks)
{
	Point clickPos(x, y);
	
	Point ok = clickPos - okPos;
	if(fabs(ok.X()) < 40. && fabs(ok.Y()) < 20.)
	{
		okIsActive = true;
		return DoKey(SDLK_RETURN);
	}
	
	if(canCancel)
	{
		Point cancel = clickPos - cancelPos;
		if(fabs(cancel.X()) < 40. && fabs(cancel.Y()) < 20.)
		{
			okIsActive = false;
			return DoKey(SDLK_RETURN);
		}
	}
	
	return true;
}



void Dialog::Focus(bool thisPanel)
{
	// Focus(true) is never called if textInputPanel is valid and pushed.
	if(thisPanel && textInputPanel)
		GetUI()->Push(textInputPanel);
}



// Dim the background of this panel.
void Dialog::DrawBackdrop() const
{
	if(!GetUI()->IsTop(this) && !(textInputPanel && GetUI()->IsTop(textInputPanel.get())))
		return;
	
	// Darken everything but the dialog.
	const Color &back = *GameData::Colors().Get("dialog backdrop");
	FillShader::Fill(Point(), Point(Screen::Width(), Screen::Height()), back);
}



// Common code from all three constructors:
void Dialog::Init(const string &message, Truncate truncate, const string &initialText,
	bool canCancel, bool isMission)
{
	this->isMission = isMission;
	this->canCancel = canCancel;
	okIsActive = true;
	
	dialogText = {message, {WIDTH - 20, Alignment::JUSTIFIED, truncate}};
	
	// The dialog with no extenders is 80 pixels tall. 10 pixels at the top and
	// bottom are "padding," but text.Height() over-reports the height by about
	// 5 pixels because it includes its own padding at the bottom. If there is a
	// text input, we need another 20 pixels for it and 10 pixels padding.
	const Font &font = FontSet::Get(14);
	height = 10 + (font.FormattedHeight(dialogText) - 5) + 10 + 30 * (!isMission && (intFun || stringFun));
	// Determine how many 40-pixel extension panels we need.
	if(height <= 80)
		height = 0;
	else
		height = (height - 40) / 40;
	
	// Get the position of the top of this dialog.
	const Sprite *top = SpriteSet::Get("ui/dialog top");
	const Sprite *middle = SpriteSet::Get("ui/dialog middle");
	const Sprite *bottom = SpriteSet::Get("ui/dialog bottom");
	topPosY = -.5f * (top->Height() + height * middle->Height() + bottom->Height());
	
	// This dialog has an input field.
	if(!isMission && (intFun || stringFun))
	{
		Point inputPos = Point(-.5 * (WIDTH - 20) + 5., -70. - topPosY - .5 * font.Height());
		const auto layout = Layout(WIDTH - 30, Truncate::FRONT);
		Color textColor = *GameData::Colors().Get("bright");
		Color cursorColor = *GameData::Colors().Get("medium");
		auto validateFunc = intFun ? ValidateIntInput : ValidateTextInput;
		textInputPanel = shared_ptr<TextInputPanel>(new TextInputPanel(font, inputPos, layout,
			textColor, cursorColor, validateFunc, IsFallThroughEvent, initialText));
		// This dialog is not set to any UI and GetUI() returns nullptr at this point,
		// so textInputPanel will be pushed to UI when Focus(true) is called.
	}
}



void Dialog::DoCallback() const
{
	if(isMission)
	{
		if(intFun)
			intFun(okIsActive ? Conversation::ACCEPT : Conversation::DECLINE);
		
		return;
	}
	
	if(intFun)
	{
		// Only call the callback if the input can be converted to an int.
		// Otherwise treat this as if the player clicked "cancel."
		try {
			intFun(stoi(textInputPanel->GetText()));
		}
		catch(...)
		{
		}
	}
	
	if(stringFun)
		stringFun(textInputPanel->GetText());
	
	if(voidFun)
		voidFun();
}
