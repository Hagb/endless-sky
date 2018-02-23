/* Languages.cpp
Copyright (c) 2019 by OOTA, Masato

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#include "Languages.h"

#include "DataNode.h"
#include "text/Format.h"
#include "text/Gettext.h"
#include "Files.h"
#include "text/FontSet.h"

#include <algorithm>
#include <functional>
#include <locale>
#include <map>
#include <stdexcept>

#ifdef _WIN32
#include <windows.h>
#endif

using namespace std;
using namespace Gettext;

namespace {
	// This class contains information related to a language.
	class Language {
	public:
		explicit Language(const string &ID);
		
		void Load(const DataNode &node);
		
		// Get the name of the language that shows in Preferences.
		const string &GetDisplayName() const;
		
		// Get the font description.
		const string &GetFontDescription() const;
		// Get the scale for the line height.
		double GetLineHeightScale() const;
		// Get the scale for the paragraph break.
		double GetParagraphBreakScale() const;
		// Get the lang code that will set to Pango.
		const string &GetLangCode() const;
		// Get the language ID that is the directory name of the catalogs.
		const string &GetLangID() const;
		
		// Get/Add the message catalog files.
		const vector<string> &CatalogFiles() const;
		void AddCatalogFile(const string &catalogFile);
		
		// Get the fullname formats.
		const vector<string> &GetFullnameFormats() const;
		
		
	private:
		bool alreadyLoaded = false;
		string displayName;
		string fontDesc = "Ubuntu";
		double lineHeightScale = 1.20;
		double paragraphBreakScale = 0.40;
		string langCode;
		const string langID;
		vector<string> catalogFiles{};
		vector<string> fullnameFormats{"<first> <last>"};
	};
	
	
	
	// The default "<fullname>" is equal to "<first> <last>".
	const vector<string> defaultFullnameFormats{"<first> <last>"};
	
	
	
	// List of known language IDs. (empty means system default)
	set<string> knownLanguageIDs{""};
	
	// map of a language. (language ID -> an instance of Language)
	map<string, Language> languages;
	
	// Current language ID.
	string currentLanguageID;
	
	// Current language ID where the catalog files were loaded from.
	string currentCatalogID;
	
	// Current fullname format.
	string currentFullnameFormat;
	
	
	
	// Extract the language code that is a front of '_' or '-' in the LanguageID.
	string ExtractLanguageCode(const string &languageID)
	{
		size_t n = languageID.find('_');
		if(n == string::npos)
			n = languageID.find('-');
		return string(languageID, 0, n);
	}
	
	
	
	// Get the system locale name (ISO-639-1 and ISO 3166-1 alpha-2).
	string GetSystemLocaleName()
	{
#ifdef _WIN32
		char lang[9];
		GetLocaleInfoA(LOCALE_USER_DEFAULT, LOCALE_SISO639LANGNAME, lang, sizeof(lang));
		char ctry[9];
		GetLocaleInfoA(LOCALE_USER_DEFAULT, LOCALE_SISO3166CTRYNAME, ctry, sizeof(ctry));
		return string(lang) + '_' + string(ctry);
#else
		try
		{
			string loc = locale("").name();
			size_t r = loc.find('.');
			if(r == string::npos)
				return loc;
			else
				return string(loc, 0, r);
		}
		catch (runtime_error &e)
		{
			return "C";
		}
#endif
	}
	
	
	
	// Convert the default langauge to real language ID.
	string ConvertDefaultToRealLanguageID()
	{
		const string loc = GetSystemLocaleName();
		
		// Choose one of the known languages if it has been already existed.
		// Try to match a locale name exactly.
		auto it = find(knownLanguageIDs.begin(), knownLanguageIDs.end(), loc);
		if(it != knownLanguageIDs.end())
			return *it;
		
		// Try to match a language code.
		const string lang = ExtractLanguageCode(loc);
		it = find(knownLanguageIDs.begin(), knownLanguageIDs.end(), lang);
		if(it != knownLanguageIDs.end())
			return *it;
		
		return loc;
	}
	
	
	
	// Get an instance of language from the map of languages.
	Language &GetLanguage(const string &languageID)
	{
		string ID = (languageID.empty() || languageID == "0") ?
			ConvertDefaultToRealLanguageID() : languageID;
		auto it = languages.find(ID);
		if(it != languages.end())
			return it->second;
		else
			// Construct new instance of Language if it has not been existed.
			return languages.emplace(ID, Language{ID}).first->second;
	}
	
	
	
	Language::Language(const string &ID)
		: displayName(ID), langCode(ExtractLanguageCode(ID)), langID(ID)
	{
		// '_' needs an escape character.
		Format::ReplaceAll(displayName, string{"_"}, string{"__"});
	}
	
	
	
	void Language::Load(const DataNode &node)
	{
		if(node.Token(0) != "language")
		{
			node.PrintTrace("Not a language node:");
			return;
		}
		if(node.Size() != 2)
		{
			node.PrintTrace("Must have one language ID parameter:");
			return;
		}
		if(alreadyLoaded)
		{
			node.PrintTrace("Duplicate language node:");
			return;
		}
		displayName = node.Token(1);
		langCode = ExtractLanguageCode(displayName);
		
		for(const DataNode &child : node)
		{
			if(child.Token(0) == "name" && child.Size() >= 2)
				displayName = child.Token(1);
			else if(child.Token(0) == "font description" && child.Size() >= 2)
				fontDesc = child.Token(1);
			else if(child.Token(0) == "line height scale" && child.Size() >= 2)
				lineHeightScale = child.Value(1);
			else if(child.Token(0) == "paragraph break scale" && child.Size() >= 2)
				paragraphBreakScale = child.Value(1);
			else if(child.Token(0) == "lang" && child.Size() >= 2)
				langCode = child.Token(1);
			else if(child.Token(0) == "fullname")
				for(const DataNode &grand : child)
				{
					const string &fullname = grand.Token(0);
					auto it = find(fullnameFormats.begin(), fullnameFormats.end(), fullname);
					if(it == fullnameFormats.end())
						fullnameFormats.push_back(grand.Token(0));
					else
						grand.PrintTrace("Duplicate <fullname> format:");
				}
			else
				child.PrintTrace("Skipping unrecognized attribute:");
		}
		
		alreadyLoaded = true;
	}
	
	
	
	const string &Language::GetDisplayName() const
	{
		return displayName;
	}
	
	
	
	const string &Language::GetFontDescription() const
	{
		return fontDesc;
	}
	
	
	
	double Language::GetLineHeightScale() const
	{
		return lineHeightScale;
	}
	
	
	
	double Language::GetParagraphBreakScale() const
	{
		return paragraphBreakScale;
	}
	
	
	
	const string &Language::GetLangCode() const
	{
		return langCode;
	}
	
	
	
	const string &Language::GetLangID() const
	{
		return langID;
	}
	
	
	
	const vector<string> &Language::CatalogFiles() const
	{
		return catalogFiles;
	}
	
	
	
	void Language::AddCatalogFile(const string &catalogFile)
	{
		catalogFiles.push_back(catalogFile);
	}
	
	
	
	const vector<string> &Language::GetFullnameFormats() const
	{
		if(fullnameFormats.empty())
			return defaultFullnameFormats;
		else
			return fullnameFormats;
	}
}



void Languages::Init(const vector<string> &sources)
{
	for(const string &source : sources)
	{
		const vector<string> langDirs = Files::ListDirectories(source + "locales/");
		for(const auto &langDir : langDirs)
		{
			string langID = langDir;
			langID.pop_back();
			langID = Files::Name(langID);
			if(!langID.empty() && langID != "0")
			{
				auto &language = GetLanguage(langID);
				for(const auto &filename : Files::List(langDir))
					if(Files::Extension(filename) == ".po")
						language.AddCatalogFile(filename);
				knownLanguageIDs.insert(langID);
			}
			else
			{
				Files::LogError("Warning: Invalid langID \"" + langID + "\" is contained in \"" + langDir + ".\"");
			}
		}
	}
	
	SetLanguageID(currentLanguageID);
}



void Languages::Load(const DataNode &node)
{
	if(node.Token(0) != "language")
	{
		node.PrintTrace("Not a language node:");
		return;
	}
	if(node.Size() != 2)
	{
		node.PrintTrace("Must have one language ID parameter:");
		return;
	}
	
	const string langID = node.Token(1);
	if(langID.empty() || langID == "0")
	{
		node.PrintTrace("A langID must not be \"\" and \"0\":");
		return;
	}
	GetLanguage(langID).Load(node);
	knownLanguageIDs.insert(langID);
	SetLanguageID(currentLanguageID);
}



string Languages::GetLanguageName()
{
	if(currentLanguageID.empty() || currentLanguageID == "0")
		return T("system default");
	else
		return GetLanguage(currentLanguageID).GetDisplayName();
}



const string &Languages::GetLanguageID()
{
	return currentLanguageID;
}



void Languages::SetLanguageID(const string &languageID)
{
	currentLanguageID = languageID == "0" ? "" : languageID;
	auto &lang = GetLanguage(currentLanguageID);
	Font::DrawingSettings settings;
	settings.description = lang.GetFontDescription();
	settings.language = lang.GetLangCode();
	settings.lineHeightScale = lang.GetLineHeightScale();
	settings.paragraphBreakScale = lang.GetParagraphBreakScale();
	FontSet::SetDrawingSettings(settings);
	
	// Avoid to load the catalogs as far as possible because
	// Gettext::UpdateCatalog() may be a heavy operation.
	if(currentCatalogID != lang.GetLangID())
		Gettext::UpdateCatalog(lang.CatalogFiles());
	currentCatalogID = lang.GetLangID();
}



const set<string> &Languages::GetKnownLanguageIDs()
{
	return knownLanguageIDs;
}



string Languages::GetFullname(const string &first, const string &last)
{
	return Format::Replace(currentFullnameFormat, { {"<first>", first}, {"<last>", last} });
}



string Languages::GetFullnameFormat()
{
	return currentFullnameFormat;
}



void Languages::SetFullnameFormat(const string &fullnameFormat)
{
	if(!fullnameFormat.empty() && fullnameFormat != "0")
		currentFullnameFormat = fullnameFormat;
	else
		currentFullnameFormat = defaultFullnameFormats.front();
}



const vector<string> &Languages::GetKnownFullnameFormats()
{
	return GetLanguage(currentLanguageID).GetFullnameFormats();
}
