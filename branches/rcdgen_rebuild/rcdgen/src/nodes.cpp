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
#include "fileio.h"

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
 * \fn GameBlock::Write(FileWriter *fw)
 * Write the block to the file.
 * @param fw File to write to.
 * @return Block number of this game block in the file.
 */


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

/**
 * Output the content to \a fw, for writing it to a file.
 * @param fw Store of file content.
 */
void FileNode::Write(FileWriter *fw)
{
	for (std::list<GameBlock *>::iterator iter = this->blocks.begin(); iter != this->blocks.end(); iter++) {
		(*iter)->Write(fw);
	}
}

FileNodeList::FileNodeList()
{
}

FileNodeList::~FileNodeList()
{
	for (std::list<FileNode *>::iterator iter = this->files.begin(); iter != this->files.end(); iter++) {
		delete *iter;
	}
}


SpriteBlock::SpriteBlock() : BlockNode()
{
}

SpriteBlock::~SpriteBlock()
{
}

/**
 * Write an 8PXL block.
 * @param fw File to write to.
 * @return Block number in the written file for this sprite.
 */
int SpriteBlock::Write(FileWriter *fw)
{
	if (this->sprite_image.data_size == 0) return 0; // Don't make empty sprites.

	FileBlock *fb = new FileBlock;
	int length = 4 * 2 + 4 * this->sprite_image.height + this->sprite_image.data_size;
	fb->StartSave("8PXL", 2, length);

	fb->SaveUInt16(this->sprite_image.width);
	fb->SaveUInt16(this->sprite_image.height);
	fb->SaveUInt16(this->sprite_image.xoffset);
	fb->SaveUInt16(this->sprite_image.yoffset);
	length = 4 * this->sprite_image.height;
	for (int i = 0; i < this->sprite_image.height; i++) {
		if (this->sprite_image.row_sizes[i] == 0) {
			fb->SaveUInt32(0);
		} else {
			fb->SaveUInt32(length);
			length += this->sprite_image.row_sizes[i];
		}
	}
	assert(length == 4 * this->sprite_image.height + this->sprite_image.data_size);
	fb->SaveBytes(this->sprite_image.data, this->sprite_image.data_size);
	fb->CheckEndSave();
	return fw->AddBlock(fb);
}

/**
 * Constructor of a sprite sheet.
 * @param line Line number of the sheet node.
 */
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

TSELBlock::TSELBlock() : GameBlock("TSEL", 1)
{
	for (int i = 0; i < SURFACE_COUNT; i++) {
		this->sprites[i] = NULL;
	}
}

/* virtual */ TSELBlock::~TSELBlock()
{
	for (int i = 0; i < SURFACE_COUNT; i++) {
		delete this->sprites[i];
	}
}

/* virtual */ int TSELBlock::Write(FileWriter *fw)
{
	FileBlock *fb = new FileBlock;
	fb->StartSave(this->blk_name, this->version, 92 - 12);
	fb->SaveUInt16(this->tile_width);
	fb->SaveUInt16(this->z_height);
	for (int i = 0; i < SURFACE_COUNT; i++) {
		fb->SaveUInt32(this->sprites[i]->Write(fw));
	}
	fb->CheckEndSave();
	return fw->AddBlock(fb);
}

TCORBlock::TCORBlock() : GameBlock("TCOR", 1)
{
	for (int i = 0; i < SURFACE_COUNT; i++) {
		this->north[i] = NULL;
		this->east[i] = NULL;
		this->south[i] = NULL;
		this->west[i] = NULL;
	}
}

/* virtual */ TCORBlock::~TCORBlock()
{
	for (int i = 0; i < SURFACE_COUNT; i++) {
		delete this->north[i];
		delete this->east[i];
		delete this->south[i];
		delete this->west[i];
	}
}

/* virtual */ int TCORBlock::Write(FileWriter *fw)
{
	FileBlock *fb = new FileBlock;
	fb->StartSave(this->blk_name, this->version, 320 - 12);
	fb->SaveUInt16(this->tile_width);
	fb->SaveUInt16(this->z_height);
	for (int i = 0; i < SURFACE_COUNT; i++) fb->SaveUInt32(this->north[i]->Write(fw));
	for (int i = 0; i < SURFACE_COUNT; i++) fb->SaveUInt32(this->east[i]->Write(fw));
	for (int i = 0; i < SURFACE_COUNT; i++) fb->SaveUInt32(this->south[i]->Write(fw));
	for (int i = 0; i < SURFACE_COUNT; i++) fb->SaveUInt32(this->west[i]->Write(fw));
	fb->CheckEndSave();
	return fw->AddBlock(fb);
}

SURFBlock::SURFBlock() : GameBlock("SURF", 3)
{
	for (int i = 0; i < SURFACE_COUNT; i++) this->sprites[i] = NULL;
}

SURFBlock::~SURFBlock()
{
	for (int i = 0; i < SURFACE_COUNT; i++) delete this->sprites[i];
}

/* virtual */ int SURFBlock::Write(FileWriter *fw)
{
	FileBlock *fb = new FileBlock;
	fb->StartSave(this->blk_name, this->version, 94 - 12);
	fb->SaveUInt16(this->surf_type);
	fb->SaveUInt16(this->tile_width);
	fb->SaveUInt16(this->z_height);
	for (int i = 0; i < SURFACE_COUNT; i++) {
		fb->SaveUInt32(this->sprites[i]->Write(fw));
	}
	fb->CheckEndSave();
	return fw->AddBlock(fb);
}

FUNDBlock::FUNDBlock() : GameBlock("FUND", 1)
{
	for (int i = 0; i < FOUNDATION_COUNT; i++) this->sprites[i] = NULL;
}

FUNDBlock::~FUNDBlock()
{
	for (int i = 0; i < FOUNDATION_COUNT; i++) delete this->sprites[i];
}

/* virtual */ int FUNDBlock::Write(FileWriter *fw)
{
	FileBlock *fb = new FileBlock;
	fb->StartSave(this->blk_name, this->version, 42 - 12);
	fb->SaveUInt16(this->found_type);
	fb->SaveUInt16(this->tile_width);
	fb->SaveUInt16(this->z_height);
	for (int i = 0; i < FOUNDATION_COUNT; i++) {
		fb->SaveUInt32(this->sprites[i]->Write(fw));
	}
	fb->CheckEndSave();
	return fw->AddBlock(fb);
}

Recolouring::Recolouring()
{
	this->orig = COLOUR_COUNT; // Invalid recolour by default.
	this->replace = 0;
}

Recolouring::~Recolouring()
{
}

/**
 * Encode a colour range remapping.
 * @return The encoded colour range remapping.
 */
uint32 Recolouring::Encode() const
{
	return (((uint32)this->orig) << 24) | (this->replace & 0xFFFFFF);
}

PersonGraphics::PersonGraphics() : BlockNode()
{
}

PersonGraphics::~PersonGraphics()
{
}

/**
 * Add a recolour mapping to the person graphics.
 * @param orig Colour range to replace.
 * @param replace Bitset of colour range that may be used instead.
 * @return Whether adding was successful.
 */
bool PersonGraphics::AddRecolour(uint8 orig, uint32 replace)
{
	if (orig >= COLOUR_COUNT || replace == 0) return true; // Invalid recolouring can always be stored.

	for (int i = 0; i < 3; i++) {
		if (this->recol[i].orig >= COLOUR_COUNT) {
			this->recol[i].orig = orig;
			this->recol[i].replace = replace;
			return true;
		}
	}
	return false;
}

PRSGBlock::PRSGBlock() : GameBlock("PRSG", 1)
{
}

PRSGBlock::~PRSGBlock()
{
}

/* virtual */ int PRSGBlock::Write(FileWriter *fw)
{
	FileBlock *fb = new FileBlock;
	fb->StartSave(this->blk_name, this->version, 1 + this->person_graphics.size() * 13);
	fb->SaveUInt8(this->person_graphics.size());
	for (std::list<PersonGraphics>::iterator iter = this->person_graphics.begin(); iter != this->person_graphics.end(); iter++) {
		const PersonGraphics &pg = *iter;
		fb->SaveUInt8(pg.person_type);
		fb->SaveUInt32(pg.recol[0].Encode());
		fb->SaveUInt32(pg.recol[1].Encode());
		fb->SaveUInt32(pg.recol[2].Encode());
	}
	fb->CheckEndSave();
	return fw->AddBlock(fb);
}

FrameData::FrameData() : BlockNode()
{
}

FrameData::~FrameData()
{
}

ANIMBlock::ANIMBlock() : GameBlock("ANIM", 2)
{
}

ANIMBlock::~ANIMBlock()
{
}

/* virtual */ int ANIMBlock::Write(FileWriter *fw)
{
	FileBlock *fb = new FileBlock;
	fb->StartSave(this->blk_name, this->version, 1 + 2 + 2 + this->frames.size() * 6);
	fb->SaveUInt8(this->person_type);
	fb->SaveUInt16(this->anim_type);
	fb->SaveUInt16(this->frames.size());
	for (std::list<FrameData>::iterator iter = this->frames.begin(); iter != this->frames.end(); iter++) {
		const FrameData &fd = *iter;
		fb->SaveUInt16(fd.duration);
		fb->SaveInt16(fd.change_x);
		fb->SaveInt16(fd.change_y);
	}
	fb->CheckEndSave();
	return fw->AddBlock(fb);
}

ANSPBlock::ANSPBlock() : GameBlock("ANSP", 1)
{
}

ANSPBlock::~ANSPBlock()
{
	for (std::list<SpriteBlock *>::iterator iter = this->frames.begin(); iter != this->frames.end(); iter++) {
		delete *iter;
	}
}

/* virtual */ int ANSPBlock::Write(FileWriter *fw)
{
	FileBlock *fb = new FileBlock;
	fb->StartSave(this->blk_name, this->version, 2 + 1 + 2 + 2 + this->frames.size() * 4);
	fb->SaveUInt16(this->tile_width);
	fb->SaveUInt8(this->person_type);
	fb->SaveUInt16(this->anim_type);
	fb->SaveUInt16(this->frames.size());
	for (std::list<SpriteBlock *>::iterator iter = this->frames.begin(); iter != this->frames.end(); iter++) {
		fb->SaveUInt32((*iter)->Write(fw));
	}
	fb->CheckEndSave();
	return fw->AddBlock(fb);
}

PATHBlock::PATHBlock() : GameBlock("PATH", 1)
{
	for (int i = 0; i < PTS_COUNT; i++) this->sprites[i] = NULL;
}

PATHBlock::~PATHBlock()
{
	for (int i = 0; i < PTS_COUNT; i++) delete this->sprites[i];
}

/* virtual */ int PATHBlock::Write(FileWriter *fw)
{
	FileBlock *fb = new FileBlock;
	fb->StartSave(this->blk_name, this->version, 2 + 2 + 2 + PTS_COUNT * 4);
	fb->SaveUInt16(this->path_type);
	fb->SaveUInt16(this->tile_width);
	fb->SaveUInt16(this->z_height);
	for (int i = 0; i < PTS_COUNT; i++) fb->SaveUInt32(this->sprites[i]->Write(fw));
	fb->CheckEndSave();
	return fw->AddBlock(fb);
}

PLATBlock::PLATBlock() : GameBlock("PLAT", 2)
{
	for (int i = 0; i < PLA_COUNT; i++) this->sprites[i] = NULL;
}

PLATBlock::~PLATBlock()
{
	for (int i = 0; i < PLA_COUNT; i++) delete this->sprites[i];
}

/* virtual */ int PLATBlock::Write(FileWriter *fw)
{
	FileBlock *fb = new FileBlock;
	fb->StartSave(this->blk_name, this->version, 2 + 2 + 2 + PLA_COUNT * 4);
	fb->SaveUInt16(this->tile_width);
	fb->SaveUInt16(this->z_height);
	fb->SaveUInt16(this->platform_type);
	for (int i = 0; i < PLA_COUNT; i++) fb->SaveUInt32(this->sprites[i]->Write(fw));
	fb->CheckEndSave();
	return fw->AddBlock(fb);
}

SUPPBlock::SUPPBlock() : GameBlock("SUPP", 1)
{
	for (int i = 0; i < SPP_COUNT; i++) this->sprites[i] = NULL;
}

SUPPBlock::~SUPPBlock()
{
	for (int i = 0; i < SPP_COUNT; i++) delete this->sprites[i];
}

/* virtual */ int SUPPBlock::Write(FileWriter *fw)
{
	FileBlock *fb = new FileBlock;
	fb->StartSave(this->blk_name, this->version, 2 + 2 + 2 + SPP_COUNT * 4);
	fb->SaveUInt16(this->support_type);
	fb->SaveUInt16(this->tile_width);
	fb->SaveUInt16(this->z_height);
	for (int i = 0; i < SPP_COUNT; i++) fb->SaveUInt32(this->sprites[i]->Write(fw));
	fb->CheckEndSave();
	return fw->AddBlock(fb);
}

/** Names of the known languages. */
static const char *_languages[] = {
	"",      // LNG_DEFAULT
	"en_GB", // LNG_EN_GB
	"nl_NL", // LNG_NL_NL
};

/**
 * Get the index of a language from its name.
 * @param lname Name of the language.
 * @param line Line number stating the name.
 * @return Index of the given language.
 */
int GetLanguageIndex(const char *lname, int line)
{
	assert(LNG_COUNT == sizeof(_languages) / sizeof(char *));
	for (int i = 0; i < LNG_COUNT; i++) {
		if (strcmp(_languages[i], lname) == 0) return i;
	}
	fprintf(stderr, "Error at line %d: Language \"%s\" is not known", line, lname);
	exit(1);
}

TextNode::TextNode() : BlockNode()
{
	for (int i = 0; i < LNG_COUNT; i++) this->lines[i] = -1;
}

/* virtual */ TextNode::~TextNode()
{
}

/**
 * Compute the number of bytes needed to store this text node in an RCD file.
 * @return Size needed to store this string.
 */
int TextNode::GetSize() const
{
	int length = 2 + 1 + this->name.size() + 1;
	for (int i = 0; i < LNG_COUNT; i++) {
		if (this->lines[i] < 0) continue;
		length += 2 + (1 + strlen(_languages[i]) + 1) + this->texts[i].size() + 1;
	}
	return length;
}

/**
 * Write the string and its translations into a file block.
 * @param fb File block to write to.
 */
void TextNode::Write(FileBlock *fb) const
{
	int length = this->GetSize();
	fb->SaveUInt16(length);
	length -= 2;

	assert(this->name.size() + 1 < 256);
	fb->SaveUInt8(this->name.size() + 1);
	fb->SaveBytes((uint8 *)this->name.c_str(), this->name.size());
	fb->SaveUInt8(0);
	length -= 1 + this->name.size() + 1;
	for (int i = 1; i < LNG_COUNT; i++) {
		if (this->lines[i] < 0) continue;
		int lname_length = strlen(_languages[i]);
		int lng_size = 2 + (1 + lname_length + 1) + this->texts[i].size() + 1;
		fb->SaveUInt16(lng_size);
		fb->SaveUInt8(lname_length + 1);
		fb->SaveBytes((uint8 *)_languages[i], lname_length);
		fb->SaveUInt8(0);
		length -= 2 + 1 + lname_length + 1;
		fb->SaveBytes((uint8 *)this->texts[i].c_str(), this->texts[i].size());
		fb->SaveUInt8(0);
		length -= this->texts[i].size() + 1;
	}
	assert(this->lines[0] >= 0);
	int lng_size = 2 + (1 + 0 + 1) + this->texts[0].size() + 1;
	fb->SaveUInt16(lng_size);
	fb->SaveUInt8(1);
	fb->SaveUInt8(0);
	length -= 2 + 1 + 1;
	fb->SaveBytes((uint8 *)this->texts[0].c_str(), this->texts[0].size());
	fb->SaveUInt8(0);
	length -= this->texts[0].size() + 1;
	assert(length == 0);
}

Strings::Strings() : BlockNode()
{
}

Strings::~Strings()
{
}

/**
 * Verify whether the strings are all valid.
 * @param names Expected string names.
 * @param name_count Number of names in \a names.
 * @param line Line number of the surrounding node (for error reporting).
 */
void Strings::CheckTranslations(const char *names[], int name_count, int line)
{
	/* Check that all necessary strings exist. */
	TextNode tn;
	for (int i = 0; i < name_count; i++) {
		tn.name = names[i];
		std::set<TextNode>::iterator iter = this->texts.find(tn);
		if (iter == this->texts.end()) {
			fprintf(stderr, "Error at line %d: String \"%s\" is not defined\n", line, names[i]);
			exit(1);
		}
	}
	/* Check that all strings have a default text. */
	for (std::set<TextNode>::const_iterator iter = this->texts.begin(); iter != this->texts.end(); iter++) {
		if ((*iter).lines[0] < 0) {
			fprintf(stderr, "Error at line %d: String \"%s\" has no default language text\n", line, (*iter).name.c_str());
			exit(1);
		}
	}
}

/**
 * Write the strings in a 'TEXT' block.
 * @param fw File to write to.
 * @return Block number where the data was saved.
 */
int Strings::Write(FileWriter *fw)
{
	FileBlock *fb = new FileBlock;
	int length = 0;
	for (std::set<TextNode>::const_iterator iter = this->texts.begin(); iter != this->texts.end(); iter++) {
		length += (*iter).GetSize();
	}
	fb->StartSave("TEXT", 1, length);

	for (std::set<TextNode>::const_iterator iter = this->texts.begin(); iter != this->texts.end(); iter++) {
		(*iter).Write(fb);
	}
	fb->CheckEndSave();
	return fw->AddBlock(fb);
}

SHOPBlock::SHOPBlock() : GameBlock("SHOP", 4)
{
	this->ne_view = NULL;
	this->se_view = NULL;
	this->sw_view = NULL;
	this->nw_view = NULL;
	this->shop_text = NULL;
}

/* virtual */ SHOPBlock::~SHOPBlock()
{
	delete this->ne_view;
	delete this->se_view;
	delete this->sw_view;
	delete this->nw_view;
	delete this->shop_text;
}

/* virtual */ int SHOPBlock::Write(FileWriter *fw)
{
	FileBlock *fb = new FileBlock;
	fb->StartSave(this->blk_name, this->version, 66 - 12);
	fb->SaveUInt16(this->tile_width);
	fb->SaveUInt8(this->height);
	fb->SaveUInt8(this->flags);
	fb->SaveUInt32(this->ne_view->Write(fw));
	fb->SaveUInt32(this->se_view->Write(fw));
	fb->SaveUInt32(this->sw_view->Write(fw));
	fb->SaveUInt32(this->nw_view->Write(fw));
	fb->SaveUInt32(this->recol[0].Encode());
	fb->SaveUInt32(this->recol[1].Encode());
	fb->SaveUInt32(this->recol[2].Encode());
	fb->SaveUInt32(this->item_cost[0]);
	fb->SaveUInt32(this->item_cost[1]);
	fb->SaveUInt32(this->ownership_cost);
	fb->SaveUInt32(this->opened_cost);
	fb->SaveUInt8(this->item_type[0]);
	fb->SaveUInt8(this->item_type[1]);
	fb->SaveUInt32(this->shop_text->Write(fw));
	fb->CheckEndSave();
	return fw->AddBlock(fb);
}

GBORBlock::GBORBlock() : GameBlock("GBOR", 1)
{
	this->tl = NULL;
	this->tm = NULL;
	this->tr = NULL;
	this->ml = NULL;
	this->mm = NULL;
	this->mr = NULL;
	this->bl = NULL;
	this->bm = NULL;
	this->br = NULL;
}

/* virtual */ GBORBlock::~GBORBlock()
{
	delete this->tl;
	delete this->tm;
	delete this->tr;
	delete this->ml;
	delete this->mm;
	delete this->mr;
	delete this->bl;
	delete this->bm;
	delete this->br;
}

/* virtual */ int GBORBlock::Write(FileWriter *fw)
{
	FileBlock *fb = new FileBlock;
	fb->StartSave(this->blk_name, this->version, 58 - 12);
	fb->SaveUInt16(this->widget_type);
	fb->SaveUInt8(this->border_top);
	fb->SaveUInt8(this->border_left);
	fb->SaveUInt8(this->border_right);
	fb->SaveUInt8(this->border_bottom);
	fb->SaveUInt8(this->min_width);
	fb->SaveUInt8(this->min_height);
	fb->SaveUInt8(this->h_stepsize);
	fb->SaveUInt8(this->v_stepsize);
	fb->SaveUInt32(this->tl->Write(fw));
	fb->SaveUInt32(this->tm->Write(fw));
	fb->SaveUInt32(this->tr->Write(fw));
	fb->SaveUInt32(this->ml->Write(fw));
	fb->SaveUInt32(this->mm->Write(fw));
	fb->SaveUInt32(this->mr->Write(fw));
	fb->SaveUInt32(this->bl->Write(fw));
	fb->SaveUInt32(this->bm->Write(fw));
	fb->SaveUInt32(this->br->Write(fw));
	fb->CheckEndSave();
	return fw->AddBlock(fb);
}

GCHKBlock::GCHKBlock() : GameBlock("GCHK", 1)
{
	this->empty = NULL;
	this->filled = NULL;
	this->empty_pressed = NULL;
	this->filled_pressed = NULL;
	this->shaded_empty = NULL;
	this->shaded_filled = NULL;
}

GCHKBlock::~GCHKBlock()
{
	delete this->empty;
	delete this->filled;
	delete this->empty_pressed;
	delete this->filled_pressed;
	delete this->shaded_empty;
	delete this->shaded_filled;
}

/* virtual */ int GCHKBlock::Write(FileWriter *fw)
{
	FileBlock *fb = new FileBlock;
	fb->StartSave(this->blk_name, this->version, 38 - 12);
	fb->SaveUInt16(this->widget_type);
	fb->SaveUInt32(this->empty->Write(fw));
	fb->SaveUInt32(this->filled->Write(fw));
	fb->SaveUInt32(this->empty_pressed->Write(fw));
	fb->SaveUInt32(this->filled_pressed->Write(fw));
	fb->SaveUInt32(this->shaded_empty->Write(fw));
	fb->SaveUInt32(this->shaded_filled->Write(fw));
	fb->CheckEndSave();
	return fw->AddBlock(fb);
}

GSLIBlock::GSLIBlock() : GameBlock("GSLI", 1)
{
	this->left = NULL;
	this->middle = NULL;
	this->right = NULL;
	this->slider = NULL;
}

GSLIBlock::~GSLIBlock()
{
	delete this->left;
	delete this->middle;
	delete this->right;
	delete this->slider;
}

/* virtual */ int GSLIBlock::Write(FileWriter *fw)
{
	FileBlock *fb = new FileBlock;
	fb->StartSave(this->blk_name, this->version, 33 - 12);
	fb->SaveUInt8(this->min_length);
	fb->SaveUInt8(this->step_size);
	fb->SaveUInt8(this->width);
	fb->SaveUInt16(this->widget_type);
	fb->SaveUInt32(this->left->Write(fw));
	fb->SaveUInt32(this->middle->Write(fw));
	fb->SaveUInt32(this->right->Write(fw));
	fb->SaveUInt32(this->slider->Write(fw));
	fb->CheckEndSave();
	return fw->AddBlock(fb);
}

GSCLBlock::GSCLBlock() : GameBlock("GSCL", 1)
{
	this->left_button = NULL;
	this->right_button = NULL;
	this->left_pressed = NULL;
	this->right_pressed = NULL;
	this->left_bottom = NULL;
	this->middle_bottom = NULL;
	this->right_bottom = NULL;
	this->left_top = NULL;
	this->middle_top = NULL;
	this->right_top = NULL;
	this->left_top_pressed = NULL;
	this->middle_top_pressed = NULL;
	this->right_top_pressed = NULL;
}

GSCLBlock::~GSCLBlock()
{
	delete this->left_button;
	delete this->right_button;
	delete this->left_pressed;
	delete this->right_pressed;
	delete this->left_bottom;
	delete this->middle_bottom;
	delete this->right_bottom;
	delete this->left_top;
	delete this->middle_top;
	delete this->right_top;
	delete this->left_top_pressed;
	delete this->middle_top_pressed;
	delete this->right_top_pressed;
}

/* virtual */ int GSCLBlock::Write(FileWriter *fw)
{
	FileBlock *fb = new FileBlock;
	fb->StartSave(this->blk_name, this->version, 70 - 12);
	fb->SaveUInt8(this->min_length);
	fb->SaveUInt8(this->step_back);
	fb->SaveUInt8(this->min_bar_length);
	fb->SaveUInt8(this->bar_step);
	fb->SaveUInt16(this->widget_type);
	fb->SaveUInt32(this->left_button->Write(fw));
	fb->SaveUInt32(this->right_button->Write(fw));
	fb->SaveUInt32(this->left_pressed->Write(fw));
	fb->SaveUInt32(this->right_pressed->Write(fw));
	fb->SaveUInt32(this->left_bottom->Write(fw));
	fb->SaveUInt32(this->middle_bottom->Write(fw));
	fb->SaveUInt32(this->right_bottom->Write(fw));
	fb->SaveUInt32(this->left_top->Write(fw));
	fb->SaveUInt32(this->middle_top->Write(fw));
	fb->SaveUInt32(this->right_top->Write(fw));
	fb->SaveUInt32(this->left_top_pressed->Write(fw));
	fb->SaveUInt32(this->middle_top_pressed->Write(fw));
	fb->SaveUInt32(this->right_top_pressed->Write(fw));
	fb->CheckEndSave();
	return fw->AddBlock(fb);
}

BDIRBlock::BDIRBlock() : GameBlock("BDIR", 1)
{
	this->sprite_ne = NULL;
	this->sprite_se = NULL;
	this->sprite_sw = NULL;
	this->sprite_nw = NULL;
}

BDIRBlock::~BDIRBlock()
{
	delete this->sprite_ne;
	delete this->sprite_se;
	delete this->sprite_sw;
	delete this->sprite_nw;
}

/* virtual */ int BDIRBlock::Write(FileWriter *fw)
{
	FileBlock *fb = new FileBlock;
	fb->StartSave(this->blk_name, this->version, 30 - 12);
	fb->SaveUInt16(this->tile_width);
	fb->SaveUInt32(this->sprite_ne->Write(fw));
	fb->SaveUInt32(this->sprite_se->Write(fw));
	fb->SaveUInt32(this->sprite_sw->Write(fw));
	fb->SaveUInt32(this->sprite_nw->Write(fw));
	fb->CheckEndSave();
	return fw->AddBlock(fb);
}

GSLPBlock::GSLPBlock() : GameBlock("GSLP", 4)
{
	this->vert_down = NULL;
	this->steep_down = NULL;
	this->gentle_down = NULL;
	this->level = NULL;
	this->gentle_up = NULL;
	this->steep_up = NULL;
	this->vert_up = NULL;
	this->pos_2d = NULL;
	this->neg_2d = NULL;
	this->pos_3d = NULL;
	this->neg_3d = NULL;
	this->close_button = NULL;
	this->terraform_dot = NULL;
	this->gui_text = NULL;
}

GSLPBlock::~GSLPBlock()
{
	delete this->vert_down;
	delete this->steep_down;
	delete this->gentle_down;
	delete this->level;
	delete this->gentle_up;
	delete this->steep_up;
	delete this->vert_up;
	delete this->pos_2d;
	delete this->neg_2d;
	delete this->pos_3d;
	delete this->neg_3d;
	delete this->close_button;
	delete this->terraform_dot;
	delete this->gui_text;
}

int GSLPBlock::Write(FileWriter *fw)
{
	FileBlock *fb = new FileBlock;
	fb->StartSave(this->blk_name, this->version, 68 - 12);
	fb->SaveUInt32(this->vert_down->Write(fw));
	fb->SaveUInt32(this->steep_down->Write(fw));
	fb->SaveUInt32(this->gentle_down->Write(fw));
	fb->SaveUInt32(this->level->Write(fw));
	fb->SaveUInt32(this->gentle_up->Write(fw));
	fb->SaveUInt32(this->steep_up->Write(fw));
	fb->SaveUInt32(this->vert_up->Write(fw));
	fb->SaveUInt32(this->pos_2d->Write(fw));
	fb->SaveUInt32(this->neg_2d->Write(fw));
	fb->SaveUInt32(this->pos_3d->Write(fw));
	fb->SaveUInt32(this->neg_3d->Write(fw));
	fb->SaveUInt32(this->close_button->Write(fw));
	fb->SaveUInt32(this->terraform_dot->Write(fw));
	fb->SaveUInt32(this->gui_text->Write(fw));
	fb->CheckEndSave();
	return fw->AddBlock(fb);
}
