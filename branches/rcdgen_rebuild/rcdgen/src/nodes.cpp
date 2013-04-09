/* $Id$ */

/*
 * This file is part of FreeRCT.
 * FreeRCT is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * FreeRCT is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with FreeRCT. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file nodes.cpp Code of the RCD file nodes. */

#include "stdafx.h"
#include "nodes.h"

BlockNode::BlockNode()
{
}

BlockNode::~BlockNode()
{
}

/**
 * Get a subnode for the given \a row and \a col.
 * @param row Row of the sub node.
 * @param col Column of the sub node.
 * @param name Variable name to receive the sub node (for error reporting only).
 * @param line Line number containing the variable name (for error reporting only).
 * @return The requested sub node.
 * @note The default implementation always fails, override to get the feature.
 */
/* virtual */ BlockNode *BlockNode::GetSubNode(int row, int col, char *name, int line)
{
	/* By default, fail. */
	fprintf(stderr, "Error at line %d: Cannot assign sub node (row=%d, column=%d) to variable \"%s\"\n", line, row, col, name);
	exit(1);
}

/**
 * Constructor of a game block.
 * @param blk_name Name of the block (4 letters string).
 * @param version Version number of the block.
 */
GameBlock::GameBlock(const char *blk_name, int version) : BlockNode()
{
	this->blk_name = blk_name;
	this->version = version;
}

GameBlock::~GameBlock()
{
}

/**
 * Constructor of the file node.
 * @param file_name Name of the file to write to.
 */
FileNode::FileNode(char *file_name) : BlockNode()
{
	this->file_name = file_name;
}

FileNode::~FileNode()
{
	free(this->file_name);
	for (std::list<GameBlock *>::iterator iter = this->blocks.begin(); iter != this->blocks.end(); iter++) {
		delete *iter;
	}
}

SpriteBlock::SpriteBlock() : BlockNode()
{
}

SpriteBlock::~SpriteBlock()
{
}

SheetBlock::SheetBlock(int line)
{
	this->line = line;
	this->img_sheet = NULL;
}

SheetBlock::~SheetBlock()
{
	delete this->img_sheet;
}

/**
 * Get the sprite sheet. Loads the sheet from the disk on the first call.
 * @return The loaded image.
 */
Image *SheetBlock::GetSheet()
{
	if (this->img_sheet != NULL) return this->img_sheet;

	this->img_sheet = new Image;
	const char *err = this->img_sheet->LoadFile(file.c_str());
	if (err != NULL) {
		fprintf(stderr, "Error at line %d, loading of the sheet-image failed: %s\n", this->line, err);
		exit(1);
	}
	return this->img_sheet;
}

/* virtual */ BlockNode *SheetBlock::GetSubNode(int row, int col, char *name, int line)
{
	Image *img = this->GetSheet();
	SpriteBlock *spr_blk = new SpriteBlock;
	const char *err = spr_blk->sprite_image.CopySprite(img, this->x_offset, this->y_offset,
			this->x_base + this->x_step * col, this->y_base + this->y_step * row, this->width, this->height);
	if (err != NULL) {
		fprintf(stderr, "Error at line %d, loading of the sprite for \"%s\" failed: %s\n", line, name, err);
		exit(1);
	}
	return spr_blk;
}

/**
 * Constructor of a TSEL block.
 * @param version Version number of the block.
 */
TSELBlock::TSELBlock(int version) : GameBlock("TSEL", version)
{
}

TSELBlock::~TSELBlock()
{
	for (int i = 0; i < SURFACE_COUNT; i++) {
		delete this->sprites[i];
	}
}

