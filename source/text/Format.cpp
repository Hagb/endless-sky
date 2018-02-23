/* Format.cpp
Copyright (c) 2014-2020 by Michael Zahniser

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#include "Format.h"
#include "Gettext.h"

#include <algorithm>
#include <array>
#include <cctype>
#include <cmath>
#include <cstdint>
#include <iterator>
#include <sstream>

using namespace std;
using namespace Gettext;

namespace {
	const T_ dpoint = T_(".", "decimal point");
	const T_ sep = T_(",", "decimal separator");
	
	// Format an integer value, inserting its digits into the given string in
	// reverse order and then reversing the string.
	void FormatInteger(int64_t value, bool isNegative, string &result)
	{
		int places = 0;
		do {
			if(places && !(places % 3))
				result += sep.Str();
			++places;
			
			result += static_cast<char>('0' + value % 10);
			value /= 10;
		} while(value);
		
		if(isNegative)
			result += '-';
		
		reverse(result.begin(), result.end());
	}
}



// Convert the given number into abbreviated format with a suffix like
// "M" for million, "B" for billion, or "T" for trillion. Any number
// above 1 quadrillion is instead shown in scientific notation.
string Format::Credits(int64_t value)
{
	bool isNegative = (value < 0);
	int64_t absolute = abs(value);
	
	// If the value is above one quadrillion, show it in scientific notation.
	if(absolute > 1000000000000000ll)
	{
		ostringstream out;
		out.precision(3);
		out << static_cast<double>(value);
		return out.str();
	}
	
	// Reserve enough space for something like "-123.456M".
	string result;
	result.reserve(8);
	
	// Handle numbers bigger than a million.
	static const vector<char> SUFFIX = {'T', 'B', 'M'};
	static const vector<int64_t> THRESHOLD = {1000000000000ll, 1000000000ll, 1000000ll};
	for(size_t i = 0; i < SUFFIX.size(); ++i)
		if(absolute > THRESHOLD[i])
		{
			result += SUFFIX[i];
			int decimals = (absolute / (THRESHOLD[i] / 1000)) % 1000;
			for(int d = 0; d < 3; ++d)
			{
				result += static_cast<char>('0' + decimals % 10);
				decimals /= 10;
			}
			const string &dp = dpoint.Str();
			reverse_copy(dp.begin(), dp.end(), inserter(result, result.end()));
			absolute /= THRESHOLD[i];
			break;
		}
	
	// Convert the number to a string, adding commas if needed.
	FormatInteger(absolute, isNegative, result);
	return result;
}



// Convert a time in seconds to years/days/hours/minutes/seconds
std::string Format::PlayTime(double timeVal)
{
	string result;
	int_fast64_t timeValFormat = 0;
	static const array<T_, 5> FORMAT = {
		T_("%1%s", "play time"),
		T_("%2%m %1%s", "play time"),
		T_("%3%h %2%m %1%s", "play time"),
		T_("%4%d %3%h %2%m %1%s", "play time"),
		T_("%5%y %4%d %3%h %2%m %1%s", "play time"),
	};
	static const array<int, 4> PERIOD = {60, 60, 24, 365};
	
	timeValFormat = max(0., timeVal);
	// Break time into larger and larger units until the largest one, or the value is empty
	size_t i = 0;
	string periodStr[5] = {};
	do {
		int period = (i < FORMAT.size() - 1 ? timeValFormat % PERIOD[i] : timeValFormat);
		do {
			periodStr[i] += static_cast<char>('0' + period % 10);
			period /= 10;
		} while(period);
		reverse(periodStr[i].begin(), periodStr[i].end());
		
		if(i < PERIOD.size())
			timeValFormat /= PERIOD[i];
		i++;
	} while (timeValFormat && i < FORMAT.size());
	
	return StringF(FORMAT[i - 1].Str(), periodStr[0], periodStr[1], periodStr[2], periodStr[3], periodStr[4]);
}



// Convert the given number to a string, with at most one decimal place.
// This is primarily for displaying ship and outfit attributes.
string Format::Number(double value)
{
	if(!value)
		return "0";
	
	string result;
	bool isNegative = (value < 0.);
	value = fabs(value);
	
	// Check if this is a whole number.
	double decimal = modf(value, &value);
	if(decimal)
	{
		if(decimal >= .95)
		{
			result += '0';
			++value;
		}
		else
			result += static_cast<char>('0' + static_cast<int>(round(decimal * 10.)));
		
		const string &dp = dpoint.Str();
		reverse_copy(dp.begin(), dp.end(), inserter(result, result.end()));
	}
	
	// Convert the number to a string, adding commas if needed.
	FormatInteger(value, isNegative, result);
	return result;
}



// Format the given value as a number with exactly the given number of
// decimal places (even if they are all 0).
string Format::Decimal(double value, int places)
{
	double integer;
	double fraction = fabs(modf(value, &integer));
	
	string result = to_string(static_cast<int>(integer)) + dpoint.Str();
	while(places--)
	{
		fraction = modf(fraction * 10., &integer);
		result += ('0' + static_cast<int>(integer));
	}
	return result;
}



// Convert a string into a number. As with the output of Number(), the
// string can have suffixes like "M", "B", etc.
double Format::Parse(const string &str)
{
	double place = 1.;
	double value = 0.;
	
	size_t idx = 0;
	const size_t end = str.length();
	const size_t dpointLen = dpoint.Str().length();
	while(idx < end && !isdigit(str[idx]) && str.compare(idx, dpointLen, dpoint.Str()) != 0)
		++idx;
	
	for( ; idx < end; ++idx)
	{
		if(str.compare(idx, dpointLen, dpoint.Str()) == 0)
		{
			place = .1;
			idx += dpointLen - 1;
		}
		else if(!isdigit(str[idx]))
			break;
		else
		{
			double digit = str[idx] - '0';
			if(place < 1.)
			{
				value += digit * place;
				place *= .1;
			}
			else
			{
				value *= 10.;
				value += digit;
			}
		}
	}
	
	if(idx != end)
	{
		auto c = str[idx];
		if(c == 'k' || c == 'K')
			value *= 1e3;
		else if(c == 'm' || c == 'M')
			value *= 1e6;
		else if(c == 'b' || c == 'B')
			value *= 1e9;
		else if(c == 't' || c == 'T')
			value *= 1e12;
	}
	
	return value;
}



string Format::Replace(const string &source, const map<string, string> keys)
{
	string result;
	result.reserve(source.length());
	
	size_t start = 0;
	size_t search = start;
	while(search < source.length())
	{
		size_t left = source.find('<', search);
		if(left == string::npos)
			break;
		
		size_t right = source.find('>', left);
		if(right == string::npos)
			break;
		
		bool matched = false;
		++right;
		size_t length = right - left;
		for(const auto &it : keys)
			if(!source.compare(left, length, it.first))
			{
				result.append(source, start, left - start);
				result.append(it.second);
				start = right;
				search = start;
				matched = true;
				break;
			}
		
		if(!matched)
			search = left + 1;
	}
	
	result.append(source, start, source.length() - start);
	return result;
}



void Format::ReplaceAll(string &text, const string &target, const string &replacement)
{
	// If the searched string is an empty string, do nothing.
	if(target.empty())
		return;
	
	string newString;
	newString.reserve(text.length());
	
	// Index at which to begin searching for the target string.
	size_t start = 0;
	size_t matchLength = target.length();
	// Index at which the target string was found.
	size_t findPos = string::npos;
	while((findPos = text.find(target, start)) != string::npos)
	{
		newString.append(text, start, findPos - start);
		newString += replacement;
		start = findPos + matchLength;
	}
	
	// Add the remaining text.
	newString += text.substr(start);
	
	text.swap(newString);
}



string Format::Capitalize(const string &str)
{
	string result = str;
	bool first = true;
	for(char &c : result)
	{
		if(!isalpha(c))
			first = true;
		else
		{
			if(first && islower(c))
				c = toupper(c);
			first = false;
		}
	}
	return result;
}



string Format::LowerCase(const string &str)
{
	string result = str;
	for(char &c : result)
		c = tolower(c);
	return result;
}



// Split a single string into substrings with the given separator.
vector<string> Format::Split(const string &str, const string &separator)
{
	vector<string> result;
	size_t begin = 0;
	while(true)
	{
		size_t pos = str.find(separator, begin);
		if(pos == string::npos)
			pos = str.length();
		result.emplace_back(str, begin, pos - begin);
		begin = pos + separator.size();
		if(begin >= str.length())
			break;
	}
	return result;
}



// Make a string according to a format-string.
string Format::StringF(initializer_list<string> args)
{
	if(args.size() == 1)
		return *args.begin();
	else if(args.size() > 1)
	{
		string buf;
		int state = 0;
		size_t n = 0;
		size_t pos = 0;
		for(const auto c : *args.begin())
		{
			buf += c;
			if(state == 0)
			{
				if(c == '%')
				{
					state = 1;
					n = 0;
					pos = buf.length() - 1;
				}
			}
			else
			{
				if(c == '%')
				{
					if(0 < n && n < args.size())
					{
						buf.erase(pos, buf.length());
						buf += args.begin()[n];
					}
					state = 0;
				}
				else if(isdigit(c))
					n = 10 * n + c - '0';
				else
					state = 0;
			}
		}
		return buf;
	}
	else
		return "";
}



Format::ListOfWords::ListOfWords()
{
	SetSeparators("");
}



Format::ListOfWords::ListOfWords(const string &format)
{
	SetSeparators(format);
}



void Format::ListOfWords::SetSeparators(const string &format)
{
	separators.clear();
	const size_t sz = format.length();
	size_t n = 2;
	size_t m = 0;
	const char delm = sz > 0 ? format[0] : '\0';
	string token;
	for(size_t i = 1; i < sz; ++i)
	{
		if(format[i] != delm)
			token += format[i];
		if(format[i] == delm || i == sz - 1)
		{
			if(m == 0)
				separators.emplace_back(vector<string>{ token });
			else
				separators.back().push_back(token);
			token.clear();
			++m;
			if(n == m + 1)
			{
				++n;
				m = 0;
			}
		}
	}
	if(m != 0)
		// Something wrong.
		separators.pop_back();
	if(separators.empty())
		separators.emplace_back(vector<string>{ "" });
}



string Format::ListOfWords::GetList(size_t n, const function<string()> &GetAndNext)
{
	string s;
	if(n == 0)
		return s;
	else if (n == 1)
	{
		s = GetAndNext();
		return s;
	}
	else
	{
		const size_t m = separators.size() > n - 2 ? n - 2 : separators.size() - 1;
		const size_t sz = separators[m].size();
		const size_t firstHalf = (sz - 1) / 2;
		const size_t secondHalf = n - 1 - (sz - firstHalf);
		size_t j = 0;
		s = GetAndNext();
		for(size_t i = 0; i < n - 1; ++i)
		{
			s += separators[m][j];
			s += GetAndNext();
			if(i < firstHalf || secondHalf <= i)
				++j;
		}
		return s;
	}
}
