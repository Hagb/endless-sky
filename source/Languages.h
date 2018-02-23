/* Languages.h
Copyright (c) 2019 by OOTA, Masato

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#ifndef LANGUAGES_H_
#define LANGUAGES_H_

#include <set>
#include <string>
#include <vector>

class DataNode;

// The class managing all language informations.
class Languages {
public:
	// Initialize.
	// "sources" is the list of "source" folders.
	static void Init(const std::vector<std::string> &sources);
	
	
	// Load a language node.
	static void Load(const DataNode &node);
	
	
	// Get current language name for preferences.
	static std::string GetLanguageName();
	
	// Get current language ID.
	// Empty and "0" mean the system default language.
	static const std::string &GetLanguageID();
	
	// Set new language ID.
	// It's preferred a form of ISO-639-1 (such as "en") or
	// ISO-639-1 + '_' + ISO 3166-1 alpha-2 (such as "en_US"),
	// but any strings are allowed except empty and "0".
	// Empty and "0" mean the system default language.
	// The languageID is not added to the known language IDs.
	static void SetLanguageID(const std::string &languageID);
	
	// Get all known language IDs.
	// The known language IDs are only provided by the source folders and Load(node).
	// The return value might not contain the result of GetLanguageID().
	static const std::set<std::string> &GetKnownLanguageIDs();
	
	
	// Get <fullname>.
	static std::string GetFullname(const std::string &first, const std::string &last);
	
	// Get current <fullname> format.
	static std::string GetFullnameFormat();
	
	// Set new <fullname> format.
	// Empty and "0" mean "<first> <last>".
	// The fullnameFormat is not added to the known fullname formats.
	static void SetFullnameFormat(const std::string &fullnameFormat);
	
	// Get all known fullname formats.
	// The known fullname formats are only provided by Load(node).
	// The return value might not contain the result of GetFullnameFormat().
	static const std::vector<std::string> &GetKnownFullnameFormats();
};

#endif
