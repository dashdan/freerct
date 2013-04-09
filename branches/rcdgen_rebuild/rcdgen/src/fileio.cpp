/* $Id$ */

/*
 * This file is part of FreeRCT.
 * FreeRCT is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * FreeRCT is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with FreeRCT. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file fileio.cpp File IO code. */

#include "stdafx.h"
#include "fileio.h"


FileBlock::FileBlock()
{
	this->data = NULL;
	this->length = 0;
}

FileBlock::~FileBlock()
{
	free(this->data);
}

/**
 * Write the file block to the output.
 * @param fp File handle to write to.
 */
void FileBlock::Write(FILE *fp)
{
	if (this->length == 0) return;

	if ((int)fwrite(this->data, 1, this->length, fp) != this->length) {
		fprintf(stderr, "Failed to write RCD block\n");
		exit(1);
	}
}

/**
 * Check whether two file blocks are identical.
 * @param fb1 First block to compare.
 * @param fb2 Second block to compare.
 * @return Whether both blocks are the same.
 */
bool operator==(const FileBlock &fb1, const FileBlock &fb2)
{
	if (fb1.length != fb2.length) return false;
	if (fb1.length == 0) return true;
	return memcmp(fb1.data, fb2.data, fb1.length) == 0;
}

FileWriter::FileWriter()
{
}

FileWriter::~FileWriter()
{
	for (FileBlockPtrList::iterator iter = this->blocks.begin(); iter != this->blocks.end(); iter++) {
		delete *iter;
	}
}

/**
 * Add a block to the file.
 * @param blk Block to add.
 * @return Block index number where the block is stored in the file.
 */
int FileWriter::AddBlock(FileBlock *blk)
{
	int idx = 1;
	FileBlockPtrList::iterator iter = this->blocks.begin();
	while (iter != this->blocks.end()) {
		if (*(*iter) == *blk) { // Block already added, just return the old block number.
			delete blk;
			return idx;
		}
		idx++;
		iter++;
	}
	this->blocks.push_back(blk);
	return idx;
}

/**
 * Write all blocks of the RCD file to the file.
 * @param fname Filename to create.
 */
void FileWriter::WriteFile(const char *fname)
{
	FILE *fp = fopen(fname, "wb");
	if (fp == NULL) {
		fprintf(stderr, "Failed to open \"%s\" for writing.", fname);
		exit(1);
	}

	static const uint8 file_header[8] = {'R', 'C', 'D', 'F', 1, 0, 0, 0};
	if (fwrite(file_header, 1, 8, fp) != 8) {
		fprintf(stderr, "Failed to write the RCD file header of \"%s\".", fname);
		exit(1);
	}

	for (FileBlockPtrList::iterator iter = this->blocks.begin(); iter != this->blocks.end(); iter++) {
		(*iter)->Write(fp);
	}
	fclose(fp);
}
