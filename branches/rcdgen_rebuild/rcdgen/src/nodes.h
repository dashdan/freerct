/* $Id$ */

/*
 * This file is part of FreeRCT.
 * FreeRCT is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * FreeRCT is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with FreeRCT. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file nodes.h Declarations of the RCD nodes. */

#ifndef NODES_H
#define NODES_H

#include <string>
#include <list>
#include "image.h"

/** Base class for all nodes. */
class BlockNode {
public:
	BlockNode();
	virtual ~BlockNode();

	virtual BlockNode *GetSubNode(int row, int col, char *name, int line);
};

/** Base class for game blocks. */
class GameBlock : public BlockNode {
public:
	GameBlock(const char *blk_name, int version);
	/* virtual */ ~GameBlock();

	const char *blk_name; ///< Name of the block.
	int version;          ///< Version of the block.
};

/** Node representing an RCD file. */
class FileNode : public BlockNode {
public:
	FileNode(char *file_name);
	/* virtual */ ~FileNode();

	char *file_name;              ///< Name of the RCD file.
	std::list<GameBlock *>blocks; ///< Blocks of the file.
};

/** Sprites of a surface. */
enum SurfaceSprites {
	SF_FLAT,       ///< Flat surface tile.
	SF_N,          ///< North corner up.
	SF_E,          ///< East corner up.
	SF_NE,         ///< North, east corners up.
	SF_S,          ///< South corner up.
	SF_NS,         ///< North, south corners up.
	SF_ES,         ///< East, south corners up.
	SF_NES,        ///< North, east, south corners up.
	SF_W,          ///< West corner up.
	SF_WN,         ///< West, north corners up.
	SF_WE,         ///< West, east corners up.
	SF_WNE,        ///< West, north, east corners up.
	SF_WS,         ///< West, south corners up.
	SF_WNS,        ///< West, north, south corners up.
	SF_WES,        ///< West, east, south corners up.
	SF_STEEP_N,    ///< Steep north slope.
	SF_STEEP_E,    ///< Steep east slope.
	SF_STEEP_S,    ///< Steep south slope.
	SF_STEEP_W,    ///< Steep west slope.
	SURFACE_COUNT, ///< Number of tiles in a surface.
};

/** Block containing a sprite. */
class SpriteBlock : public BlockNode {
public:
	SpriteBlock();
	/* virtual */ ~SpriteBlock();

	SpriteImage sprite_image; ///< The stored sprite.
};

/** Block containing a sprite sheet. */
class SheetBlock : public BlockNode {
public:
	SheetBlock(int line);
	/* virtual */ ~SheetBlock();


	/* virtual */ BlockNode *GetSubNode(int row, int col, char *name, int line);
	Image *GetSheet();

	int line;         ///< Line number defining the sheet.
	std::string file; ///< Name of the file containing the sprite sheet.
	int x_base;       ///< Horizontal base offset in the sheet.
	int y_base;       ///< Vertical base offset in the sheet.
	int x_step;       ///< Column step size.
	int y_step;       ///< Row step size.
	int x_offset;     ///< Sprite offset (from the origin to the left edge of the sprite).
	int y_offset;     ///< Sprite offset (from the origin to the top edge of the sprite).
	int width;        ///< Width of a sprite.
	int height;       ///< Height of a sprite.

	Image *img_sheet; ///< Sheet of images.
};

/** A 'TSEL' block. */
class TSELBlock : public GameBlock {
public:
	TSELBlock(int version);
	/* virtual */ ~TSELBlock();

	int tile_width; ///< Zoom-width of a tile of the surface.
	int z_height;   ///< Change in Z height (in pixels) when going up or down a tile level.
	SpriteBlock *sprites[SURFACE_COUNT]; ///< Surface tiles.
};


#endif

