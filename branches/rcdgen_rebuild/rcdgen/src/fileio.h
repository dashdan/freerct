/* $Id$ */

/*
 * This file is part of FreeRCT.
 * FreeRCT is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * FreeRCT is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with FreeRCT. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file fileio.h File IO declarations. */

#ifndef FILEIO_H
#define FILEIO_H

#include <list>

/** A block in an RCD file. */
class FileBlock {
public:
	FileBlock();
	~FileBlock();

	void StartSave(const char *blk_name, int version, int data_length);
	void SaveUInt8(uint8 d);
	void SaveUInt16(uint16 d);
	void SaveUInt32(uint32 d);
	void SaveBytes(uint8 *data, int size);
	void CheckEndSave();

	void Write(FILE *fp);

	uint8 *data;    ///< Data of the block.
	int length;     ///< Length of the block.
	int save_index; ///< Index in #data to write content into the file block.
};

bool operator==(const FileBlock &fb1, const FileBlock &fb2);


typedef std::list<FileBlock *> FileBlockPtrList;

/** RCD output file. */
class FileWriter {
public:
	FileWriter();
	~FileWriter();

	int AddBlock(FileBlock *fb);

	void WriteFile(const char *fname);

private:
	FileBlockPtrList blocks; ///< Blocks stored in the file so far.
};

#endif
