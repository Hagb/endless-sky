/* ConversationPanel.cpp
Copyright (c) 2014 by Michael Zahniser

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#include "ConversationPanel.h"

#include "text/alignment.hpp"
#include "BoardingPanel.h"
#include "Color.h"
#include "Command.h"
#include "Conversation.h"
#include "FillShader.h"
#include "text/Font.h"
#include "text/FontSet.h"
#include "text/FontUtilities.h"
#include "text/Format.h"
#include "GameData.h"
#include "text/Gettext.h"
#include "Government.h"
#include "Languages.h"
#include "MapDetailPanel.h"
#include "PlayerInfo.h"
#include "Point.h"
#include "Preferences.h"
#include "Screen.h"
#include "shift.h"
#include "Ship.h"
#include "Sprite.h"
#include "SpriteSet.h"
#include "SpriteShader.h"
#include "TextInputPanel.h"
#include "UI.h"
#include "text/Utf8.h"

#if defined _WIN32
#include "Files.h"
#endif

#include <algorithm>
#include <cctype>
#include <iterator>

using namespace std;
using namespace Gettext;

namespace {
#if defined _WIN32
	size_t PATH_LENGTH;
#endif
	
	// The length of the name that is not selected now.
	size_t otherNameLength = 0;
	
	// Width of the conversation text.
	const int WIDTH = 540;
	// Margin on either side of the text.
	const int MARGIN = 20;
	
	
	// Convert a raw input text to the valid text.
	// Remove characters that can't be used in a file name.
	const string FORBIDDEN = "/\\?*:|\"<>~";
	string ValidateName(const string &input)
	{
		size_t MAX_NAME_LENGTH = 250;
#if defined _WIN32
		MAX_NAME_LENGTH -= PATH_LENGTH;
#endif
		
		string name;
		name.reserve(input.length());
		
		// Sanity check.
		if(MAX_NAME_LENGTH <= otherNameLength)
			return name;
		
		// Both first and last names need one character at least.
		const size_t maxLength = MAX_NAME_LENGTH - max(otherNameLength, static_cast<size_t>(1));
		
		// Remove the non-acceptable characters.
		for(const char c : input)
			if(!iscntrl(static_cast<unsigned char>(c)) && FORBIDDEN.find(c) == string::npos)
				name += c;
		// Truncate too long name.
		if(name.length() > maxLength)
		{
			const size_t n = Utf8::CodePointStart(name, maxLength);
			name.erase(name.begin() + n, name.end());
		}
		return name;
	}
	
	
	// Return true if the event have to handle this panel.
	bool IsFallThroughEvent(SDL_Keycode key, Uint16, const Command &, bool)
	{
		return key == '\t' || key == SDLK_RETURN || key == SDLK_KP_ENTER;
	}
}



// Constructor.
ConversationPanel::ConversationPanel(PlayerInfo &player, const Conversation &conversation, const System *system, const shared_ptr<Ship> &ship)
	: player(player), conversation(conversation), scroll(0.), system(system), ship(ship)
{
#if defined _WIN32
	PATH_LENGTH = Files::Saves().size();
#endif
	// These substitutions need to be applied on the fly as each paragraph of
	// text is prepared for display.
	subs["<first>"] = player.FirstName();
	subs["<last>"] = player.LastName();
	subs["<fullname>"] = Languages::GetFullname(player.FirstName(), player.LastName());
	if(ship)
		subs["<ship>"] = ship->Name();
	else if(player.Flagship())
		subs["<ship>"] = player.Flagship()->Name();
	
	// Begin at the start of the conversation.
	Goto(0);
}



// Draw this panel.
void ConversationPanel::Draw()
{
	// Dim out everything outside this panel.
	DrawBackdrop();
	
	// Draw the panel itself, stretching from top to bottom of the screen on
	// the left side. The edge sprite contains 10 pixels of the margin; the rest
	// of the margin is included in the filled rectangle drawn here:
	const Color &back = *GameData::Colors().Get("conversation background");
	double boxWidth = WIDTH + 2. * MARGIN - 10.;
	FillShader::Fill(
		Point(Screen::Left() + .5 * boxWidth, 0.),
		Point(boxWidth, Screen::Height()),
		back);
	
	const Sprite *edgeSprite = SpriteSet::Get("ui/right edge");
	if(edgeSprite->Height())
	{
		// If the screen is high enough, the edge sprite should repeat.
		double spriteHeight = edgeSprite->Height();
		Point pos(
			Screen::Left() + boxWidth + .5 * edgeSprite->Width(),
			Screen::Top() + .5 * spriteHeight);
		for( ; pos.Y() - .5 * spriteHeight < Screen::Bottom(); pos.Y() += spriteHeight)
			SpriteShader::Draw(edgeSprite, pos);
	}
	
	// Get the font and colors we'll need for drawing everything.
	const Font &font = FontSet::Get(14);
	const Color &selectionColor = *GameData::Colors().Get("faint");
	const Color &dim = *GameData::Colors().Get("dim");
	const Color &grey = *GameData::Colors().Get("medium");
	const Color &bright = *GameData::Colors().Get("bright");
	
	// Figure out where we should start drawing.
	Point point(
		Screen::Left() + MARGIN,
		Screen::Top() + MARGIN + scroll);
	// Draw all the conversation text up to this point.
	for(const Paragraph &it : text)
		point = it.Draw(point, grey);
	
	// Draw whatever choices are being presented.
	if(node < 0)
	{
		// The conversation has already ended. Draw a "done" button.
		static const T_ done = T_("[done]");
		int width = font.Width(done.Str());
		int height = font.Height();
		Point off(Screen::Left() + MARGIN + WIDTH - width, point.Y());
		font.Draw(done.Str(), off, bright);
		
		// Handle clicks on this button.
		AddZone(Rectangle::FromCorner(off, Point(width, height)), [this](){ this->Exit(); });
	}
	else if(choices.empty())
	{
		// This conversation node is prompting the player to enter their name.
		Point fieldSize(150, 20);
		const auto layout = Layout(fieldSize.X() - 10, Truncate::FRONT);
		
		if(!textInputPanel)
		{
			textInputPanel = shared_ptr<TextInputPanel>(new TextInputPanel(font, point, layout,
				bright, dim, ValidateName, IsFallThroughEvent));
			GetUI()->Push(textInputPanel);
		}
		
		for(int side = 0; side < 2; ++side)
		{
			Point center = point + Point(side ? 420 : 190, 9);
			Point textPoint = point + Point(side ? 350 : 120, 0);
			if(side != choice)
			{
				// Handle mouse clicks in whatever field is not selected.
				AddZone(Rectangle(center, fieldSize), [this, side](){ this->ClickName(side); });
				// Draw the name that is not selected.
				string otherName = FontUtilities::Escape(side ? lastName : firstName);
				font.Draw({otherName, layout}, textPoint, grey);
			}
			else
			{
				// Fill in whichever entry box is active right now.
				FillShader::Fill(center, fieldSize, selectionColor);
				// Update the point of the input panel.
				textInputPanel->SetPoint(textPoint);
			}
		}
		
		font.Draw(T("First name:"), point + Point(40, 0), dim);
		font.Draw(T("Last name:"), point + Point(270, 0), dim);
		
		// Draw the OK button, and remember its location.
		static const T_ ok = T_("[ok]");
		int width = font.Width(ok.Str());
		int height = font.Height();
		Point off(Screen::Left() + MARGIN + WIDTH - width, point.Y());
		font.Draw(ok.Str(), off, bright);

		// Handle clicks on this button.
		AddZone(Rectangle::FromCorner(off, Point(width, height)), SDLK_RETURN);
	}
	else
	{
		string label = "0:";
		int index = 0;
		for(const Paragraph &it : choices)
		{
			++label[0];
		
			Point center = point + it.Center();
			Point size(WIDTH, it.Height());
		
			if(index == choice)
				FillShader::Fill(center + Point(-5, 0), size + Point(30, 0), selectionColor);
			AddZone(Rectangle::FromCorner(point, size), [this, index](){ this->ClickChoice(index); });
			++index;
		
			font.Draw(label, point + Point(-15, 0), dim);
			point = it.Draw(point, bright);
		}
	}
	// Store the total height of the text.
	maxScroll = min(0., Screen::Top() - (point.Y() - scroll) + font.Height() + 15.);
}



// Handle key presses.
bool ConversationPanel::KeyDown(SDL_Keycode key, Uint16 mod, const Command &command, bool isNewPress)
{
	// Map popup happens when you press the map key, unless the name text entry
	// fields are currently active.
	if(command.Has(Command::MAP) && !choices.empty())
		GetUI()->Push(new MapDetailPanel(player, system));
	if(node < 0)
	{
		// If the conversation has ended, the only possible action is to exit.
		if(isNewPress && (key == SDLK_RETURN || key == SDLK_KP_ENTER || key == 'd'))
		{
			Exit();
			return true;
		}
		return false;
	}
	if(choices.empty())
	{
		// Right now we're asking the player to enter their name.
		string &name = (choice ? lastName : firstName);
		if(textInputPanel)
			name = textInputPanel->GetText();
		const string &otherName = (choice ? firstName : lastName);
		// The tab key toggles to the other entry field,
		// as does the return key if the other field is still empty.
		if(key == '\t' || ((key == SDLK_RETURN || key == SDLK_KP_ENTER) && otherName.empty()))
		{
			if(textInputPanel)
			{
				textInputPanel->SetText(otherName);
				otherNameLength = name.length();
			}
			choice = !choice;
		}
		else if((key == SDLK_RETURN || key == SDLK_KP_ENTER) && !firstName.empty() && !lastName.empty())
		{
			player.SetName(firstName, lastName);
			subs["<first>"] = player.FirstName();
			subs["<last>"] = player.LastName();
			subs["<fullname>"] = Languages::GetFullname(player.FirstName(), player.LastName());
			
			// Display the name the player entered.
			string name = Format::StringF(T("\t\tName: %1% %2%.\n"), player.FirstName(), player.LastName());
			text.emplace_back(name);
			
			if(textInputPanel)
			{
				GetUI()->Pop(textInputPanel.get());
				textInputPanel.reset();
			}
			Goto(node + 1);
		}
		else
			return false;
		
		return true;
	}
	
	// Let the player select choices by using the arrow keys and then pressing
	// return, or by pressing a number key.
	if(key == SDLK_UP && choice > 0)
		--choice;
	else if(key == SDLK_DOWN && choice < conversation.Choices(node) - 1)
		++choice;
	else if((key == SDLK_RETURN || key == SDLK_KP_ENTER) && isNewPress && choice < conversation.Choices(node))
		Goto(conversation.NextNode(node, choice), choice);
	else if(key >= '1' && key < static_cast<SDL_Keycode>('1' + choices.size()))
		Goto(conversation.NextNode(node, key - '1'), key - '1');
	else if(key >= SDLK_KP_1 && key < static_cast<SDL_Keycode>(SDLK_KP_1 + choices.size()))
		Goto(conversation.NextNode(node, key - SDLK_KP_1), key - SDLK_KP_1);
	else
		return false;
	
	return true;
}



// Allow scrolling by click and drag.
bool ConversationPanel::Drag(double dx, double dy)
{
	scroll = min(0., max(maxScroll, scroll + dy));
	
	return true;
}



// Handle the scroll wheel.
bool ConversationPanel::Scroll(double dx, double dy)
{
	return Drag(0., dy * Preferences::ScrollSpeed());
}



// Dim the background of this panel.
void ConversationPanel::DrawBackdrop() const
{
	if(!GetUI()->IsTop(this) && !(textInputPanel && GetUI()->IsTop(textInputPanel.get())))
		return;
	
	// Darken everything but the dialog.
	const Color &back = *GameData::Colors().Get("dialog backdrop");
	FillShader::Fill(Point(), Point(Screen::Width(), Screen::Height()), back);
}



// The player just selected the given choice.
void ConversationPanel::Goto(int index, int choice)
{
	if(index)
	{
		// Add the chosen option to the text.
		if(choice >= 0 && choice < static_cast<int>(choices.size()))
			text.splice(text.end(), choices, next(choices.begin(), choice));
		
		// Scroll to the start of the new text, unless the conversation ended.
		if(index >= 0)
		{
			scroll = -MARGIN;
			for(const Paragraph &it : text)
				scroll -= it.Height();
			scroll += text.back().BottomMargin();
		}
	}
	
	// We'll need to reload the choices from whatever new node we arrive at.
	choices.clear();
	node = index;
	// Not every conversation node allows a choice. Move forward through the
	// nodes until we encounter one that does, or the conversation ends.
	while(node >= 0 && !conversation.IsChoice(node))
	{
		int choice = 0;
		if(conversation.IsBranch(node))
		{
			// Branch nodes change the flow of the conversation based on the
			// player's condition variables rather than player input.
			choice = !conversation.Conditions(node).Test(player.Conditions());
		}
		else if(conversation.IsApply(node))
		{
			// Apply nodes alter the player's condition variables but do not
			// display any conversation text of their own.
			player.SetReputationConditions();
			conversation.Conditions(node).Apply(player.Conditions());
			// Update any altered government reputations.
			player.CheckReputationConditions();
		}
		else
		{
			// This is an ordinary conversation node. Perform any necessary text
			// replacement, then add the text to the display.
			string altered = Format::Replace(conversation.Text(node), subs);
			text.emplace_back(altered, conversation.Scene(node), text.empty());
		}
		node = conversation.NextNode(node, choice);
	}
	// Display whatever choices are being offered to the player.
	for(int i = 0; i < conversation.Choices(node); ++i)
	{
		string altered = Format::Replace(conversation.Text(node, i), subs);
		choices.emplace_back(altered);
	}
	this->choice = 0;
}



// Exit this panel and do whatever needs to happen next.
void ConversationPanel::Exit()
{
	GetUI()->Pop(this);
	// Some conversations may be offered from an NPC, e.g. an assisting or
	// boarding mission's `on offer`, or from completing a mission's NPC
	// block (e.g. scanning or boarding or killing all required targets).
	if(node == Conversation::DIE || node == Conversation::EXPLODE)
		player.Die(node, ship);
	else if(ship)
	{
		// A forced-launch ending (LAUNCH, FLEE, or DEPART) destroys any NPC.
		if(Conversation::RequiresLaunch(node))
			ship->Destroy();
		// Only show the BoardingPanel for a hostile NPC that is being boarded.
		// (NPC completion conversations can result from non-boarding events.)
		// TODO: Is there a better / more robust boarding check than relative position?
		else if(node != Conversation::ACCEPT && ship->GetGovernment()->IsEnemy()
				&& !ship->IsDestroyed() && ship->IsDisabled()
				&& ship->Position().Distance(player.Flagship()->Position()) <= 1.)
			GetUI()->Push(new BoardingPanel(player, ship));
	}
	// Call the exit response handler to manage the conversation's effect
	// on the player's missions, or force takeoff from a planet.
	if(callback)
		callback(node);
}



// The player just clicked one of the two name entry text fields.
void ConversationPanel::ClickName(int side)
{
	choice = side;
}



// The player just clicked on a conversation choice.
void ConversationPanel::ClickChoice(int index)
{
	Goto(conversation.NextNode(node, index), index);
}



// Paragraph constructor.
ConversationPanel::Paragraph::Paragraph(const string &t, const Sprite *scene, bool isFirst)
	: scene(scene), text(t, {WIDTH, Alignment::JUSTIFIED}), isFirst(isFirst)
{
	const Font &font = FontSet::Get(14);
	textHeight = font.FormattedHeight(text);
	textParagraphBreak = font.ParagraphBreak(text.GetLayout());
}



// Get the height of this paragraph (including any "scene" image).
int ConversationPanel::Paragraph::Height() const
{
	int height = textHeight;
	if(scene)
		height += 20 * !isFirst + scene->Height() + 20;
	return height;
}



// Get the center point of this paragraph.
Point ConversationPanel::Paragraph::Center() const
{
	return Point(.5 * WIDTH, .5 * (Height() - textParagraphBreak));
}



// Draw this paragraph, and return the point that the next paragraph below it
// should be drawn at.
Point ConversationPanel::Paragraph::Draw(Point point, const Color &color) const
{
	if(scene)
	{
		Point offset(WIDTH / 2, 20 * !isFirst + scene->Height() / 2);
		SpriteShader::Draw(scene, point + offset);
		point.Y() += 20 * !isFirst + scene->Height() + 20;
	}
	const Font &font = FontSet::Get(14);
	font.Draw(text, point, color);
	point.Y() += textHeight;
	return point;
}



// Bottom Margin
int ConversationPanel::Paragraph::BottomMargin() const
{
	const Font &font = FontSet::Get(14);
	return textParagraphBreak + font.LineHeight(text.GetLayout()) - font.Height();
}
