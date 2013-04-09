/* $Id$ */

/*
 * This file is part of FreeRCT.
 * FreeRCT is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * FreeRCT is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with FreeRCT. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file main.cpp Main program. */

#include "stdafx.h"
#include "image.h"

int main()
{
	Image img;

	const char *result = img.LoadFile("../../sprites_src/tracks/track/track1x1basic_template8bpp64.png");
	if (result != NULL) {
		fprintf(stderr, "ERROR: %s\n", result);
		exit(1);
	}

	printf("Image is %d x %d\n", img.GetWidth(), img.GetHeight());
	exit(0);
}
