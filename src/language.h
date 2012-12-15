/* $Id$ */

/*
 * This file is part of FreeRCT.
 * FreeRCT is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * FreeRCT is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with FreeRCT. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file language.h %Language support. */

#ifndef LANGUAGE_H
#define LANGUAGE_H


class TextData;
class Money;
class Date;

extern int _current_language;

/**
 * Table of string-parts in the game.
 *
 * The largest part of the string space is allocated for 'simple' strings that exist only once, mostly in the GUI.
 *
 * For rides, such as shops, this does not work since there are several shop types loaded.
 * Instead, such strings are allocated at StringTable::STR_GENERIC_SHOP_START, while the
 * strings of each type are elsewhere created. By using the
 * ShopType::GetString, the real string number of the queried shop type is returned.
 */
enum StringTable {
	STR_NULL = 0,  ///< \c NULL string.
	STR_EMPTY = 1, ///< Empty string.

	STR_GUI_START, ///< Start of the GUI strings.

	/* After the gui strings come the other registered strings. */

	STR_END_FREE_SPACE = 0xF800,
	/**
	 * Generic shop strings, translated to 'real' string numbers by each shop type object by means of the ShopType::GetString function.
	 */
	STR_GENERIC_SHOP_START = STR_END_FREE_SPACE,

	STR_GENERIC_END = 0xFFFF,

	STR_INVALID = STR_GENERIC_END, ///< Invalid string.
};

#include "table/gui_strings.h"
#include "table/shops_strings.h"

typedef uint16 StringID; ///< Type of a string value.

/** Languages known by the program. */
enum Languages {
	DEFAULT_LANGUAGE = 0, ///< Id of the default language.

	LANGUAGE_COUNT = 3,   ///< Number of available languages.
};

/**
 * A string with its name and its translations. Memory of the text is not owned by the class.
 */
class TextString {
public:
	TextString();

	void Clear();

	/**
	 * Get the string in the currently selected language.
	 * @return Text of this string in the currently selected language.
	 */
	const uint8 *GetString() const
	{
		if (_current_language < 0 || _current_language >= LANGUAGE_COUNT) return (uint8 *)"<out of bounds>";
		if (this->languages[_current_language] != NULL) return this->languages[_current_language];
		if (this->languages[0] != NULL) return this->languages[0];
		return (uint8 *)"<no-text>";
	}

	const char *name;                      ///< Name of the string.
	const uint8 *languages[LANGUAGE_COUNT]; ///< The string in all languages.
};

/** Types of parameters for string parameters. */
enum StringParamType {
	SPT_NONE,   ///< Parameter contains nothing (and should not be used thus).
	SPT_STRID,  ///< Parameter is another #StringID.
	SPT_NUMBER, ///< Parameter is a number.
	SPT_MONEY,  ///< Parameter is an amount of money.
	SPT_DATE,   ///< Parameter is a date.
	SPT_UINT8,  ///< Parameter is a C text string.
};

/** Data of one string parameter. */
struct StringParameterData {
	uint8 parm_type; ///< Type of the parameter. @see StringParamType
	union {
		StringID str; ///< String number.
		uint8 *text;  ///< C text pointer. Memory  is not managed!
		uint32 dmy;   ///< Day/month/year.
		int64 number; ///< Signed number or money amount.
	} u; ///< Data of the parameter.
};

/** All string parameters. */
struct StringParameters {
	StringParameters();

	void SetNone(int num);
	void SetStrID(int num, StringID strid);
	void SetNumber(int num, int64 number);
	void SetMoney(int num, const Money &amount);
	void SetDate(int num, const Date &date);
	void SetUint8(int num, uint8 *text);

	void Clear();

	bool set_mode; ///< When not in set-mode, all parameters are cleared on first use of a Set function.
	StringParameterData parms[16]; ///< Parameters of the string, arbitrary limit.
};

/**
 * Class for retrieving language strings.
 * @todo Implement me.
 */
class Language {
public:
	Language();
	~Language();

	void Clear();

	uint16 RegisterStrings(const TextData &td, const char * const names[], uint16 base=STR_GENERIC_END);

	const uint8 *GetText(StringID number);
private:
	/** Registered strings. Entries may be \c NULL for unregistered or non-existing strings. */
	const TextString *registered[2048]; // Arbitrary size.
	uint first_free; ///< 'First' string index that is not allocated yet.
};

int GetLanguageIndex(const char *lang_name);
void InitLanguage();
void UninitLanguage();

StringID GetMonthName(int month = 0);
void GetTextSize(StringID num, int *width, int *height);

extern Language _language;
extern StringParameters _str_params;

void DrawText(StringID num, uint8 *buffer, uint length, StringParameters *params = &_str_params);

#endif
