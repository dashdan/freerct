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
#include "ast.h"
#include "nodes.h"
#include "fileio.h"

/**
 * The main program of rcdgen.
 * @param argc Number of argument given to the program.
 * @param argv Argument texts.
 * @return Exit code.
 */
int main(int argc, const char *argv[])
{
	if (argc > 1 && (strcmp(argv[1], "--help") == 0 || strcmp(argv[1], "-h") == 0)) {
		printf("RCD file generator. Usage: rcdgen [-h | --help] file\n");
		exit(0);
	}

	if (argc > 2) {
		fprintf(stderr, "Error: Too many arguments (use -h or --help for online help)\n");
		exit(1);
	}

	FILE *infile = NULL;
	if (argc == 2) {
		infile = fopen(argv[1], "rb");
		if (infile == NULL) {
			printf("Error: Could not open file \"%s\"\n", argv[1]);
			exit(1);
		}
	}

	/* Phase 1: Parse the input file. */
	if (infile != NULL) yyin = infile;

	_parsed_data = NULL;
	yyparse();

	if (infile != NULL) fclose(infile);

	if (_parsed_data == NULL) {
		fprintf(stderr, "Parsing of the input file failed");
		exit(1);
	}

	/* Phase 2: Check and simplify the loaded input. */
	FileNodeList *file_nodes = CheckTree(_parsed_data);
	delete _parsed_data;

	/* Phase 3: Construct output files. */
	for (std::list<FileNode *>::iterator iter = file_nodes->files.begin(); iter != file_nodes->files.end(); iter++) {
		FileWriter fw;
		(*iter)->Write(&fw);
		fw.WriteFile((*iter)->file_name);
	}

	delete file_nodes;
	exit(0);
}
