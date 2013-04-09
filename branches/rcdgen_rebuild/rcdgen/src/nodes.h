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

class FileWriter;

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

	virtual int Write(FileWriter *fw) = 0;

	const char *blk_name; ///< %Name of the block.
	int version;          ///< Version of the block.
};

/** Node representing an RCD file. */
class FileNode : public BlockNode {
public:
	FileNode(char *file_name);
	/* virtual */ ~FileNode();

	void Write(FileWriter *fw);

	char *file_name;              ///< %Name of the RCD file.
	std::list<GameBlock *>blocks; ///< Blocks of the file.
};

/** A sequence of file nodes. */
class FileNodeList {
public:
	FileNodeList();
	~FileNodeList();

	std::list<FileNode *>files; ///< Output files.
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

	int Write(FileWriter *fw);

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
	std::string file; ///< %Name of the file containing the sprite sheet.
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
	TSELBlock();
	/* virtual */ ~TSELBlock();

	/* virtual */ int Write(FileWriter *fw);

	int tile_width; ///< Zoom-width of a tile of the surface.
	int z_height;   ///< Change in Z height (in pixels) when going up or down a tile level.
	SpriteBlock *sprites[SURFACE_COUNT]; ///< Surface tiles.
};

/** A 'TCOR' block. */
class TCORBlock : public GameBlock {
public:
	TCORBlock();
	/* virtual */ ~TCORBlock();

	/* virtual */ int Write(FileWriter *fw);

	int tile_width; ///< Zoom-width of a tile of the surface.
	int z_height;   ///< Change in Z height (in pixels) when going up or down a tile level.
	SpriteBlock *north[SURFACE_COUNT]; ///< Corner select tiles while viewing to north.
	SpriteBlock *east[SURFACE_COUNT];  ///< Corner select tiles while viewing to east.
	SpriteBlock *south[SURFACE_COUNT]; ///< Corner select tiles while viewing to south.
	SpriteBlock *west[SURFACE_COUNT];  ///< Corner select tiles while viewing to west.
};

/** Ground surface block SURF. */
class SURFBlock : public GameBlock {
public:
	SURFBlock();
	/* virtual */ ~SURFBlock();

	/* virtual */ int Write(FileWriter *fw);

	int surf_type;  ///< Type of surface.
	int tile_width; ///< Zoom-width of a tile of the surface.
	int z_height;   ///< Change in Z height (in pixels) when going up or down a tile level.
	SpriteBlock *sprites[SURFACE_COUNT]; ///< Surface tiles.
};

/** Sprites of a foundation. */
enum FoundationSprites {
	FND_SE_E0, ///< Vertical south-east foundation, east visible, south down.
	FND_SE_0S, ///< Vertical south-east foundation, east down, south visible.
	FND_SE_ES, ///< Vertical south-east foundation, east visible, south visible.
	FND_SW_S0, ///< Vertical south-west foundation, south visible, west down.
	FND_SW_0W, ///< Vertical south-west foundation, south down, west visible.
	FND_SW_SW, ///< Vertical south-west foundation, south visible, west visible.

	FOUNDATION_COUNT, ///< Number of foundation sprites.
};

/** Foundation game block FUND. */
class FUNDBlock : public GameBlock {
public:
	FUNDBlock();
	/* virtual */ ~FUNDBlock();

	/* virtual */ int Write(FileWriter *fw);

	int found_type; ///< Type of foundation.
	int tile_width; ///< Zoom-width of a tile of the surface.
	int z_height;   ///< Change in Z height (in pixels) when going up or down a tile level.
	SpriteBlock *sprites[FOUNDATION_COUNT]; ///< Foundation tiles.
};

/** Colour ranges. */
enum ColourRange {
	COL_GREY,        ///< Colour range for grey.
	COL_GREEN_BROWN, ///< Colour range for green_brown.
	COL_BROWN,       ///< Colour range for brown.
	COL_YELLOW,      ///< Colour range for yellow.
	COL_DARK_RED,    ///< Colour range for dark_red.
	COL_DARK_GREEN,  ///< Colour range for dark_green.
	COL_LIGHT_GREEN, ///< Colour range for light_green.
	COL_GREEN,       ///< Colour range for green.
	COL_LIGHT_RED,   ///< Colour range for light_red.
	COL_DARK_BLUE,   ///< Colour range for dark_blue.
	COL_BLUE,        ///< Colour range for blue.
	COL_LIGHT_BLUE,  ///< Colour range for light_blue.
	COL_PURPLE,      ///< Colour range for purple.
	COL_RED,         ///< Colour range for red.
	COL_ORANGE,      ///< Colour range for orange.
	COL_SEA_GREEN,   ///< Colour range for sea_green.
	COL_PINK,        ///< Colour range for pink.
	COL_BEIGE,       ///< Colour range for beige.

	COLOUR_COUNT,    ///< Number of colour ranges.
};

/** Colour range remapping definition. */
class Recolouring : public BlockNode {
public:
	Recolouring();
	/* virtual */ ~Recolouring();

	uint8 orig;     ///< Colour range to replace.
	uint32 replace; ///< Bitset of colour ranges that may be used as replacement.

	uint32 Encode() const;
};

/** Definition of graphics of one type of person. */
class PersonGraphics : public BlockNode {
public:
	PersonGraphics();
	/* virtual */ ~PersonGraphics();

	int person_type;      ///< Type of person being defined.
	Recolouring recol[3]; ///< Recolour definitions.

	bool AddRecolour(uint8 orig, uint32 replace);
};

/** Person graphics game block. */
class PRSGBlock : public GameBlock {
public:
	PRSGBlock();
	/* virtual */ ~PRSGBlock();

	/* virtual */ int Write(FileWriter *fw);

	std::list<PersonGraphics> person_graphics; ///< Stored person graphics.
};

/** ANIM frame data for a single frame. */
class FrameData : public BlockNode {
public:
	FrameData();
	/* virtual */ ~FrameData();

	int duration; ///< Duration of this frame.
	int change_x; ///< Change in x after the frame is displayed.
	int change_y; ///< Change in y after the frame is displayed.
};

/** ANIM game block. */
class ANIMBlock : public GameBlock {
public:
	ANIMBlock();
	/* virtual */ ~ANIMBlock();

	/* virtual */ int Write(FileWriter *fw);

	int person_type; ///< Type of person being defined.
	int anim_type;   ///< Type of animation being defined.

	std::list<FrameData> frames; ///< Frame data of every frame.
};

/** ANSP game block. */
class ANSPBlock : public GameBlock {
public:
	ANSPBlock();
	/* virtual */ ~ANSPBlock();

	/* virtual */ int Write(FileWriter *fw);

	int tile_width;  ///< Size of the tile.
	int person_type; ///< Type of person being defined.
	int anim_type;   ///< Type of animation being defined.

	std::list<SpriteBlock *> frames; ///< Sprite for every frame.
};

#endif

