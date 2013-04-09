/* $Id$ */

/*
 * This file is part of FreeRCT.
 * FreeRCT is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * FreeRCT is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with FreeRCT. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file main.cpp Main program. */

#include "stdafx.h"
#include "scanner_funcs.h"
#include "check_data.h"

int main()
{
	/* Phase 1: Parse the input file. */
	_parsed_data = NULL;
	yyparse();
	if (_parsed_data == NULL) {
		fprintf(stderr, "Parsing of the input file failed");
		exit(1);
	}

	/* Phase 2: Check and simplify the loaded input. */
	CheckTree(_parsed_data);

	exit(0);
}
