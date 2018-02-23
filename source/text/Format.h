/* Format.h
Copyright (c) 2014-2020 by Michael Zahniser

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#ifndef ES_TEXT_FORMAT_H_
#define ES_TEXT_FORMAT_H_

#include <functional>
#include <initializer_list>
#include <map>
#include <string>
#include <vector>



// Collection of functions for formatting strings for display.
class Format {
public:
	// Convert the given number into abbreviated format with a suffix like
	// "M" for million, "B" for billion, or "T" for trillion. Any number
	// above 1 quadrillion is instead shown in scientific notation.
	static std::string Credits(int64_t value);
	// Convert a time in seconds to years/days/hours/minutes/seconds
	static std::string PlayTime(double timeVal);
	// Convert the given number to a string, with at most one decimal place.
	// This is primarily for displaying ship and outfit attributes.
	static std::string Number(double value);
	// Format the given value as a number with exactly the given number of
	// decimal places (even if they are all 0).
	static std::string Decimal(double value, int places);
	// Convert a string into a number. As with the output of Number(), the
	// string can have suffixes like "M", "B", etc.
	static double Parse(const std::string &str);
	// Replace a set of "keys," which must be strings in the form "<name>", with
	// a new set of strings, and return the result.
	static std::string Replace(const std::string &source, const std::map<std::string, std::string> keys);
	// Replace all occurences of "target" with "replacement" in-place.
	static void ReplaceAll(std::string &text, const std::string &target, const std::string &replacement);
	
	// Convert a string to title caps or to lower case.
	static std::string Capitalize(const std::string &str);
	static std::string LowerCase(const std::string &str);
	
	// Split a single string into substrings with the given separator.
	static std::vector<std::string> Split(const std::string &str, const std::string &separator);
	
	// This function makes a string according to a format-string.
	// The format-string has positional directives that are replaced each
	// argument at that position. The directive %n% is replaced by nth
	// argument.
	static std::string StringF(std::initializer_list<std::string> args);
	template <class ...T>
	static std::string StringF(const std::string &format, T... args);
	
	
	// This class makes a list of words.
	class ListOfWords {
	public:
		ListOfWords();
		// This format is a string that is concatenated conjunctions that are
		// separated by a delimiter whose character is located at the begin of
		// the string. The 1st conjunction is used for 2 words-list, the 2nd and
		// 3rd conjunctions are used for 3 words-list, the remained n-1
		// conjunctions are used for n words. When a list has more than n words,
		// the center (round down) of the n words-list is used as extra.
		//
		// For example of the format ": and :, :, and "
		// The delimiter is ':'.
		// 2 words-list {"a", "b"} causes "a and b".
		// 3 words-list {"a", "b", "c"} causes "a, b, and c".
		// 4 words-list {"a", "b", "c", "d"} causes "a, b, c, and d".
		// 5 words-list {"a", "b", "c", "d", "e"} causes "a, b, c, d, and e".
		explicit ListOfWords(const std::string &format);
		
		// Set separators in advance.
		void SetSeparators(const std::string &format);
		
		// Make a string list of n words.
		// GetAndNext() returns a word, and it will return a next word at the
		// next time.
		std::string GetList(size_t n, const std::function<std::string()> &GetAndNext);
	private:
		std::vector<std::vector<std::string>> separators;
	};
};



template <class ...T>
std::string Format::StringF(const std::string &format, T... args)
{
	return StringF({format, args...});
}



#endif
