/* $Id$ */

/*
 * This file is part of FreeRCT.
 * FreeRCT is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * FreeRCT is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with FreeRCT. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file check_data.cpp Check and simplify functions. */

#include "stdafx.h"
#include <map>
#include <string>
#include "ast.h"
#include "check_data.h"
#include "nodes.h"

static BlockNode *ConvertNodeGroup(NodeGroup *ng);

/**
 * Check the number of expressions given in \a exprs, and expand them into the \a out array for easier access.
 * @param exprs %Expression list containing parameters.
 * @param out [out] Output array for storing \a expected expressions from the list.
 * @param expected Expected number of expressions in \a exprs.
 * @param line Line number of the node (for reporting errors).
 * @param node %Name of the node being checked and expanded.
 */
static void ExpandExpressions(ExpressionList *exprs, Expression *out[], size_t expected, int line, const char *node)
{
	if (exprs == NULL) {
		if (expected == 0) return;
		fprintf(stderr, "Error at line %d: No arguments found for node \"%s\" (expected %lu)\n", line, node, expected);
		exit(1);
	}
	if (exprs->exprs.size() != expected) {
		fprintf(stderr, "Error at line %d: Found %lu arguments for node \"%s\", expected %lu\n", line, exprs->exprs.size(), node, expected);
		exit(1);
	}
	int idx = 0;
	for (std::list<Expression *>::iterator iter = exprs->exprs.begin(); iter != exprs->exprs.end(); iter++) {
		out[idx++] = *iter;
	}
}

/**
 * Check that there are no expressions provided in \a exprs. Give an error otherwise.
 * @param exprs %Expression list containing parameters.
 * @param line Line number of the node (for reporting errors).
 * @param node %Name of the node being checked and expanded.
 */
static void ExpandNoExpression(ExpressionList *exprs, int line, const char *node)
{
	if (exprs == NULL || exprs->exprs.size() == 0) return;

	fprintf(stderr, "Error at line %d: No arguments expected for node \"%s\" (found %lu)\n", line, node, exprs->exprs.size());
	exit(1);
}

/**
 * Extract a string from the given expression.
 * @param expr %Expression to evaluate.
 * @param index Parameter number (0-based, for error reporting).
 * @param node %Name of the node.
 * @return Value of the string (caller should release the memory after use).
 */
static char *GetString(Expression *expr, int index, const char *node)
{
	/* Simple case, expression is a string literal. */
	StringLiteral *sl = dynamic_cast<StringLiteral *>(expr);
	if (sl != NULL) return sl->CopyText();

	/* General case, compute its value. */
	Expression *expr2 = expr->Evaluate(NULL);
	sl = dynamic_cast<StringLiteral *>(expr2);
	if (sl == NULL) {
		fprintf(stderr, "Error at line %d: Expression parameter %d of node %s is not a string", expr->line, index + 1, node);
		exit(1);
	}
	char *result = sl->CopyText();
	delete expr2;
	return result;
}

/**
 * Extract a number from the given expression.
 * @param expr %Expression to evaluate.
 * @param index Parameter number (0-based, for error reporting).
 * @param node %Name of the node.
 * @param symbols Symbols available for use in the expression.
 * @return Value of the number.
 */
static long long GetNumber(Expression *expr, int index, const char *node, const Symbol *symbols = NULL)
{
	/* Simple case, expression is a number literal. */
	NumberLiteral *nl = dynamic_cast<NumberLiteral *>(expr);
	if (nl != NULL) return nl->value;

	/* General case, compute its value. */
	Expression *expr2 = expr->Evaluate(symbols);
	nl = dynamic_cast<NumberLiteral *>(expr2);
	if (nl == NULL) {
		fprintf(stderr, "Error at line %d: Expression parameter %d of node %s is not a number", expr->line, index + 1, node);
		exit(1);
	}
	long long value = nl->value;
	delete expr2;
	return value;
}

/**
 * Convert a 'file' node (taking a string parameter for the filename, and a sequence of game blocks).
 * @param ng Generic tree of nodes to convert to a 'file' node.
 * @return The converted file node.
 */
static FileNode *ConvertFileNode(NodeGroup *ng)
{
	Expression *argument;
	ExpandExpressions(ng->exprs, &argument, 1, ng->line, "file");

	char *filename = GetString(argument, 0, "file");
	FileNode *fn = new FileNode(filename);

	for (std::list<NamedValue *>::iterator iter = ng->values->values.begin(); iter != ng->values->values.end(); iter++) {
		NamedValue *nv = *iter;
		if (nv->name != NULL) fprintf(stderr, "Warning at line %d: Unexpected name encountered, ignoring\n", nv->name->GetLine());
		NodeGroup *ng = nv->group->CastToNodeGroup();
		if (ng == NULL) {
			fprintf(stderr, "Error at line %d: Only node groups may be added\n", nv->group->GetLine());
			exit(1);
		}
		BlockNode *bn = ConvertNodeGroup(ng);
		GameBlock *gb = dynamic_cast<GameBlock *>(bn);
		if (gb == NULL) {
			fprintf(stderr, "Error at line %d: Only game blocks can be added to a \"file\" node\n", nv->group->GetLine());
			exit(1);
		}
		fn->blocks.push_back(gb);
	}
	return fn;
}

/** All information that needs to be stored about a named value. */
class ValueInformation {
public:
	ValueInformation();
	ValueInformation(const std::string &name, int line);
	ValueInformation(const ValueInformation &vi);
	ValueInformation &operator=(const ValueInformation &vi);
	~ValueInformation();

	long long GetNumber(int line, const char *node, const Symbol *symbols = NULL);
	std::string GetString(int line, const char *node);
	SpriteBlock *GetSprite(int line, const char *node);

	Expression *expr_value; ///< %Expression attached to it (if any).
	BlockNode *node_value;  ///< Node attached to it (if any).
	std::string name;       ///< %Name of the value.
	int line;               ///< Line number of the name.
	bool used;              ///< Is the value used?
};

/** Default constructor. */
ValueInformation::ValueInformation()
{
	this->expr_value = NULL;
	this->node_value = NULL;
	this->name = "_unknown_";
	this->line = 0;
	this->used = false;
}

/**
 * Constructor of the class.
 * @param name %Name of the value.
 * @param line Line number of the name.
 */
ValueInformation::ValueInformation(const std::string &name, int line)
{
	this->expr_value = NULL;
	this->node_value = NULL;
	this->name = name;
	this->line = line;
	this->used = false;
}

/**
 * Copy-constructor, moves ownership of #expr_value and #node_value to the assigned object.
 * @param vi Original object.
 */
ValueInformation::ValueInformation(const ValueInformation &vi)
{
	ValueInformation &w_vi = const_cast<ValueInformation &>(vi);
	this->expr_value = w_vi.expr_value;
	this->node_value = w_vi.node_value;
	w_vi.expr_value = NULL;
	w_vi.node_value = NULL;

	this->name = vi.name;
	this->line = vi.line;
	this->used = vi.used;
}

/**
 * Assignment operator, moves ownership of #expr_value and #node_value to the assigned object.
 * @param vi Original object.
 * @return Assigned object.
 */
ValueInformation &ValueInformation::operator=(const ValueInformation &vi)
{
	if (&vi != this) {
		ValueInformation &w_vi = const_cast<ValueInformation &>(vi);
		this->expr_value = w_vi.expr_value;
		this->node_value = w_vi.node_value;
		w_vi.expr_value = NULL;
		w_vi.node_value = NULL;

		this->name = vi.name;
		this->line = vi.line;
		this->used = vi.used;
	}
	return *this;
}

ValueInformation::~ValueInformation()
{
	delete this->expr_value;
	delete this->node_value;
}

/**
 * Extract a number from the given expression.
 * @param line Line number of the node (for reporting errors).
 * @param node %Name of the node.
 * @param symbols Symbols available for use in the expression.
 * @return Numeric value.
 */
long long ValueInformation::GetNumber(int line, const char *node, const Symbol *symbols)
{
	if (this->expr_value == NULL) {
fail:
		fprintf(stderr, "Error at line %d: Field \"%s\" of node \"%s\" is not a numeric value\n", line, this->name.c_str(), node);
		exit(1);
	}
	NumberLiteral *nl = dynamic_cast<NumberLiteral *>(this->expr_value); // Simple common case.
	if (nl != NULL) return nl->value;

	Expression *expr2 = this->expr_value->Evaluate(symbols); // Generic case, evaluate the expression.
	nl = dynamic_cast<NumberLiteral *>(expr2);
	if (nl == NULL) goto fail;

	long long value = nl->value;
	delete expr2;
	return value;
}

/**
 * Extract a string from the given expression.
 * @param line Line number of the node (for reporting errors).
 * @param node %Name of the node.
 * @return String value, should be freed by the user.
 */
std::string ValueInformation::GetString(int line, const char *node)
{
	if (this->expr_value == NULL) {
fail:
		fprintf(stderr, "Error at line %d: Field \"%s\" of node \"%s\" is not a string value\n", line, this->name.c_str(), node);
		exit(1);
	}
	StringLiteral *sl = dynamic_cast<StringLiteral *>(this->expr_value); // Simple common case.
	if (sl != NULL) return std::string(sl->text);

	Expression *expr2 = this->expr_value->Evaluate(NULL); // Generic case
	sl = dynamic_cast<StringLiteral *>(expr2);
	if (sl == NULL) goto fail;

	std::string result = std::string(sl->text);
	delete expr2;
	return result;
}

/**
 * Get a sprite (#SpriteBlock) from the given node value.
 * @param line Line number of the node (for reporting errors).
 * @param node %Name of the node.
 * @return The sprite.
 */
SpriteBlock *ValueInformation::GetSprite(int line, const char *node)
{
	SpriteBlock *sb = dynamic_cast<SpriteBlock *>(this->node_value);
	if (sb != NULL) {
		this->node_value = NULL;
		return sb;
	}
	fprintf(stderr, "Error at line %d: Field \"%s\" of node \"%s\" is not a sprite node\n", line, this->name.c_str(), node);
	exit(1);
}

/**
 * Find the value information named \a fld_name.
 * @param fld_name %Name of the field looking for.
 * @param vis Collected named values.
 * @param length Number of entries in \a vis.
 * @param line Line number of the node (for reporting errors).
 * @param node %Name of the node.
 * @return Reference in the \a vis array.
 */
static ValueInformation &FindValueInformation(const char *fld_name, ValueInformation *vis, int length, int line, const char *node)
{
	while (length > 0) {
		if (!vis->used && vis->name == fld_name) {
			vis->used = true;
			return *vis;
		}
		vis++;
		length--;
	}
	fprintf(stderr, "Error at line %d: Cannot find a value for field \"%s\" in node \"%s\"\n", line, fld_name, node);
	exit(1);
}

/**
 * Assign sub-nodes to the names of a 2D table.
 * @param bn Node to split in sub-nodes.
 * @param nt 2D name table.
 * @param vis Array being filled.
 * @param length [inout] Updated number of used entries in \a vis.
 */
void AssignNames(BlockNode *bn, NameTable *nt, ValueInformation *vis, int *length)
{
	int row = 0;
	for (std::list<NameRow *>::iterator row_iter = nt->rows.begin(); row_iter != nt->rows.end(); row_iter++) {
		NameRow *nr = *row_iter;
		int col = 0;
		for (std::list<IdentifierLine *>::iterator col_iter = nr->identifiers.begin(); col_iter != nr->identifiers.end(); col_iter++) {
			IdentifierLine *il = *col_iter;
			if (il->IsValid()) {
				vis[*length].expr_value = NULL;
				vis[*length].node_value = bn->GetSubNode(row, col, il->name, il->line);
				vis[*length].name = std::string(il->name);
				vis[*length].line = il->line;
				vis[*length].used = false;
				(*length)++;
			}
			col++;
		}
		row++;
	}
}

/**
 * Prepare the named values for access by field name.
 * @param values Named value to prepare.
 * @param length [out] Number of named values returned.
 * @param symbols Optional symbols that may be used in the values.
 * @return Collection named values available for use. Caller should release its memory.
 */
ValueInformation *PrepareNamedValues(NamedValueList *values, int *length, const Symbol *symbols = NULL)
{
	int total = 0;
	for (std::list<NamedValue *>::iterator iter = values->values.begin(); iter != values->values.end(); iter++) {
		/* Check availability of names. */
		if ((*iter)->name == NULL) {
			fprintf(stderr, "Error at line %d: Value should have a name\n", (*iter)->group->GetLine());
			exit(1);
		}
		int count = (*iter)->name->GetNameCount();
		total += count;
	}

	ValueInformation *vis = new ValueInformation[total];
	*length = 0;
	for (std::list<NamedValue *>::iterator iter = values->values.begin(); iter != values->values.end(); iter++) {
		/* Node group? */
		NodeGroup *ng = (*iter)->group->CastToNodeGroup();
		if (ng != NULL) {
			BlockNode *bn = ConvertNodeGroup(ng);
			SingleName *sn = dynamic_cast<SingleName *>((*iter)->name);
			if (sn != NULL) {
				vis[*length].expr_value = NULL;
				vis[*length].node_value = bn;
				vis[*length].name = std::string(sn->name);
				vis[*length].line = sn->line;
				vis[*length].used = false;
				(*length)++;
				continue;
			}
			NameTable *nt = dynamic_cast<NameTable *>((*iter)->name);
			assert(nt != NULL);
			AssignNames(bn, nt, vis, length);
			continue;
		}

		/* Expression group. */
		ExpressionGroup *eg = (*iter)->group->CastToExpressionGroup();
		assert(eg != NULL);
		SingleName *sn = dynamic_cast<SingleName *>((*iter)->name);
		if (sn == NULL) {
			fprintf(stderr, "Error at line %d: Expression must have a single name\n", (*iter)->name->GetLine());
			exit(1);
		}
		vis[*length].expr_value = eg->expr->Evaluate(symbols);
		vis[*length].node_value = NULL;
		vis[*length].name = std::string(sn->name);
		vis[*length].line = sn->line;
		vis[*length].used = false;
		(*length)++;
	}
	assert(*length == total);
	return vis;
}

/**
 * Verify whether all named values were used in a node.
 * @param vis Collected named values.
 * @param length Number of entries in \a vis.
 * @param node %Name of the node.
 */
static void VerifyNamedValuesUse(ValueInformation *vis, int length, const char *node)
{
	while (length > 0) {
		if (!vis->used) {
			fprintf(stderr, "Warning at line %d: Named value \"%s\" was not used in node \"%s\"\n", vis->line, vis->name.c_str(), node);
		}
		vis++;
		length--;
	}
}

/** Names of surface sprites in a single direction of view. */
static const char *_surface_sprite[] = {
	"#",    // SF_FLAT
	"#n",   // SF_N
	"#e",   // SF_E
	"#ne",  // SF_NE
	"#s",   // SF_S
	"#ns",  // SF_NS
	"#es",  // SF_ES
	"#nes", // SF_NES
	"#w",   // SF_W
	"#nw",  // SF_WN
	"#ew",  // SF_WE
	"#new", // SF_WNE
	"#sw",  // SF_WS
	"#nsw", // SF_WNS
	"#esw", // SF_WES
	"#N",   // SF_STEEP_N
	"#E",   // SF_STEEP_E
	"#S",   // SF_STEEP_S
	"#W",   // SF_STEEP_W
};

/**
 * Convert a node group to a TSEL game block.
 * @param ng Generic tree of nodes to convert.
 * @return The created TSEL game block.
 */
static TSELBlock *ConvertTSELNode(NodeGroup *ng)
{
	ExpandNoExpression(ng->exprs, ng->line, "TSEL");
	TSELBlock *blk = new TSELBlock;

	/* Prepare named values for access. */
	int length = 0;
	ValueInformation *vis = PrepareNamedValues(ng->values, &length);

	/* get the fields and their value. */
	blk->tile_width = FindValueInformation("tile_width", vis, length, ng->line, "TSEL").GetNumber(ng->line, "TSEL");
	blk->z_height   = FindValueInformation("z_height",   vis, length, ng->line, "TSEL").GetNumber(ng->line, "TSEL");

	char buffer[16];
	buffer[0] = 'n';
	for (int i = 0; i < SURFACE_COUNT; i++) {
		strcpy(buffer + 1, _surface_sprite[i]);
		blk->sprites[i] = FindValueInformation(buffer, vis, length, ng->line, "TSEL").GetSprite(ng->line, "TSEL");
	}

	VerifyNamedValuesUse(vis, length, "TSEL");
	delete[] vis;
	return blk;
}

/** Available types of surface. */
static const Symbol _surface_types[] = {
	{"reserved",      0},
	{"the_green",    16},
	{"short_grass",  17},
	{"medium_grass", 18},
	{"long_grass",   19},
	{"sand",         32},
	{"cursor",       48},
	{NULL, 0},
};

/**
 * Convert a node group to a SURF game block.
 * @param ng Generic tree of nodes to convert.
 * @return The created SURF game block.
 */
static SURFBlock *ConvertSURFNode(NodeGroup *ng)
{
	ExpandNoExpression(ng->exprs, ng->line, "SURF");
	SURFBlock *sb = new SURFBlock;

	/* Prepare named values for access. */
	int length = 0;
	ValueInformation *vis = PrepareNamedValues(ng->values, &length, _surface_types);

	sb->surf_type  = FindValueInformation("surf_type",  vis, length, ng->line, "SURF").GetNumber(ng->line, "SURF");
	sb->tile_width = FindValueInformation("tile_width", vis, length, ng->line, "SURF").GetNumber(ng->line, "SURF");
	sb->z_height   = FindValueInformation("z_height",   vis, length, ng->line, "SURF").GetNumber(ng->line, "SURF");

	char buffer[16];
	buffer[0] = 'n';
	for (int i = 0; i < SURFACE_COUNT; i++) {
		strcpy(buffer + 1, _surface_sprite[i]);
		sb->sprites[i] = FindValueInformation(buffer, vis, length, ng->line, "SURF").GetSprite(ng->line, "SURF");
	}

	VerifyNamedValuesUse(vis, length, "SURF");
	delete[] vis;
	return sb;
}

/** Names of the foundation sprites. */
static const char *_foundation_sprite[] = {
	"se_e0", // FND_SE_E0,
	"se_0s", // FND_SE_0S,
	"se_es", // FND_SE_ES,
	"sw_s0", // FND_SW_S0,
	"sw_0w", // FND_SW_0W,
	"sw_sw", // FND_SW_SW,
};

/** Numeric symbols of the foundation types. */
static const Symbol _fund_symbols[] = {
	{"reserved",  0},
	{"ground",   16},
	{"wood",     32},
	{"brick",    48},
	{NULL, 0}
};

/**
 * Convert a node group to a FUND game block.
 * @param ng Generic tree of nodes to convert.
 * @return The created FUND game block.
 */
static FUNDBlock *ConvertFUNDNode(NodeGroup *ng)
{
	ExpandNoExpression(ng->exprs, ng->line, "FUND");
	FUNDBlock *fb = new FUNDBlock;

	/* Prepare named values for access. */
	int length = 0;
	ValueInformation *vis = PrepareNamedValues(ng->values, &length, _fund_symbols);

	fb->found_type = FindValueInformation("found_type", vis, length, ng->line, "FUND").GetNumber(ng->line, "FUND");
	fb->tile_width = FindValueInformation("tile_width", vis, length, ng->line, "FUND").GetNumber(ng->line, "FUND");
	fb->z_height   = FindValueInformation("z_height",   vis, length, ng->line, "FUND").GetNumber(ng->line, "FUND");

	for (int i = 0; i < FOUNDATION_COUNT; i++) {
		fb->sprites[i] = FindValueInformation(_foundation_sprite[i], vis, length, ng->line, "FUND").GetSprite(ng->line, "FUND");
	}

	VerifyNamedValuesUse(vis, length, "FUND");
	delete[] vis;
	return fb;
}

/**
 * Convert a node group to a TCOR game block.
 * @param ng Generic tree of nodes to convert.
 * @return The created TCOR game block.
 */
static TCORBlock *ConvertTCORNode(NodeGroup *ng)
{
	ExpandNoExpression(ng->exprs, ng->line, "TCOR");

	TCORBlock *blk = new TCORBlock;

	/* Prepare named values for access. */
	int length = 0;
	ValueInformation *vis = PrepareNamedValues(ng->values, &length);

	/* get the fields and their value. */
	blk->tile_width = FindValueInformation("tile_width", vis, length, ng->line, "TCOR").GetNumber(ng->line, "TCOR");
	blk->z_height   = FindValueInformation("z_height",   vis, length, ng->line, "TCOR").GetNumber(ng->line, "TCOR");

	char buffer[16];
	buffer[0] = 'n';
	for (int i = 0; i < SURFACE_COUNT; i++) {
		strcpy(buffer + 1, _surface_sprite[i]);

		buffer[0] = 'n';
		blk->north[i] = FindValueInformation(buffer, vis, length, ng->line, "TCOR").GetSprite(ng->line, "TCOR");

		buffer[0] = 'e';
		blk->east[i] = FindValueInformation(buffer, vis, length, ng->line, "TCOR").GetSprite(ng->line, "TCOR");

		buffer[0] = 's';
		blk->south[i] = FindValueInformation(buffer, vis, length, ng->line, "TCOR").GetSprite(ng->line, "TCOR");

		buffer[0] = 'w';
		blk->west[i] = FindValueInformation(buffer, vis, length, ng->line, "TCOR").GetSprite(ng->line, "TCOR");
	}

	VerifyNamedValuesUse(vis, length, "TCOR");
	delete[] vis;
	return blk;
}

/**
 * Convert a node group to a sprite-sheet block.
 * @param ng Generic tree of nodes to convert.
 * @return The created sprite-sheet node.
 */
static BlockNode *ConvertSheetNode(NodeGroup *ng)
{
	ExpandNoExpression(ng->exprs, ng->line, "sheet");

	SheetBlock *sb = new SheetBlock(ng->line);

	int length = 0;
	ValueInformation *vis = PrepareNamedValues(ng->values, &length);

	sb->file     = FindValueInformation("file",     vis, length, ng->line, "sheet").GetString(ng->line, "sheet");
	sb->x_base   = FindValueInformation("x_base",   vis, length, ng->line, "sheet").GetNumber(ng->line, "sheet");
	sb->y_base   = FindValueInformation("y_base",   vis, length, ng->line, "sheet").GetNumber(ng->line, "sheet");
	sb->x_step   = FindValueInformation("x_step",   vis, length, ng->line, "sheet").GetNumber(ng->line, "sheet");
	sb->y_step   = FindValueInformation("y_step",   vis, length, ng->line, "sheet").GetNumber(ng->line, "sheet");
	sb->x_offset = FindValueInformation("x_offset", vis, length, ng->line, "sheet").GetNumber(ng->line, "sheet");
	sb->y_offset = FindValueInformation("y_offset", vis, length, ng->line, "sheet").GetNumber(ng->line, "sheet");
	sb->width    = FindValueInformation("width",    vis, length, ng->line, "sheet").GetNumber(ng->line, "sheet");
	sb->height   = FindValueInformation("height",   vis, length, ng->line, "sheet").GetNumber(ng->line, "sheet");

	VerifyNamedValuesUse(vis, length, "sheet");
	delete[] vis;
	return sb;
}

/**
 * Convert a 'sprite' node.
 * @param ng Generic tree of nodes to convert.
 * @return The converted sprite block.
 */
static SpriteBlock *ConvertSpriteNode(NodeGroup *ng)
{
	ExpandNoExpression(ng->exprs, ng->line, "sprite");

	int length = 0;
	ValueInformation *vis = PrepareNamedValues(ng->values, &length);

	std::string file = FindValueInformation("file",     vis, length, ng->line, "sprite").GetString(ng->line, "sprite");
	int xbase        = FindValueInformation("x_base",   vis, length, ng->line, "sprite").GetNumber(ng->line, "sprite");
	int ybase        = FindValueInformation("y_base",   vis, length, ng->line, "sprite").GetNumber(ng->line, "sprite");
	int width        = FindValueInformation("width",    vis, length, ng->line, "sprite").GetNumber(ng->line, "sprite");
	int height       = FindValueInformation("height",   vis, length, ng->line, "sprite").GetNumber(ng->line, "sprite");
	int xoffset      = FindValueInformation("x_offset", vis, length, ng->line, "sprite").GetNumber(ng->line, "sprite");
	int yoffset      = FindValueInformation("y_offset", vis, length, ng->line, "sprite").GetNumber(ng->line, "sprite");

	VerifyNamedValuesUse(vis, length, "sprite");
	delete[] vis;

	SpriteBlock *sb = new SpriteBlock;
	Image img;
	img.LoadFile(file.c_str());
	const char *err = sb->sprite_image.CopySprite(&img, xoffset, yoffset, xbase, ybase, width, height);
	if (err != NULL) {
		fprintf(stderr, "Error at line %d, loading of the sprite for \"%s\" failed: %s\n", ng->line, ng->name, err);
		exit(1);
	}

	return sb;
}

/**
 * Convert a node group.
 * @param ng Node group to convert.
 * @return The converted node.
 */
static BlockNode *ConvertNodeGroup(NodeGroup *ng)
{
	if (strcmp(ng->name, "file")  == 0) return ConvertFileNode(ng);
	if (strcmp(ng->name, "sheet") == 0) return ConvertSheetNode(ng);
	if (strcmp(ng->name, "sprite") == 0) return ConvertSpriteNode(ng);

	/* Game blocks. */
	if (strcmp(ng->name, "TSEL") == 0) return ConvertTSELNode(ng);
	if (strcmp(ng->name, "TCOR") == 0) return ConvertTCORNode(ng);
	if (strcmp(ng->name, "SURF") == 0) return ConvertSURFNode(ng);
	if (strcmp(ng->name, "FUND") == 0) return ConvertFUNDNode(ng);

	/* Unknown type of node. */
	fprintf(stderr, "Error at line %d: Do not know how to check and simplify node \"%s\"\n", ng->line, ng->name);
	exit(1);
}

/**
 * Check and convert the tree to nodes.
 * @param root Root node of the tree.
 * @return The converted and checked sequence of file data to write.
 */
FileNodeList *CheckTree(GroupList *root)
{
	assert(sizeof(_surface_sprite) / sizeof(_surface_sprite[0]) == SURFACE_COUNT);
	assert(sizeof(_foundation_sprite) / sizeof(_foundation_sprite[0]) == FOUNDATION_COUNT);

	FileNodeList *file_nodes = new FileNodeList;
	for (std::list<Group *>::iterator iter = root->groups.begin(); iter != root->groups.end(); iter++) {
		NodeGroup *ng = (*iter)->CastToNodeGroup();
		assert(ng != NULL); // A GroupList can only have node groups.
		BlockNode *bn = ConvertNodeGroup(ng);
		FileNode *fn = dynamic_cast<FileNode *>(bn);
		if (fn == NULL) {
			fprintf(stderr, "Error at line %d: Node is not a file node\n", ng->GetLine());
			exit(1);
		}
		file_nodes->files.push_back(fn);
	}
	return file_nodes;
}
