/* TextInputPanel.cpp
Copyright (c) 2021 by OOTA, Masato

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#include "TextInputPanel.h"

#include "FillShader.h"
#include "text/Font.h"
#include "text/FontUtilities.h"
#include "Screen.h"
#include "shift.h"
#include "text/Utf8.h"

#include <cctype>
#include <map>

using namespace std;

namespace {
	const int IME_RECT_Y_MARGIN = 0;
	
	// Map any conceivable numeric keypad keys to their ASCII values. Most of
	// these will presumably only exist on special programming keyboards.
	const map<SDL_Keycode, char> KEY_MAP = {
		{SDLK_KP_0, '0'},
		{SDLK_KP_1, '1'},
		{SDLK_KP_2, '2'},
		{SDLK_KP_3, '3'},
		{SDLK_KP_4, '4'},
		{SDLK_KP_5, '5'},
		{SDLK_KP_6, '6'},
		{SDLK_KP_7, '7'},
		{SDLK_KP_8, '8'},
		{SDLK_KP_9, '9'},
		{SDLK_KP_AMPERSAND, '&'},
		{SDLK_KP_AT, '@'},
		{SDLK_KP_A, 'a'},
		{SDLK_KP_B, 'b'},
		{SDLK_KP_C, 'c'},
		{SDLK_KP_D, 'd'},
		{SDLK_KP_E, 'e'},
		{SDLK_KP_F, 'f'},
		{SDLK_KP_COLON, ':'},
		{SDLK_KP_COMMA, ','},
		{SDLK_KP_DIVIDE, '/'},
		{SDLK_KP_EQUALS, '='},
		{SDLK_KP_EXCLAM, '!'},
		{SDLK_KP_GREATER, '>'},
		{SDLK_KP_HASH, '#'},
		{SDLK_KP_LEFTBRACE, '{'},
		{SDLK_KP_LEFTPAREN, '('},
		{SDLK_KP_LESS, '<'},
		{SDLK_KP_MINUS, '-'},
		{SDLK_KP_MULTIPLY, '*'},
		{SDLK_KP_PERCENT, '%'},
		{SDLK_KP_PERIOD, '.'},
		{SDLK_KP_PLUS, '+'},
		{SDLK_KP_POWER, '^'},
		{SDLK_KP_RIGHTBRACE, '}'},
		{SDLK_KP_RIGHTPAREN, ')'},
		{SDLK_KP_SPACE, ' '},
		{SDLK_KP_VERTICALBAR, '|'}
	};
}



TextInputPanel::TextInputPanel(const Font &font, Point point, const Layout &layout,
	const Color &textColor, const Color &cursorColor,
	string (*validateFunc)(const string &),
	bool (*isFallThroughFunc)(SDL_Keycode key, Uint16 mod, const Command &command, bool isNewPress),
	const string &initialText)
	: font(font), point(point), layout(layout), textColor(textColor), cursorColor(cursorColor),
	validateFunc(validateFunc), isFallThroughFunc(isFallThroughFunc), inputText(initialText)
{
	SetTrapAllEvents(false);
}



const string &TextInputPanel::GetText() const
{
	return inputText;
}



void TextInputPanel::SetText(const string &newText)
{
	inputText = newText;
	editText.clear();
}



void TextInputPanel::SetPoint(Point newPoint)
{
	point = newPoint;
}



void TextInputPanel::Draw()
{
	Point textPoint = point;
	string text = FontUtilities::Escape(inputText);
	auto displayText = DisplayText(text, layout);
	// Draw the committed text.
	if(!inputText.empty())
		textPoint.X() += font.FormattedWidth(displayText);
	SetIMERect(textPoint);
	
	// Draw the editing text.
	if(!editText.empty())
	{
		text += "<span underline='single'>";
		text += FontUtilities::Escape(editText);
		text += "</span>";
	}
	displayText.SetText(text);
	font.Draw(displayText, point, textColor);
	textPoint.X() = point.X() + font.FormattedWidth(displayText);
	
	// Draw the cursor.
	textPoint.X() += 2.;
	const int height = font.Height();
	const Point barPos(textPoint.X(), textPoint.Y() + height / 2.);
	FillShader::Fill(barPos, Point(1., font.Height()), cursorColor);
}



bool TextInputPanel::KeyDown(SDL_Keycode key, Uint16 mod, const Command &command, bool isNewPress)
{
	if(!editText.empty())
	{
#if defined(HAVE_FCITX_FRONTEND_H) || defined(HAVE_IBUS_IBUS_H)
		if(key <= 0x7F)
			// ibus and fcitx raise no event EditingText when deleting a last character.
			// If key is a control code (0..0x1F and 0x7F) here, the input method is inactive.
			// key <= 0x7F catches only control codes, because ibus and fcitx
			// don't call KeyDown with printable characters when they has a composition.
			editText.clear();
#endif
		return true;
	}
	
	if(isFallThroughFunc(key, mod, command, isNewPress))
		return false;
	
	auto it = KEY_MAP.find(key);
	if(it != KEY_MAP.end())
	{
		int ascii = it->second;
		char c = ((mod & KMOD_SHIFT) ? SHIFT[ascii] : ascii);
		// Caps lock should shift letters, but not any other keys.
		if((mod & KMOD_CAPS) && c >= 'a' && c <= 'z')
			c += 'A' - 'a';
		AddText(string(1, c), true);
	}
	else if(key == SDLK_DELETE || key == SDLK_BACKSPACE)
	{
		if(!inputText.empty())
		{
			const size_t n = Utf8::CodePointStart(inputText, inputText.size() - 1);
			inputText.erase(inputText.begin() + n, inputText.end());
		}
	}
	else if(key == 'v' && (mod & KMOD_CTRL))
	{
		// Copy a text from the clipboard.
		// The text may be encoded by UTF-8 but accepts no control code.
		// Ignore any control codes only at the beginning of the text.
		char *clipboard = SDL_GetClipboardText();
		string clipboardInput;
		const char *ptr = clipboard;
		for( ; *ptr != '\0' && iscntrl(static_cast<unsigned char>(*ptr)); ++ptr)
			;
		for( ; *ptr != '\0'; ++ptr)
			if(iscntrl(static_cast<unsigned char>(*ptr)))
				break;
			else
				clipboardInput += *ptr;
		AddText(clipboardInput);
		SDL_free(clipboard);
	}
	else
		return false;
	
	return true;
}



bool TextInputPanel::TextEditing(const char *text, Sint32 start, Sint32 length)
{
	if(isFocused)
	{
#if defined(HAVE_FCITX_FRONTEND_H) || defined(HAVE_IBUS_IBUS_H)
		if(start != 0)
			editText += text;
		else
			editText = text;
#else
		editText = text;
#endif
		return true;
	}
	return false;
}



bool TextInputPanel::TextInput(const char *text)
{
	if(isFocused)
	{
		AddText(text);
		editText.clear();
		return true;
	}
	return false;
}



void TextInputPanel::Focus(bool thisPanel)
{
	if(thisPanel)
	{
		isFocused = true;
		SDL_StartTextInput();
		// This panel have to ignore one SDL_TEXTINPUT event that caused to call this function.
		SDL_PumpEvents();
		SDL_Event event;
		if(SDL_PeepEvents(&event, 1, SDL_PEEKEVENT, SDL_FIRSTEVENT, SDL_LASTEVENT) > 0 && event.type == SDL_TEXTINPUT)
			SDL_PollEvent(&event);
		
		SetIMERect(point);
	}
	else if(isFocused)
	{
		isFocused = false;
		editText.clear();
		SDL_StopTextInput();
	}
}



void TextInputPanel::AddText(const string &s, bool keydownEvent)
{
	// This logic removes a duplicate character because both KeyDown() and
	// TextInput() may try to add one of the KEY_MAP characters.
	const char *p = s.c_str();
	if(!inputText.empty() && keydownEvent != previousEventIsKeyDown && inputText.back() == p[0])
		++p;
	else
		previousEventIsKeyDown = keydownEvent;
		
	inputText = validateFunc(inputText + p);
}



// Set the coordinate used for IME window.
void TextInputPanel::SetIMERect(Point point)
{
	// SDL_SetTextInputRect() is called only if the point is changed.
	if(IMERectPoint - point)
	{
		SDL_Rect imeRect;
		imeRect.x = point.X() * Screen::Zoom() / 100 + Screen::RawWidth() * .5;
		imeRect.y = (point.Y() + font.Height() + IME_RECT_Y_MARGIN) * Screen::Zoom() / 100
			+ Screen::RawHeight() * .5;
		imeRect.w = 0;
		imeRect.h = 0;
		SDL_SetTextInputRect(&imeRect);
		
		IMERectPoint = point;
	}
}
