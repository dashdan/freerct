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

SheetBlock::SheetBlock()
{
}

SheetBlock::~SheetBlock()
{
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

