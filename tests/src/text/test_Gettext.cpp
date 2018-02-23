#include "es-test.hpp"

// Include only the tested class's header.
#include "../../source/text/Gettext.h"

// ... and any system includes needed for the test file.
#include <cstring>
#include <sstream>
#include <string>

namespace { // test namespace
using namespace Gettext;

// #region mock data
void SetCatalog(const std::string &po)
{
	std::istringstream istr(po);
	std::string istrname("-");
	std::vector<std::pair<std::istream*, const std::string *>> catalogs;
	catalogs.emplace_back(std::make_pair(&istr, &istrname));
	UpdateCatalog(catalogs);
}
// #endregion mock data



// #region unit tests
SCENARIO("A unit test of namespace Gettext", "[Gettext]") {
	GIVEN( "Initial state" ) {
		WHEN( "G() is called" ) {
			THEN( "G() returns msgid." ) {
				CHECK( strcmp(G("abc"), "abc") == 0 );
				CHECK( strcmp(G("abc", "xyz"), "abc") == 0 );
			}
		}
		WHEN( "T() is called" ) {
			THEN( "T() returns msgid." ) {
				CHECK( T("abc") == "abc" );
				CHECK( T("abc", "xyz") == "abc" );
			}
		}
		WHEN( "nT() is called" ) {
			THEN( "nT() returns msgid." ) {
				CHECK( nT("abc", "abcs", 1) == "abc" );
				CHECK( nT("abc", "abcs", 2) == "abcs" );
				CHECK( nT("abc", "abcs", "xyz", 1) == "abc" );
				CHECK( nT("abc", "abcs", "xyz", 2) == "abcs" );
			}
		}
		WHEN( "Class T_ is used" ) {
			T_ t("abc");
			T_ tc("abc", "xzy");
			T_ td("abc", T_::DONT);
			T_ tf("abc", T_::FORCE);
			T_ tx = Tx("abc");
			THEN( "Str() returns msgid." ) {
				CHECK( t.Str() == "abc" );
				CHECK( t.Str() == t.Original() );
				CHECK( tc.Str() == "abc" );
				CHECK( tc.Str() == tc.Original() );
				CHECK( td.Str() == "abc" );
				CHECK( td.Str() == td.Original() );
				CHECK( tf.Str() == "abc" );
				CHECK( tf.Str() == tf.Original() );
				CHECK( tx.Str() == "abc" );
				CHECK( tx.Str() == tx.Original() );
			}
		}
		WHEN( "IsTranslating() is called" ) {
			THEN( "IsTranslating() retuens true." ) {
				CHECK( IsTranslating() );
			}
		}
	}
	
	GIVEN( "A catalog including a singular msgid without msgctxt" ) {
		SetCatalog(R"(
msgid ""
msgstr ""
"Plural-Forms: nplurals=2; plural=(n != 1);"

msgid "abc"
msgstr "ABC"
)");
		WHEN( "G() is called" ) {
			THEN( "G() returns msgid." ) {
				CHECK( strcmp(G("abc"), "abc") == 0 );
				CHECK( strcmp(G("abc", "xyz"), "abc") == 0 );
			}
		}
		WHEN( "T() is called" ) {
			THEN( "T(msgid) returns msgstr." ) {
				CHECK( T("abc") == "ABC" );
			}
			THEN( "T(msgid, msgctxt) returns msgid." ) {
				CHECK( T("abc", "xyz") == "abc" );
			}
		}
		WHEN( "nT() is called" ) {
			THEN( "nT(msgid) returns msgstr." ) {
				CHECK( nT("abc", "abcs", 1) == "ABC" );
				CHECK( nT("abc", "abcs", 2) == "ABC" );
			}
			THEN( "nT(msgid, msgctxt) returns msgid." ) {
				CHECK( nT("abc", "abcs", "xyz", 1) == "abc" );
				CHECK( nT("abc", "abcs", "xyz", 2) == "abcs" );
			}
		}
		WHEN( "Class T_ is used" ) {
			T_ t("abc");
			T_ tc("abc", "xyz");
			T_ td("abc", T_::DONT);
			T_ tf("abc", T_::FORCE);
			T_ tx = Tx("abc");
			THEN( "T_::Str() returns msgstr." ) {
				CHECK( t.Str() == "ABC" );
				CHECK( tf.Str() == "ABC" );
			}
			THEN( "T_::Str() returns msgid." ) {
				CHECK( tc.Str() == "abc" );
				CHECK( td.Str() == "abc" );
				CHECK( tx.Str() == "abc" );
			}
		}
		WHEN( "IsTranslating() is called" ) {
			THEN( "IsTranslating() retuens true." ) {
				CHECK( IsTranslating() );
			}
		}
	}
	
	GIVEN( "A catalog including a singular msgid with msgctxt" ) {
		SetCatalog(R"(
msgid ""
msgstr ""
"Plural-Forms: nplurals=2; plural=(n != 1);"

msgctxt "xyz"
msgid "abc"
msgstr "ABC"
)");
		WHEN( "G() is called" ) {
			THEN( "G() returns msgid." ) {
				CHECK( strcmp(G("abc"), "abc") == 0 );
				CHECK( strcmp(G("abc", "xyz"), "abc") == 0 );
			}
		}
		WHEN( "T() is called" ) {
			THEN( "T(msgid) returns msgid." ) {
				CHECK( T("abc") == "abc" );
			}
			THEN( "T(msgid, msgctxt) returns msgstr." ) {
				CHECK( T("abc", "xyz") == "ABC" );
			}
		}
		WHEN( "nT() is called" ) {
			THEN( "nT(msgid) returns msgid." ) {
				CHECK( nT("abc", "abcs", 1) == "abc" );
				CHECK( nT("abc", "abcs", 2) == "abcs" );
			}
			THEN( "nT(msgid, msgctxt) returns msgstr." ) {
				CHECK( nT("abc", "abcs", "xyz", 1) == "ABC" );
				CHECK( nT("abc", "abcs", "xyz", 2) == "ABC" );
			}
		}
		WHEN( "Class T_ is used" ) {
			T_ t("abc");
			T_ tc("abc", "xyz");
			T_ td("abc", T_::DONT);
			T_ tf("abc", T_::FORCE);
			T_ tx = Tx("abc");
			THEN( "T_::Str() returns msgid." ) {
				CHECK( t.Str() == "abc" );
				CHECK( tf.Str() == "abc" );
				CHECK( td.Str() == "abc" );
				CHECK( tx.Str() == "abc" );
			}
			THEN( "T_::Str() returns msgstr." ) {
				CHECK( tc.Str() == "ABC" );
			}
		}
		WHEN( "IsTranslating() is called" ) {
			THEN( "IsTranslating() retuens true." ) {
				CHECK( IsTranslating() );
			}
		}
	}
	
	GIVEN( "A catalog including a plural msgid without msgctxt" ) {
		SetCatalog(R"(
msgid ""
msgstr ""
"Plural-Forms: nplurals=2; plural=(n != 1);"

msgid "abc"
msgid_plural "abcs"
msgstr[0] "ABC"
msgstr[1] "ABCs"
)");
		WHEN( "G() is called" ) {
			THEN( "G() returns msgid." ) {
				CHECK( strcmp(G("abc"), "abc") == 0 );
				CHECK( strcmp(G("abc", "xyz"), "abc") == 0 );
			}
		}
		WHEN( "T() is called" ) {
			THEN( "T(msgid) returns msgstr." ) {
				CHECK( T("abc") == "ABC" );
			}
			THEN( "T(msgid, msgctxt) returns msgid." ) {
				CHECK( T("abc", "xyz") == "abc" );
			}
		}
		WHEN( "nT() is called" ) {
			THEN( "nT(msgid) returns msgstr." ) {
				CHECK( nT("abc", "abcs", 1) == "ABC" );
				CHECK( nT("abc", "abcs", 2) == "ABCs" );
			}
			THEN( "nT(msgid, msgctxt) returns msgid." ) {
				CHECK( nT("abc", "abcs", "xyz", 1) == "abc" );
				CHECK( nT("abc", "abcs", "xyz", 2) == "abcs" );
			}
		}
		WHEN( "Class T_ is used" ) {
			T_ t("abc");
			T_ tc("abc", "xyz");
			T_ td("abc", T_::DONT);
			T_ tf("abc", T_::FORCE);
			T_ tx = Tx("abc");
			THEN( "T_::Str() returns msgstr." ) {
				CHECK( t.Str() == "ABC" );
				CHECK( tf.Str() == "ABC" );
			}
			THEN( "T_::Str() returns msgid." ) {
				CHECK( tc.Str() == "abc" );
				CHECK( td.Str() == "abc" );
				CHECK( tx.Str() == "abc" );
			}
		}
		WHEN( "IsTranslating() is called" ) {
			THEN( "IsTranslating() retuens true." ) {
				CHECK( IsTranslating() );
			}
		}
	}
	
	GIVEN( "A catalog including a plural msgid with msgctxt" ) {
		SetCatalog(R"(
msgid ""
msgstr ""
"Plural-Forms: nplurals=2; plural=(n != 1);"

msgctxt "xyz"
msgid "abc"
msgid_plural "abcs"
msgstr[0] "ABC"
msgstr[1] "ABCs"
)");
		WHEN( "G() is called" ) {
			THEN( "G() returns msgid." ) {
				CHECK( strcmp(G("abc"), "abc") == 0 );
				CHECK( strcmp(G("abc", "xyz"), "abc") == 0 );
			}
		}
		WHEN( "T() is called" ) {
			THEN( "T(msgid) returns msgid." ) {
				CHECK( T("abc") == "abc" );
			}
			THEN( "T(msgid, msgctxt) returns msgstr." ) {
				CHECK( T("abc", "xyz") == "ABC" );
			}
		}
		WHEN( "nT() is called" ) {
			THEN( "nT(msgid) returns msgid." ) {
				CHECK( nT("abc", "abcs", 1) == "abc" );
				CHECK( nT("abc", "abcs", 2) == "abcs" );
			}
			THEN( "nT(msgid, msgctxt) returns msgstr." ) {
				CHECK( nT("abc", "abcs", "xyz", 1) == "ABC" );
				CHECK( nT("abc", "abcs", "xyz", 2) == "ABCs" );
			}
		}
		WHEN( "Class T_ is used" ) {
			T_ t("abc");
			T_ tc("abc", "xyz");
			T_ td("abc", T_::DONT);
			T_ tf("abc", T_::FORCE);
			T_ tx = Tx("abc");
			THEN( "T_::Str() returns msgid." ) {
				CHECK( t.Str() == "abc" );
				CHECK( tf.Str() == "abc" );
				CHECK( td.Str() == "abc" );
				CHECK( tx.Str() == "abc" );
			}
			THEN( "T_::Str() returns msgstr." ) {
				CHECK( tc.Str() == "ABC" );
			}
		}
		WHEN( "IsTranslating() is called" ) {
			THEN( "IsTranslating() retuens true." ) {
				CHECK( IsTranslating() );
			}
		}
	}
	
	GIVEN( "A catalog including a empty msgid with/without msgctxt" ) {
		SetCatalog(R"(
msgid ""
msgstr ""
"Plural-Forms: nplurals=2; plural=(n != 1);"

msgctxt "xyz"
msgid ""
msgstr "EMPTY"
)");
		WHEN( "G() is called" ) {
			THEN( "G() returns msgid." ) {
				CHECK( strcmp(G(""), "") == 0 );
				CHECK( strcmp(G("", "xyz"), "") == 0 );
			}
		}
		WHEN( "T() is called" ) {
			THEN( "T(\"\") returns \"\"." ) {
				CHECK( T("") == "" );
			}
			THEN( "T(\"\", msgctxt) returns msgstr." ) {
				CHECK( T("", "xyz") == "EMPTY" );
			}
		}
		WHEN( "nT() is called" ) {
			THEN( "nT(\"\") returns \"\"." ) {
				CHECK( nT("", "", 1) == "" );
				CHECK( nT("", "", 2) == "" );
			}
			THEN( "nT(\"\", msgctxt) returns msgstr." ) {
				CHECK( nT("", "", "xyz", 1) == "EMPTY" );
				CHECK( nT("", "", "xyz", 2) == "EMPTY" );
			}
		}
		WHEN( "Class T_ is used" ) {
			T_ t("");
			T_ tc("", "xyz");
			T_ td("", T_::DONT);
			T_ tf("", T_::FORCE);
			T_ tx = Tx("");
			THEN( "T_::Str() returns \"\"." ) {
				CHECK( t.Str() == "" );
				CHECK( tf.Str() == "" );
				CHECK( td.Str() == "" );
				CHECK( tx.Str() == "" );
			}
			THEN( "T_::Str() returns msgstr." ) {
				CHECK( tc.Str() == "EMPTY" );
			}
		}
	}
	
	GIVEN( "A catalog including two same msgid" ) {
		SetCatalog(R"(
msgid ""
msgstr ""
"Plural-Forms: nplurals=2; plural=(n != 1);"

msgid "abc"
msgstr "ABC1"

msgctxt "xyz"
msgid "abc"
msgstr "xyzABC1"

msgid "abc"
msgstr "ABC2"

msgctxt "xyz"
msgid "abc"
msgstr "xyzABC2"
)");
		WHEN( "G() is called" ) {
			THEN( "G() returns msgid." ) {
				CHECK( strcmp(G("abc"), "abc") == 0 );
				CHECK( strcmp(G("abc", "xyz"), "abc") == 0 );
			}
		}
		WHEN( "T() is called" ) {
			THEN( "T() returns the first msgstr." ) {
				CHECK( T("abc") == "ABC1" );
			}
			THEN( "T(msgid, msgctxt) returns the first msgstr." ) {
				CHECK( T("abc", "xyz") == "xyzABC1" );
			}
		}
		WHEN( "nT() is called" ) {
			THEN( "nT() returns the first msgstr." ) {
				CHECK( nT("abc", "abcs", 1) == "ABC1" );
				CHECK( nT("abc", "abcs", 2) == "ABC1" );
			}
			THEN( "nT(msgid, msgctxt) returns the first msgstr." ) {
				CHECK( nT("abc", "abcs", "xyz", 1) == "xyzABC1" );
				CHECK( nT("abc", "abcs", "xyz", 2) == "xyzABC1" );
			}
		}
		WHEN( "Class T_ is used" ) {
			T_ t("abc");
			T_ tc("abc", "xyz");
			T_ td("abc", T_::DONT);
			T_ tf("abc", T_::FORCE);
			T_ tx = Tx("abc");
			THEN( "T_::Str() returns the first msgstr." ) {
				CHECK( t.Str() == "ABC1" );
				CHECK( tc.Str() == "xyzABC1" );
				CHECK( tf.Str() == "ABC1" );
			}
			THEN( "T_::Str() returns the msgid." ) {
				CHECK( td.Str() == "abc" );
				CHECK( tx.Str() == "abc" );
			}
		}
	}
	
	GIVEN( "UpdateCatalog() and then StopTranslation()" ) {
		SetCatalog(R"(
msgid ""
msgstr ""
"Plural-Forms: nplurals=2; plural=(n != 1);"

msgid "abc"
msgstr "ABC"
)");
		StopTranslating();
		
		WHEN( "T() is called" ) {
			THEN( "T(msgid) returns msgid." ) {
				CHECK( T("abc") == "abc" );
			}
		}
		WHEN( "nT() is called" ) {
			THEN( "nT(msgid) returns msgid." ) {
				CHECK( nT("abc", "abcs", 1) == "abc" );
				CHECK( nT("abc", "abcs", 2) == "abcs" );
			}
		}
		WHEN( "Class T_ is used" ) {
			T_ t("abc");
			T_ td("abc", T_::DONT);
			T_ tf("abc", T_::FORCE);
			T_ tx = Tx("abc");
			THEN( "T_::Str() returns msgid." ) {
				CHECK( t.Str() == "abc" );
				CHECK( tf.Str() == "abc" );
				CHECK( td.Str() == "abc" );
				CHECK( tx.Str() == "abc" );
			}
		}
		WHEN( "IsTranslating() is called" ) {
			THEN( "IsTranslating() retuens false." ) {
				CHECK_FALSE( IsTranslating() );
			}
		}
		
		RestartTranslating();
	}
	
	GIVEN( "StopTranslation() and then UpdateCatalog(), RestartTranslating()" ) {
		StopTranslating();
		SetCatalog(R"(
msgid ""
msgstr ""
"Plural-Forms: nplurals=2; plural=(n != 1);"

msgid "abc"
msgstr "ABC"
)");
		RestartTranslating();
		
		WHEN( "T() is called" ) {
			THEN( "T(msgid) returns msgstr." ) {
				CHECK( T("abc") == "ABC" );
			}
		}
		WHEN( "nT() is called" ) {
			THEN( "nT(msgid) returns msgstu." ) {
				CHECK( nT("abc", "abcs", 1) == "ABC" );
				CHECK( nT("abc", "abcs", 2) == "ABC" );
			}
		}
		WHEN( "Class T_ is used" ) {
			T_ t("abc");
			T_ td("abc", T_::DONT);
			T_ tf("abc", T_::FORCE);
			T_ tx = Tx("abc");
			THEN( "T_::Str() returns msgstr." ) {
				CHECK( t.Str() == "ABC" );
				CHECK( tf.Str() == "ABC" );
			}
			THEN( "T_::Str() returns msgid." ) {
				CHECK( td.Str() == "abc" );
				CHECK( tx.Str() == "abc" );
			}
		}
		WHEN( "IsTranslating() is called" ) {
			THEN( "IsTranslating() retuens true." ) {
				CHECK( IsTranslating() );
			}
		}
	}
	
	GIVEN( "Some instances of class T_ constructed during not IsTranslating()" ) {
		SetCatalog(R"(
msgid ""
msgstr ""
"Plural-Forms: nplurals=2; plural=(n != 1);"

msgid "abc"
msgstr "ABC"
)");
		T_ tprev("abc");
		StopTranslating();
		T_ t("abc");
		T_ tcopy = tprev;
		T_ td("abc", T_::DONT);
		T_ tf("abc", T_::FORCE);
		T_ tx = Tx("abc");
		RestartTranslating();
		T_ tcopy2 = t;
		
		WHEN( "T_::Str() is called" ) {
			THEN( "T_::Str() returns msgstr." ) {
				CHECK( tf.Str() == "ABC" );
				CHECK( tprev.Str() == "ABC" );
				CHECK( tcopy.Str() == "ABC" );
			}
			THEN( "T_::Str() returns msgid." ) {
				CHECK( t.Str() == "abc" );
				CHECK( td.Str() == "abc" );
				CHECK( tx.Str() == "abc" );
				CHECK( tcopy2.Str() == "abc" );
			}
		}
	}
	
	GIVEN( "A vector of class T_" ) {
		SetCatalog(R"(
msgid ""
msgstr ""
"Plural-Forms: nplurals=2; plural=(n != 1);"

msgid "abc"
msgstr "ABC"
)");
		std::vector<T_> vecT;
		
		WHEN( "Concat() is called" ) {
			vecT.emplace_back("abc");
			vecT.emplace_back("def");
			vecT.emplace_back("abc");
			THEN( "Concat() returns the text concatenated all T_::Str()." ) {
				CHECK( Concat(vecT) == "ABCdefABC" );
			}
		}
		WHEN( "IsEmptyText() is called for an empty vector." ) {
			THEN( "IsEmptyText() returns true." ) {
				CHECK( IsEmptyText(vecT) );
			}
		}
		WHEN( "IsEmptyText() is called for an vector whose items are the empty strings." ) {
			vecT.emplace_back("");
			vecT.emplace_back("");
			vecT.emplace_back("");
			THEN( "IsEmptyText() returns true." ) {
				CHECK( IsEmptyText(vecT) );
			}
		}
		WHEN( "IsEmptyText() is called for an vector including some texts." ) {
			vecT.emplace_back("");
			vecT.emplace_back("a");
			vecT.emplace_back("");
			THEN( "IsEmptyText() returns false." ) {
				CHECK_FALSE( IsEmptyText(vecT) );
			}
		}
	}
	
	GIVEN( "A catalog that is set \"nplurals=1; plural=0;\"" ) {
		// Only one form: Japanese, Vietnamese, Korean, Thai
		SetCatalog(R"(
msgid ""
msgstr ""
"Plural-Forms: nplurals=1; plural=0;"

msgid "abc"
msgid_plural "abcs"
msgstr[0] "0"
msgstr[1] "1"
msgstr[2] "2"
msgstr[3] "3"
msgstr[4] "4"
msgstr[5] "5"
)");
		WHEN( "nT() is called" ) {
			THEN( "nT() returns proper forms." ) {
				CHECK( nT("abc", "abcs", 0) == "0" );
				CHECK( nT("abc", "abcs", 1) == "0" );
				CHECK( nT("abc", "abcs", 2) == "0" );
				CHECK( nT("abc", "abcs", 3) == "0" );
			}
		}
	}
	
	GIVEN( "A catalog that is set \"nplurals=2; plural=n != 1;\"" ) {
		// Two forms, singular used for one only: English, German, Dutch, Swedish, Danish, Norwegian, Faroese, Spanish, Portuguese, Italian, Greek, Bulgarian, Finnish, Estonian, Hebrew, Bahasa Indonesian, Esperanto, Hungarian, Turkish
		SetCatalog(R"(
msgid ""
msgstr ""
"Plural-Forms: nplurals=2; plural=n != 1;"

msgid "abc"
msgid_plural "abcs"
msgstr[0] "0"
msgstr[1] "1"
msgstr[2] "2"
msgstr[3] "3"
msgstr[4] "4"
msgstr[5] "5"
)");
		WHEN( "nT() is called" ) {
			THEN( "nT() returns proper forms." ) {
				CHECK( nT("abc", "abcs", 0) == "1" );
				CHECK( nT("abc", "abcs", 1) == "0" );
				CHECK( nT("abc", "abcs", 2) == "1" );
				CHECK( nT("abc", "abcs", 3) == "1" );
			}
		}
	}
	
	GIVEN( "A catalog that is set \"nplurals=2; plural=n>1;\"" ) {
		// Two forms, singular used for zero and one: Brazilian Portuguese, French
		SetCatalog(R"(
msgid ""
msgstr ""
"Plural-Forms: nplurals=2; plural=n>1;"

msgid "abc"
msgid_plural "abcs"
msgstr[0] "0"
msgstr[1] "1"
msgstr[2] "2"
msgstr[3] "3"
msgstr[4] "4"
msgstr[5] "5"
)");
		WHEN( "nT() is called" ) {
			THEN( "nT() returns proper forms." ) {
				CHECK( nT("abc", "abcs", 0) == "0" );
				CHECK( nT("abc", "abcs", 1) == "0" );
				CHECK( nT("abc", "abcs", 2) == "1" );
				CHECK( nT("abc", "abcs", 3) == "1" );
			}
		}
	}
	
	GIVEN( "A catalog that is set \"nplurals=3; plural=n%10==1 && n%100!=11 ? 0 : n != 0 ? 1 : 2;\"" ) {
		// Three forms, special case for zero: Latvian
		SetCatalog(R"(
msgid ""
msgstr ""
"Plural-Forms: nplurals=3; plural=n%10==1 && n%100!=11 ? 0 : n != 0 ? 1 : 2;"

msgid "abc"
msgid_plural "abcs"
msgstr[0] "0"
msgstr[1] "1"
msgstr[2] "2"
msgstr[3] "3"
msgstr[4] "4"
msgstr[5] "5"
)");
		WHEN( "nT() is called" ) {
			THEN( "nT() returns proper forms." ) {
				CHECK( nT("abc", "abcs", 0) == "2" );
				CHECK( nT("abc", "abcs", 1) == "0" );
				CHECK( nT("abc", "abcs", 2) == "1" );
				CHECK( nT("abc", "abcs", 3) == "1" );
				CHECK( nT("abc", "abcs", 10) == "1" );
				CHECK( nT("abc", "abcs", 11) == "1" );
				CHECK( nT("abc", "abcs", 20) == "1" );
				CHECK( nT("abc", "abcs", 21) == "0" );
				CHECK( nT("abc", "abcs", 100) == "1" );
				CHECK( nT("abc", "abcs", 101) == "0" );
				CHECK( nT("abc", "abcs", 110) == "1" );
				CHECK( nT("abc", "abcs", 111) == "1" );
			}
		}
	}
	
	GIVEN( "A catalog that is set \"nplurals=3; plural=n==1 ? 0 : n==2 ? 1 : 2;\"" ) {
		// Three forms, special cases for one and two: Gaeilge (Irish)
		SetCatalog(R"(
msgid ""
msgstr ""
"Plural-Forms: nplurals=3; plural=n==1 ? 0 : n==2 ? 1 : 2;"

msgid "abc"
msgid_plural "abcs"
msgstr[0] "0"
msgstr[1] "1"
msgstr[2] "2"
msgstr[3] "3"
msgstr[4] "4"
msgstr[5] "5"
)");
		WHEN( "nT() is called" ) {
			THEN( "nT() returns proper forms." ) {
				CHECK( nT("abc", "abcs", 0) == "2" );
				CHECK( nT("abc", "abcs", 1) == "0" );
				CHECK( nT("abc", "abcs", 2) == "1" );
				CHECK( nT("abc", "abcs", 3) == "2" );
			}
		}
	}
	
	GIVEN( "A catalog that is set \"nplurals=3; plural=n==1 ? 0 : (n==0 || (n%100 > 0 && n%100 < 20)) ? 1 : 2;\"" ) {
		// Three forms, special case for numbers ending in 00 or [2-9][0-9]: Romanian
		SetCatalog(R"(
msgid ""
msgstr ""
"Plural-Forms: nplurals=3; plural=n==1 ? 0 : (n==0 || (n%100 > 0 && n%100 < 20)) ? 1 : 2;"

msgid "abc"
msgid_plural "abcs"
msgstr[0] "0"
msgstr[1] "1"
msgstr[2] "2"
msgstr[3] "3"
msgstr[4] "4"
msgstr[5] "5"
)");
		WHEN( "nT() is called" ) {
			THEN( "nT() returns proper forms." ) {
				CHECK( nT("abc", "abcs", 0) == "1" );
				CHECK( nT("abc", "abcs", 1) == "0" );
				CHECK( nT("abc", "abcs", 2) == "1" );
				CHECK( nT("abc", "abcs", 3) == "1" );
				CHECK( nT("abc", "abcs", 19) == "1" );
				CHECK( nT("abc", "abcs", 20) == "2" );
				CHECK( nT("abc", "abcs", 100) == "2" );
				CHECK( nT("abc", "abcs", 101) == "1" );
				CHECK( nT("abc", "abcs", 119) == "1" );
				CHECK( nT("abc", "abcs", 120) == "2" );
			}
		}
	}
	
	GIVEN( "A catalog that is set \"nplurals=3; plural=n%10==1 && n%100!=11 ? 0 : n%10>=2 && (n%100<10 || n%100>=20) ? 1 : 2;\"" ) {
		// Three forms, special case for numbers ending in 1[2-9]: Lithuanian
		SetCatalog(R"(
msgid ""
msgstr ""
"Plural-Forms: nplurals=3; plural=n%10==1 && n%100!=11 ? 0 : n%10>=2 && (n%100<10 || n%100>=20) ? 1 : 2;"

msgid "abc"
msgid_plural "abcs"
msgstr[0] "0"
msgstr[1] "1"
msgstr[2] "2"
msgstr[3] "3"
msgstr[4] "4"
msgstr[5] "5"
)");
		WHEN( "nT() is called" ) {
			THEN( "nT() returns proper forms." ) {
				CHECK( nT("abc", "abcs", 0) == "2" );
				CHECK( nT("abc", "abcs", 1) == "0" );
				CHECK( nT("abc", "abcs", 2) == "1" );
				CHECK( nT("abc", "abcs", 3) == "1" );
				CHECK( nT("abc", "abcs", 9) == "1" );
				CHECK( nT("abc", "abcs", 10) == "2" );
				CHECK( nT("abc", "abcs", 11) == "2" );
				CHECK( nT("abc", "abcs", 19) == "2" );
				CHECK( nT("abc", "abcs", 20) == "2" );
				CHECK( nT("abc", "abcs", 21) == "0" );
				CHECK( nT("abc", "abcs", 22) == "1" );
				CHECK( nT("abc", "abcs", 100) == "2" );
				CHECK( nT("abc", "abcs", 101) == "0" );
				CHECK( nT("abc", "abcs", 102) == "1" );
				CHECK( nT("abc", "abcs", 111) == "2" );
			}
		}
	}
	
	GIVEN( "A catalog that is set \"nplurals=3; plural=n%10==1 && n%100!=11 ? 0 : n%10>=2 && n%10<=4 && (n%100<10 || n%100>=20) ? 1 : 2;\"" ) {
		// Three forms, special cases for numbers ending in 1 and 2, 3, 4, except those ending in 1[1-4]: Russian, Ukrainian, Belarusian, Serbian, Croatian
		SetCatalog(R"(
msgid ""
msgstr ""
"Plural-Forms: nplurals=3; plural=n%10==1 && n%100!=11 ? 0 : n%10>=2 && n%10<=4 && (n%100<10 || n%100>=20) ? 1 : 2;"

msgid "abc"
msgid_plural "abcs"
msgstr[0] "0"
msgstr[1] "1"
msgstr[2] "2"
msgstr[3] "3"
msgstr[4] "4"
msgstr[5] "5"
)");
		WHEN( "nT() is called" ) {
			THEN( "nT() returns proper forms." ) {
				CHECK( nT("abc", "abcs", 0) == "2" );
				CHECK( nT("abc", "abcs", 1) == "0" );
				CHECK( nT("abc", "abcs", 2) == "1" );
				CHECK( nT("abc", "abcs", 3) == "1" );
				CHECK( nT("abc", "abcs", 4) == "1" );
				CHECK( nT("abc", "abcs", 5) == "2" );
				CHECK( nT("abc", "abcs", 9) == "2" );
				CHECK( nT("abc", "abcs", 10) == "2" );
				CHECK( nT("abc", "abcs", 11) == "2" );
				CHECK( nT("abc", "abcs", 12) == "2" );
				CHECK( nT("abc", "abcs", 19) == "2" );
				CHECK( nT("abc", "abcs", 20) == "2" );
				CHECK( nT("abc", "abcs", 21) == "0" );
				CHECK( nT("abc", "abcs", 22) == "1" );
				CHECK( nT("abc", "abcs", 100) == "2" );
				CHECK( nT("abc", "abcs", 101) == "0" );
				CHECK( nT("abc", "abcs", 102) == "1" );
				CHECK( nT("abc", "abcs", 111) == "2" );
			}
		}
	}
	
	GIVEN( "A catalog that is set \"nplurals=3; plural=(n==1) ? 0 : (n>=2 && n<=4) ? 1 : 2;\"" ) {
		// Three forms, special cases for 1 and 2, 3, 4: Czech, Slovak
		SetCatalog(R"(
msgid ""
msgstr ""
"Plural-Forms: nplurals=3; plural=(n==1) ? 0 : (n>=2 && n<=4) ? 1 : 2;"

msgid "abc"
msgid_plural "abcs"
msgstr[0] "0"
msgstr[1] "1"
msgstr[2] "2"
msgstr[3] "3"
msgstr[4] "4"
msgstr[5] "5"
)");
		WHEN( "nT() is called" ) {
			THEN( "nT() returns proper forms." ) {
				CHECK( nT("abc", "abcs", 0) == "2" );
				CHECK( nT("abc", "abcs", 1) == "0" );
				CHECK( nT("abc", "abcs", 2) == "1" );
				CHECK( nT("abc", "abcs", 3) == "1" );
				CHECK( nT("abc", "abcs", 4) == "1" );
				CHECK( nT("abc", "abcs", 5) == "2" );
			}
		}
	}
	
	GIVEN( "A catalog that is set \"nplurals=3; plural=n==1 ? 0 : n%10>=2 && n%10<=4 && (n%100<10 || n%100>=20) ? 1 : 2;\"" ) {
		// Three forms, special case for one and some numbers ending in 2, 3, or 4: Polish
		SetCatalog(R"(
msgid ""
msgstr ""
"Plural-Forms: nplurals=3; plural=n==1 ? 0 : n%10>=2 && n%10<=4 && (n%100<10 || n%100>=20) ? 1 : 2;"

msgid "abc"
msgid_plural "abcs"
msgstr[0] "0"
msgstr[1] "1"
msgstr[2] "2"
msgstr[3] "3"
msgstr[4] "4"
msgstr[5] "5"
)");
		WHEN( "nT() is called" ) {
			THEN( "nT() returns proper forms." ) {
				CHECK( nT("abc", "abcs", 0) == "2" );
				CHECK( nT("abc", "abcs", 1) == "0" );
				CHECK( nT("abc", "abcs", 2) == "1" );
				CHECK( nT("abc", "abcs", 3) == "1" );
				CHECK( nT("abc", "abcs", 4) == "1" );
				CHECK( nT("abc", "abcs", 5) == "2" );
				CHECK( nT("abc", "abcs", 10) == "2" );
				CHECK( nT("abc", "abcs", 11) == "2" );
				CHECK( nT("abc", "abcs", 12) == "2" );
				CHECK( nT("abc", "abcs", 14) == "2" );
				CHECK( nT("abc", "abcs", 15) == "2" );
				CHECK( nT("abc", "abcs", 20) == "2" );
				CHECK( nT("abc", "abcs", 21) == "2" );
				CHECK( nT("abc", "abcs", 22) == "1" );
				CHECK( nT("abc", "abcs", 24) == "1" );
				CHECK( nT("abc", "abcs", 25) == "2" );
				CHECK( nT("abc", "abcs", 100) == "2" );
				CHECK( nT("abc", "abcs", 101) == "2" );
				CHECK( nT("abc", "abcs", 102) == "1" );
				CHECK( nT("abc", "abcs", 104) == "1" );
				CHECK( nT("abc", "abcs", 105) == "2" );
			}
		}
	}
	
	GIVEN( "A catalog that is set \"Plural-Forms: nplurals=4; plural=n%100==1 ? 0 : n%100==2 ? 1 : n%100==3 || n%100==4 ? 2 : 3;\"" ) {
		// Four forms, special case for one and all numbers ending in 02, 03, or 04: Slovenian
		SetCatalog(R"(
msgid ""
msgstr ""
"Plural-Forms: Plural-Forms: nplurals=4; plural=n%100==1 ? 0 : n%100==2 ? 1 : n%100==3 || n%100==4 ? 2 : 3;"

msgid "abc"
msgid_plural "abcs"
msgstr[0] "0"
msgstr[1] "1"
msgstr[2] "2"
msgstr[3] "3"
msgstr[4] "4"
msgstr[5] "5"
)");
		WHEN( "nT() is called" ) {
			THEN( "nT() returns proper forms." ) {
				CHECK( nT("abc", "abcs", 0) == "3" );
				CHECK( nT("abc", "abcs", 1) == "0" );
				CHECK( nT("abc", "abcs", 2) == "1" );
				CHECK( nT("abc", "abcs", 3) == "2" );
				CHECK( nT("abc", "abcs", 4) == "2" );
				CHECK( nT("abc", "abcs", 5) == "3" );
				CHECK( nT("abc", "abcs", 10) == "3" );
				CHECK( nT("abc", "abcs", 11) == "3" );
				CHECK( nT("abc", "abcs", 12) == "3" );
				CHECK( nT("abc", "abcs", 13) == "3" );
				CHECK( nT("abc", "abcs", 14) == "3" );
				CHECK( nT("abc", "abcs", 15) == "3" );
				CHECK( nT("abc", "abcs", 100) == "3" );
				CHECK( nT("abc", "abcs", 101) == "0" );
				CHECK( nT("abc", "abcs", 102) == "1" );
				CHECK( nT("abc", "abcs", 103) == "2" );
				CHECK( nT("abc", "abcs", 104) == "2" );
				CHECK( nT("abc", "abcs", 105) == "3" );
			}
		}
	}
	
	GIVEN( "A catalog that is set \"Plural-Forms: nplurals=6; plural=n==0 ? 0 : n==1 ? 1 : n==2 ? 2 : n%100>=3 && n%100<=10 ? 3 : n%100>=11 ? 4 : 5;\"" ) {
		// Six forms, special cases for one, two, all numbers ending in 02, 03, … 10, all numbers ending in 11 … 99, and others: Arabic
		SetCatalog(R"(
msgid ""
msgstr ""
"Plural-Forms: Plural-Forms: nplurals=6; plural=n==0 ? 0 : n==1 ? 1 : n==2 ? 2 : n%100>=3 && n%100<=10 ? 3 : n%100>=11 ? 4 : 5;"

msgid "abc"
msgid_plural "abcs"
msgstr[0] "0"
msgstr[1] "1"
msgstr[2] "2"
msgstr[3] "3"
msgstr[4] "4"
msgstr[5] "5"
)");
		WHEN( "nT() is called" ) {
			THEN( "nT() returns proper forms." ) {
				CHECK( nT("abc", "abcs", 0) == "0" );
				CHECK( nT("abc", "abcs", 1) == "1" );
				CHECK( nT("abc", "abcs", 2) == "2" );
				CHECK( nT("abc", "abcs", 3) == "3" );
				CHECK( nT("abc", "abcs", 4) == "3" );
				CHECK( nT("abc", "abcs", 10) == "3" );
				CHECK( nT("abc", "abcs", 11) == "4" );
				CHECK( nT("abc", "abcs", 12) == "4" );
				CHECK( nT("abc", "abcs", 99) == "4" );
				CHECK( nT("abc", "abcs", 100) == "5" );
				CHECK( nT("abc", "abcs", 101) == "5" );
				CHECK( nT("abc", "abcs", 102) == "5" );
				CHECK( nT("abc", "abcs", 103) == "3" );
				CHECK( nT("abc", "abcs", 104) == "3" );
				CHECK( nT("abc", "abcs", 110) == "3" );
				CHECK( nT("abc", "abcs", 111) == "4" );
			}
		}
	}
}
// #endregion unit tests



} // test namespace
