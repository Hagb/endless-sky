/* Date.cpp
Copyright (c) 2014 by Michael Zahniser

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#include "Date.h"
#include "text/Format.h"
#include "text/Gettext.h"

using namespace std;
using namespace Gettext;



namespace {
	// Figure out the day of the week of the given date.
	int Weekday(int day, int month, int year)
	{
		// Zeller's congruence.
		if(month < 3)
		{
			--year;
			month += 12;
		}
		day = (day + (13 * (month + 1)) / 5 + year + year / 4 + 6 * (year / 100) + year / 400) % 7;
		
		return day;
	}
	
	// The day of week.
	const T_ dayOfWeek[] = {T_("Sat"), T_("Sun"), T_("Mon"), T_("Tue"), T_("Wed"), T_("Thu"), T_("Fri")};
	// The short name of month.
	const T_ shortNameOfMonth[] = {
		T_("???", "short month"),
		T_("Jan", "short month"),
		T_("Feb", "short month"),
		T_("Mar", "short month"),
		T_("Apr", "short month"),
		T_("May", "short month"),
		T_("Jun", "short month"),
		T_("Jul", "short month"),
		T_("Aug", "short month"),
		T_("Sep", "short month"),
		T_("Oct", "short month"),
		T_("Nov", "short month"),
		T_("Dec", "short month"),
		T_("???", "short month"),
		T_("???", "short month"),
		T_("???", "short month"),
	};
	// The full name of month.
	const T_ fullNameOfMonth[] = {
		T_("???", "full month"),
		T_("January", "full month"),
		T_("February", "full month"),
		T_("March", "full month"),
		T_("April", "full month"),
		T_("May", "full month"),
		T_("June", "full month"),
		T_("July", "full month"),
		T_("August", "full month"),
		T_("September", "full month"),
		T_("October", "full month"),
		T_("November", "full month"),
		T_("December", "full month"),
		T_("???", "full month"),
		T_("???", "full month"),
		T_("???", "full month"),
	};
	// The day of the month.
	const T_ dayOfMonth[] = {
		T_("???", "Date"),
		T_("1st"), T_("2nd"), T_("3rd"), T_("4th"), T_("5th"),
		T_("6th"), T_("7th"), T_("8th"), T_("9th"), T_("10th"),
		T_("11th"), T_("12th"), T_("13th"), T_("14th"), T_("15th"),
		T_("16th"), T_("17th"), T_("18th"), T_("19th"), T_("20th"),
		T_("21st"), T_("22nd"), T_("23rd"), T_("24th"), T_("25th"),
		T_("26th"), T_("27th"), T_("28th"), T_("29th"), T_("30th"),
		T_("31st")
	};
	// The date format-string.
	// TRANSLATORS: %1%: day of week, %2%: day, %3%: short month, %4%: year
	const T_ dateStringFormat = T_("%1%, %2% %3% %4%");
	// TRANSLATORS: %1%: day,  %2%: full month
	const T_ dateStringInConversationFormat = T_("%1% of %2%", "Date");
	const T_ unknown = T_("???", "Date");
	
	// Epoch in translating.
	int epochInTranslating = 0;
	
	// The Hook of translation.
	function<void()> updateCatalog([](){
		++epochInTranslating;
	});
	// Set the hook.
	volatile bool hooked = AddHookUpdating(&updateCatalog);
}



// Since converting a date to a string is the most common operation, store the
// date in a way that allows easy extraction of the day, month, and year. Allow
// 5 bits for the day and 4 for the month. This also allows easy comparison.
Date::Date(int day, int month, int year)
	: date(day + (month << 5) + (year << 9))
{
}



// Convert a date to a string.
const string &Date::ToString() const
{
	// Because this is a somewhat "costly" operation, cache the result. The
	// cached value is discarded if the date is changed.
	if(date && (str.empty() || epoch != epochInTranslating))
	{
		int day = Day();
		int month = Month();
		int year = Year();
		int week = Weekday(day, month, year);
		
		const string y = to_string(year);
		const string &m = shortNameOfMonth[month].Str();
		const string d = to_string(Day());
		const string &w = dayOfWeek[week].Str();
		str = Format::StringF(dateStringFormat.Str(), w, d, m, y);
		epoch = epochInTranslating;
	}
	
	return str;
}



// Convert a date to the format in which it would be stated in conversation.
string Date::LongString() const
{
	if(!date)
		return string();
	
	const size_t m = Month();
	const string &month = fullNameOfMonth[m].Str();
	const size_t d = Day();
	const string &day = dayOfMonth[d].Str();
	return Format::StringF(dateStringInConversationFormat.Str(), day, month);
}



// Check if this date has been initialized.
Date::operator bool() const
{
	return !!*this;
}



// Check if this date has not been initialized.
bool Date::operator!() const
{
	return !date;
}



// Increment this date (prefix).
Date &Date::operator++()
{
	*this = *this + 1;
	return *this;
}



// Increment this date (postfix).
Date Date::operator++(int)
{
	auto before = *this;
	++*this;
	return before;
}



// Add the given number of days to this date.
Date Date::operator+(int days) const
{
	// If this date is not initialized, adding to it does nothing.
	if(!date || !days)
		return *this;
	
	int day = Day();
	int month = Month();
	int year = Year();
	
	day += days;
	int leap = !(year % 4) - !(year % 100) + !(year % 400);
	int MDAYS[] = {31, 28 + leap, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
	while(day > MDAYS[month - 1])
	{
		// Moving forward in time:
		day -= MDAYS[month - 1];
		++month;
		if(month == 13)
		{
			month = 1;
			++year;
			// If we cycle to a new year, recalculate the days in February.
			MDAYS[1] = 28 + !(year % 4) - !(year % 100) + !(year % 400);
		}
	}
	while(day < 1)
	{
		// Moving backward in time:
		--month;
		if(month == 0)
		{
			month = 12;
			--year;
			// If we cycle to a new year, recalculate the days in February.
			MDAYS[1] = 28 + !(year % 4) - !(year % 100) + !(year % 400);
		}
		day += MDAYS[month - 1];
	}
	return Date(day, month, year);
}



// Get the number of days between the two given dates.
int Date::operator-(const Date &other) const
{
	return DaysSinceEpoch() - other.DaysSinceEpoch();
}



// Date comparison.
bool Date::operator<(const Date &other) const
{
	return date < other.date;
}



bool Date::operator<=(const Date &other) const
{
	return date <= other.date;
}



bool Date::operator>(const Date &other) const
{
	return date > other.date;
}



bool Date::operator>=(const Date &other) const
{
	return date >= other.date;
}



bool Date::operator==(const Date &other) const
{
	return date == other.date;
}



bool Date::operator!=(const Date &other) const
{
	return date != other.date;
}



// Get the number of days that have elapsed since the "epoch". This is used only
// for finding the number of days in between two dates.
int Date::DaysSinceEpoch() const
{
	if(date && !daysSinceEpoch)
	{
		daysSinceEpoch = Day();
		int month = Month();
		int year = Year();
		
		// Months contain a variable number of days.
		int MDAYS[] = {0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334};
		daysSinceEpoch += MDAYS[month - 1];
		// Add in a leap day if this is a leap year and it is after February.
		if(month > 2 && !(year % 4) && ((year % 100) || !(year % 400)))
			++daysSinceEpoch;
		
		// Simplify the calculations by starting from year 1, so that leap years
		// occur at the very end of the cycle.
		--year;
		
		// Every four centuries is 365.2425*400 = 146097 days.
		daysSinceEpoch += 146097 * (year / 400);
		year %= 400;
	
		// Every century since the last one divisible by 400 contains 36524 days.
		daysSinceEpoch += 36524 * (year / 100);
		year %= 100;
	
		// Every four years since the century contain 4 * 365 + 1 = 1461 days.
		daysSinceEpoch += 1461 * (year / 4);
		year %= 4;
	
		// Every year since the last leap year contains 365 days.
		daysSinceEpoch += 365 * year;
	}
	return daysSinceEpoch;
}



// Get the current day of the month.
int Date::Day() const
{
	return (date & 31);
}



// Get the current month (January = 1, rather than being zero-indexed).
int Date::Month() const
{
	return ((date >> 5) & 15);
}



// Get the current year.
int Date::Year() const
{
	return (date >> 9);
}
