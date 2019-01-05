/* TextInputPanel.h
Copyright (c) 2021 by OOTA, Masato

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#ifndef TEXT_INPUT_PANEL_H_
#define TEXT_INPUT_PANEL_H_

#include "Panel.h"

#include "Color.h"
#include "text/layout.hpp"
#include "Point.h"

#include <string>

class Font;



// A text input panel handles the events to input a text by a player and
// displays the text.
// Expected use case: A panel that need to get a text from a player overlays this panel,
// or possesses this panel and transfers some events and drawing to it.
class TextInputPanel : public Panel {
public:
	// 'validateFunc' converts a raw input to the valid input text.
	// 'isFallThroughFunc' returns true if the event have to handle a lower panel.
	explicit TextInputPanel(const Font &font, Point point, const Layout &layout,
		const Color &textColor, const Color &cursorColor,
		std::string (*validateFunc)(const std::string &),
		bool (*isFallThroughFunc)(SDL_Keycode key, Uint16 mod, const Command &command, bool isNewPress),
		const std::string &initialText = "");
	
	// Get the inputted text.
	const std::string &GetText() const;
	// Overwrite the inputted text.
	void SetText(const std::string &newText);
	
	// Update the point where the inputted text is drawn.
	void SetPoint(Point newPoint);
	
	
	// Draw this panel.
	virtual void Draw() override;
	
	
protected:
	virtual bool KeyDown(SDL_Keycode key, Uint16 mod, const Command &command, bool isNewPress) override;
	virtual bool TextEditing(const char *text, Sint32 start, Sint32 length) override;
	virtual bool TextInput(const char *text) override;
	virtual void Focus(bool thisPanel) override;
	
	
private:
	void AddText(const std::string &s, bool keydownEvent = false);
	
	// Set the coordinate used for IME window.
	void SetIMERect(Point point);
	
	
private:
	const Font &font;
	Point point;
	Layout layout;
	Color textColor;
	Color cursorColor;
	
	// validateFunc is applied to the whole text instead of the additional text
	// when a player inputs a text.
	std::string (*validateFunc)(const std::string &);
	bool (*isFallThroughFunc)(SDL_Keycode key, Uint16 mod, const Command &command, bool isNewPress);
	
	std::string inputText;
	std::string editText;
	
	bool isFocused = false;
	bool previousEventIsKeyDown = false;
	Point IMERectPoint;
};



#endif
