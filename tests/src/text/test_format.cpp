/* text/test_format.cpp
Copyright (c) 2021 by Terin

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#include "es-test.hpp"

// Include only the tested class's header.
#include "../../../source/text/Format.h"

// ... and any system includes needed for the test file.
#include <string>

namespace { // test namespace
using ListOfWords = Format::ListOfWords;

// #region mock data

// Insert file-local data here, e.g. classes, structs, or fixtures that will be useful
// to help test this class/method.

// #endregion mock data



// #region unit tests
SCENARIO("A unit of playing time is to be made human-readable", "[Format][PlayTime]") {
	GIVEN( "A time of 0" ) {
		THEN( "0s is returned" ) {
			CHECK( Format::PlayTime(0) == "0s");
		}
	}

	GIVEN( "A time of a half second" ) {
		THEN( "0s is returned" ) {
			CHECK( Format::PlayTime(.5) == "0s");
		}
	}

	GIVEN( "A time under a minute" ) {
		THEN( "A time in only seconds is returned" ) {
			CHECK( Format::PlayTime(47) == "47s");
		}
	}

	GIVEN( "A time over a minute but under an hour " ) {
		THEN( "A time in only minutes and seconds is returned" ) {
			CHECK( Format::PlayTime(567) == "9m 27s");
		}
	}

	GIVEN( "A time over an hour but under a day" ) {
		THEN( "A time in only hours, minutes, and seconds is returned" ) {
			CHECK( Format::PlayTime(8492) == "2h 21m 32s");
		}
	}

	GIVEN( "A time over a day but under a year" ) {
		THEN( "A time in only days, hours, minutes, and seconds is returned" ) {
			CHECK( Format::PlayTime(5669274) == "65d 14h 47m 54s");
		}
	}

	GIVEN( "A time over a year" ) {
		THEN( "A time using all units is returned" ) {
			CHECK( Format::PlayTime(98957582) == "3y 50d 8h 13m 2s");
		}
	}
	GIVEN( "A negative time" ) {
		THEN( "0s is returned " ) {
			CHECK( Format::PlayTime(-300) == "0s");
		}
	}
}

SCENARIO("A unit test of formatted string function", "[Format][StringF]") {
	GIVEN( "empty parameter" ) {
		THEN( "An empty string is returned" ) {
			CHECK( Format::StringF({}) == "" );
		}
	}
	
	GIVEN( "A single parameter" ) {
		THEN( "The parameter is returned" ) {
			CHECK( Format::StringF("abc") == "abc" );
		}
	}
	
	GIVEN( "A positional directive and the string referred by it" ) {
		THEN( "The formatted string is returned" ) {
			CHECK( Format::StringF("abc%1%def", "xyz") == "abcxyzdef");
		}
	}
	
	GIVEN( "A format that has a positional directive at the first position" ) {
		THEN( "The formatted string is returned" ) {
			CHECK( Format::StringF("%1%def", "xyz") == "xyzdef");
		}
	}
	
	GIVEN( "A format that has a positional directive at the last position" ) {
		THEN( "The formatted string is returned" ) {
			CHECK( Format::StringF("abc%1%", "xyz") == "abcxyz");
		}
	}
	
	GIVEN( "Eleven strings referred by a format" ) {
		THEN( "The formatted string is returned" ) {
			CHECK( Format::StringF("a%1%b%2%c%3%d%4%e%5%f%6%g%7%h%8%i%9%j%10%k%11%l", "p", "q", "r", "s", "t", "u", "v", "w", "x", "y", "z") == "apbqcrdsetfugvhwixjykzl");
		}
	}
	
	GIVEN( "Reverse-ordered positional parameters" ) {
		THEN( "The formatted string is returned" ) {
			CHECK( Format::StringF("a%2%b%1%c", "y", "z") == "azbyc");
		}
	}
	
	GIVEN( "Too many parameters" ) {
		THEN( "A parameter not to referred by the format is ignored" ) {
			CHECK( Format::StringF("a%1%b%2%c", "x", "y", "z") == "axbyc");
		}
	}
	
	GIVEN( "Too few parameters" ) {
		THEN( "No parameter to referred by the format is not replaced" ) {
			CHECK( Format::StringF("a%1%b%2%c", "x") == "axb%2%c");
		}
	}
	
	GIVEN( "Positional directive %0%" ) {
		THEN( "%0% is not replaced" ) {
			CHECK( Format::StringF("a%0%b%1%c", "x") == "a%0%bxc");
		}
	}
	
	GIVEN( "A format string including %" ) {
		THEN( "% is not replaced" ) {
			CHECK( Format::StringF("a%1%b%c", "x", "y") == "axb%c");
		}
	}
	
	GIVEN( "A format string ended by %1" ) {
		THEN( "%1 is not replaced" ) {
			CHECK( Format::StringF("a%1", "x") == "a%1");
		}
	}
	
	GIVEN( "A format string including %a%" ) {
		THEN( "%a% is not replaced" ) {
			CHECK( Format::StringF("x%a%z", "y") == "x%a%z");
		}
	}
}

SCENARIO("A unit test of class ListOfWords", "[Format][ListOfWords]") {
	GIVEN( "A ListOfWords constructed by default" ) {
		ListOfWords listOfWords;
		
		WHEN( "There is no items" ) {
			std::vector<std::string> items{};
			auto it = items.begin();
			THEN( "An empty string is returned" ) {
				CHECK( listOfWords.GetList(items.size(), [&it](){ return *it++; }) == "" );
			}
		}
		WHEN( "There is one items" ) {
			std::vector<std::string> items{"a"};
			auto it = items.begin();
			THEN( "The item is returned" ) {
				CHECK( listOfWords.GetList(items.size(), [&it](){ return *it++; }) == "a" );
			}
		}
		WHEN( "There is two items" ) {
			std::vector<std::string> items{"a", "b"};
			auto it = items.begin();
			THEN( "The string concatinated two items is returned" ) {
				CHECK( listOfWords.GetList(items.size(), [&it](){ return *it++; }) == "ab" );
			}
		}
		WHEN( "There is three items" ) {
			std::vector<std::string> items{"a", "b", "c"};
			auto it = items.begin();
			THEN( "The string concatinated three items is returned" ) {
				CHECK( listOfWords.GetList(items.size(), [&it](){ return *it++; }) == "abc" );
			}
		}
	}
	
	GIVEN( "A ListOfWords created by a comma separator" ) {
		ListOfWords listOfWords(":, :");
		
		WHEN( "There is no items" ) {
			std::vector<std::string> items{};
			auto it = items.begin();
			THEN( "An empty string is returned" ) {
				CHECK( listOfWords.GetList(items.size(), [&it](){ return *it++; }) == "" );
			}
		}
		WHEN( "There is one item" ) {
			std::vector<std::string> items{"a"};
			auto it = items.begin();
			THEN( "The item is returned" ) {
				CHECK( listOfWords.GetList(items.size(), [&it](){ return *it++; }) == "a" );
			}
		}
		WHEN( "There is two items" ) {
			std::vector<std::string> items{"a", "b"};
			auto it = items.begin();
			THEN( "The string separated two items by comma is returned" ) {
				CHECK( listOfWords.GetList(items.size(), [&it](){ return *it++; }) == "a, b" );
			}
		}
		WHEN( "There is three items" ) {
			std::vector<std::string> items{"a", "b", "c"};
			auto it = items.begin();
			THEN( "The string separated three items by comma is returned" ) {
				CHECK( listOfWords.GetList(items.size(), [&it](){ return *it++; }) == "a, b, c" );
			}
		}
	}
	
	GIVEN( "A ListOfWords created by an oxford comma separator" ) {
		ListOfWords listOfWords(": and :, :, and :");
		
		WHEN( "There is no items" ) {
			std::vector<std::string> items{};
			auto it = items.begin();
			THEN( "An empty string is returned" ) {
				CHECK( listOfWords.GetList(items.size(), [&it](){ return *it++; }) == "" );
			}
		}
		WHEN( "There is one item" ) {
			std::vector<std::string> items{"a"};
			auto it = items.begin();
			THEN( "The item is returned" ) {
				CHECK( listOfWords.GetList(items.size(), [&it](){ return *it++; }) == "a" );
			}
		}
		WHEN( "There is two items" ) {
			std::vector<std::string> items{"a", "b"};
			auto it = items.begin();
			THEN( "The string separated two items by oxford comma is returned" ) {
				CHECK( listOfWords.GetList(items.size(), [&it](){ return *it++; }) == "a and b" );
			}
		}
		WHEN( "There is three items" ) {
			std::vector<std::string> items{"a", "b", "c"};
			auto it = items.begin();
			THEN( "The string separated three items by oxford comma is returned" ) {
				CHECK( listOfWords.GetList(items.size(), [&it](){ return *it++; }) == "a, b, and c" );
			}
		}
		WHEN( "There is four items" ) {
			std::vector<std::string> items{"a", "b", "c", "d"};
			auto it = items.begin();
			THEN( "The string separated four items by oxford comma is returned" ) {
				CHECK( listOfWords.GetList(items.size(), [&it](){ return *it++; }) == "a, b, c, and d" );
			}
		}
		WHEN( "There is five items" ) {
			std::vector<std::string> items{"a", "b", "c", "d", "e"};
			auto it = items.begin();
			THEN( "The string separated five items by oxford comma is returned" ) {
				CHECK( listOfWords.GetList(items.size(), [&it](){ return *it++; }) == "a, b, c, d, and e" );
			}
		}
	}
	
	GIVEN( "A ListOfWords created by more complex separator" ) {
		ListOfWords listOfWords(":%:, :$:, :, :#:-:=:|:/:");
		
		WHEN( "There is no items" ) {
			std::vector<std::string> items{};
			auto it = items.begin();
			THEN( "An empty string is returned" ) {
				CHECK( listOfWords.GetList(items.size(), [&it](){ return *it++; }) == "" );
			}
		}
		WHEN( "There is one item" ) {
			std::vector<std::string> items{"a"};
			auto it = items.begin();
			THEN( "The item is returned" ) {
				CHECK( listOfWords.GetList(items.size(), [&it](){ return *it++; }) == "a" );
			}
		}
		WHEN( "There is two items" ) {
			std::vector<std::string> items{"a", "b"};
			auto it = items.begin();
			THEN( "The string separated two items by % is returned" ) {
				CHECK( listOfWords.GetList(items.size(), [&it](){ return *it++; }) == "a%b" );
			}
		}
		WHEN( "There is three items" ) {
			std::vector<std::string> items{"a", "b", "c"};
			auto it = items.begin();
			THEN( "The string separated three items by , and $ is returned" ) {
				CHECK( listOfWords.GetList(items.size(), [&it](){ return *it++; }) == "a, b$c" );
			}
		}
		WHEN( "There is four items" ) {
			std::vector<std::string> items{"a", "b", "c", "d"};
			auto it = items.begin();
			THEN( "The string separated four items by , , and # is returned" ) {
				CHECK( listOfWords.GetList(items.size(), [&it](){ return *it++; }) == "a, b, c#d" );
			}
		}
		WHEN( "There is five items" ) {
			std::vector<std::string> items{"a", "b", "c", "d", "e"};
			auto it = items.begin();
			THEN( "The string separated five items by -=|/ is returned" ) {
				CHECK( listOfWords.GetList(items.size(), [&it](){ return *it++; }) == "a-b=c|d/e" );
			}
		}
		WHEN( "There is six items" ) {
			std::vector<std::string> items{"a", "b", "c", "d", "e", "f"};
			auto it = items.begin();
			THEN( "The string separated six items by -==|/ is returned" ) {
				CHECK( listOfWords.GetList(items.size(), [&it](){ return *it++; }) == "a-b=c=d|e/f" );
			}
		}
		WHEN( "There is seven items" ) {
			std::vector<std::string> items{"a", "b", "c", "d", "e", "f", "g"};
			auto it = items.begin();
			THEN( "The string separated seven items by -===|/ is returned" ) {
				CHECK( listOfWords.GetList(items.size(), [&it](){ return *it++; }) == "a-b=c=d=e|f/g" );
			}
		}
	}
	
	GIVEN( "A ListOfWords created by a separator that has a delimiter '/'" ) {
		ListOfWords listOfWords("/%/, /$/, /, /#/-/=/|/:/");
		
		WHEN( "There is seven items" ) {
			std::vector<std::string> items{"a", "b", "c", "d", "e", "f", "g"};
			auto it = items.begin();
			THEN( "The string separated seven items by -===|: is returned" ) {
				CHECK( listOfWords.GetList(items.size(), [&it](){ return *it++; }) == "a-b=c=d=e|f:g" );
			}
		}
	}
	
	GIVEN( "A ListOfWords called SetSeparators" ) {
		ListOfWords listOfWords("/%/, /$/, /, /#/-/=/|/:/");
		listOfWords.SetSeparators("/ and /, /, and /");
		
		WHEN( "There is seven items" ) {
			std::vector<std::string> items{"a", "b", "c", "d", "e", "f", "g"};
			auto it = items.begin();
			THEN( "The string separated seven items by oxford comma is returned" ) {
				CHECK( listOfWords.GetList(items.size(), [&it](){ return *it++; }) == "a, b, c, d, e, f, and g" );
			}
		}
	}
}
// #endregion unit tests

// #region benchmarks
#ifdef CATCH_CONFIG_ENABLE_BENCHMARKING
TEST_CASE( "Benchmark Format::PlayTime", "[!benchmark][format]" ) {
	BENCHMARK( "Format::PlayTime() with a value under an hour" ) {
		return Format::PlayTime(1943);
	};
	BENCHMARK( "Format::PlayTime() with a high, but realistic playtime (40-400h)" ) {
		return Format::PlayTime(1224864);
	};
	BENCHMARK( "Format::PlayTime() with an uncapped value" ) {
		return Format::PlayTime(std::numeric_limits<int>::max());
	};
}
#endif

} // test namespace
