/* Dialog.h
Copyright (c) 2014 by Michael Zahniser

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#ifndef DIALOG_H_
#define DIALOG_H_

#include "Panel.h"

#include "Font.h"
#include "Point.h"
#include "WrappedText.h"

#include <functional>
#include <string>

class DataNode;
class PlayerInfo;
class System;



// A dialog box displays a given message to the player. The box will expand to
// fit the message, and may also include a text input field. The box may have
// only an "ok" button, or may also have a "cancel" button. If this dialog is
// introducing a mission, the buttons are instead "accept" and "decline". A
// callback function can be given to receive the player's response.
class Dialog : public Panel {
public:
	// Dialog that has no callback (information only). In this form, there is
	// only an "ok" button, not a "cancel" button.
	explicit Dialog(const std::string &text, Font::Truncate trunc = Font::TRUNC_NONE);
	// Mission accept / decline dialog.
	Dialog(const std::string &text, PlayerInfo &player, const System *system = nullptr);
	virtual ~Dialog() = default;
	
	// Three different kinds of dialogs can be constructed: requesting numerical
	// input, requesting text input, or not requesting any input at all. In any
	// case, the callback is called only if the user selects "ok", not "cancel."
template <class T>
	Dialog(T *t, void (T::*fun)(int), const std::string &text, Font::Truncate trunc = Font::TRUNC_NONE);
template <class T>
	Dialog(T *t, void (T::*fun)(int), const std::string &text, int initialValue, Font::Truncate trunc = Font::TRUNC_NONE);
	
template <class T>
	Dialog(T *t, void (T::*fun)(const std::string &), const std::string &text, std::string initialValue = "", Font::Truncate trunc = Font::TRUNC_NONE);
	
template <class T>
	Dialog(T *t, void (T::*fun)(), const std::string &text, Font::Truncate trunc = Font::TRUNC_NONE);
	
	// Draw this panel.
	virtual void Draw() override;
	
	// Static method used to convert a DataNode into formatted Dialog text.
	static void ParseTextNode(const DataNode &node, size_t startingIndex, std::string &text);
	
	
protected:
	// The use can click "ok" or "cancel", or use the tab key to toggle which
	// button is highlighted and the enter key to select it.
	virtual bool KeyDown(SDL_Keycode key, Uint16 mod, const Command &command, bool isNewPress) override;
	virtual bool Click(int x, int y, int clicks) override;
	
	
private:
	// Common code from all three constructors:
	void Init(const std::string &message, bool canCancel = true, bool isMission = false, Font::Truncate trunc = Font::TRUNC_NONE);
	void DoCallback() const;
	
	
protected:
	WrappedText text;
	int height;
	
	std::function<void(int)> intFun;
	std::function<void(const std::string &)> stringFun;
	std::function<void()> voidFun;
	
	bool canCancel;
	bool okIsActive;
	bool isMission;
	
	std::string input;
	
	Point okPos;
	Point cancelPos;
	
	const System *system = nullptr;
	PlayerInfo *player = nullptr;
};



template <class T>
Dialog::Dialog(T *t, void (T::*fun)(int), const std::string &text, Font::Truncate trunc)
	: intFun(std::bind(fun, t, std::placeholders::_1))
{
	Init(text, true, false, trunc);
}



template <class T>
Dialog::Dialog(T *t, void (T::*fun)(int), const std::string &text, int initialValue, Font::Truncate trunc)
	: intFun(std::bind(fun, t, std::placeholders::_1)), input(std::to_string(initialValue))
{
	Init(text, true, false, trunc);
}



template <class T>
Dialog::Dialog(T *t, void (T::*fun)(const std::string &), const std::string &text, std::string initialValue, Font::Truncate trunc)
	: stringFun(std::bind(fun, t, std::placeholders::_1)), input(initialValue)
{
	Init(text, true, false, trunc);
}



template <class T>
Dialog::Dialog(T *t, void (T::*fun)(), const std::string &text, Font::Truncate trunc)
	: voidFun(std::bind(fun, t))
{
	Init(text, true, false, trunc);
}



#endif
