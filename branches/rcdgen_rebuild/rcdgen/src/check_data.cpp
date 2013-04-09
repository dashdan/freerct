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

/** Class for storing found named values. */
class Values {
public:
	Values(const char *node_name, int node_line);
	~Values();

	const char *node_name; ///< Name of the node using the values (memory is not released).
	int node_line;         ///< Line number of the node.

	void PrepareNamedValues(NamedValueList *values, bool allow_named, bool allow_unnamed, const Symbol *symbols = NULL);
	ValueInformation &FindValue(const char *fld_name);
	void VerifyUsage();

	int named_count;   ///< Number of found values with a name.
	int unnamed_count; ///< Number of found value without a name.

	ValueInformation *named_values;   ///< Information about each named value.
	ValueInformation *unnamed_values; ///< Information about each unnamed value.

private:
	void CreateValues(int named_count, int unnamed_count);
};

/**
 * Constructor of the values collection.
 * @param node_name Name of the node using the values (caller owns the memory).
 * @param node_line Line number of the node.
 */
Values::Values(const char *node_name, int node_line)
{
	this->node_name = node_name;
	this->node_line = node_line;

	this->named_count = 0;
	this->unnamed_count = 0;
	this->named_values = NULL;
	this->unnamed_values = NULL;
}

/**
 * Create space for the information of of the values.
 * @param named_count Number of named values.
 * @param unnamed_count Number of unnamed valued.
 */
void Values::CreateValues(int named_count, int unnamed_count)
{
	delete[] this->named_values;
	delete[] this->unnamed_values;

	this->named_count = named_count;
	this->unnamed_count = unnamed_count;

	this->named_values = (named_count > 0) ? new ValueInformation[named_count] : NULL;
	this->unnamed_values = (unnamed_count > 0) ? new ValueInformation[unnamed_count] : NULL;
}

Values::~Values()
{
	delete[] this->named_values;
	delete[] this->unnamed_values;
}

/**
 * Prepare the named values for access by field name.
 * @param values Named value to prepare.
 * @param allow_named Allow named values.
 * @param allow_unnamed Allow unnamed values.
 * @param symbols Optional symbols that may be used in the values.
 */
void Values::PrepareNamedValues(NamedValueList *values, bool allow_named, bool allow_unnamed, const Symbol *symbols)
{
	/* Count number of named and unnamed values. */
	int named_count = 0;
	int unnamed_count = 0;
	for (std::list<NamedValue *>::iterator iter = values->values.begin(); iter != values->values.end(); iter++) {
		if ((*iter)->name == NULL) { // Unnamed value.
			if (!allow_unnamed) {
				fprintf(stderr, "Error at line %d: Value should have a name\n", (*iter)->group->GetLine());
				exit(1);
			}
			unnamed_count++;
		} else {
			if (!allow_named) {
				fprintf(stderr, "Error at line %d: Value should not have a name\n", (*iter)->group->GetLine());
				exit(1);
			}
			int count = (*iter)->name->GetNameCount();
			named_count += count;
		}
	}

	this->CreateValues(named_count, unnamed_count);

	named_count = 0;
	unnamed_count = 0;
	for (std::list<NamedValue *>::iterator iter = values->values.begin(); iter != values->values.end(); iter++) {
		if ((*iter)->name == NULL) { // Unnamed value.
			NodeGroup *ng = (*iter)->group->CastToNodeGroup();
			if (ng != NULL) {
				this->unnamed_values[unnamed_count].expr_value = NULL;
				this->unnamed_values[unnamed_count].node_value = ConvertNodeGroup(ng);
				this->unnamed_values[unnamed_count].name = "???";
				this->unnamed_values[unnamed_count].line = ng->GetLine();
				this->unnamed_values[unnamed_count].used = false;
				unnamed_count++;
				continue;
			}
			ExpressionGroup *eg = (*iter)->group->CastToExpressionGroup();
			assert(eg != NULL);
			this->unnamed_values[unnamed_count].expr_value = eg->expr->Evaluate(symbols);
			this->unnamed_values[unnamed_count].node_value = NULL;
			this->unnamed_values[unnamed_count].name = "???";
			this->unnamed_values[unnamed_count].line = ng->GetLine();
			this->unnamed_values[unnamed_count].used = false;
			unnamed_count++;
			continue;
		} else { // Named value.
			NodeGroup *ng = (*iter)->group->CastToNodeGroup();
			if (ng != NULL) {
				BlockNode *bn = ConvertNodeGroup(ng);
				SingleName *sn = dynamic_cast<SingleName *>((*iter)->name);
				if (sn != NULL) {
					this->named_values[named_count].expr_value = NULL;
					this->named_values[named_count].node_value = bn;
					this->named_values[named_count].name = std::string(sn->name);
					this->named_values[named_count].line = sn->line;
					this->named_values[named_count].used = false;
					named_count++;
					continue;
				}
				NameTable *nt = dynamic_cast<NameTable *>((*iter)->name);
				assert(nt != NULL);
				AssignNames(bn, nt, this->named_values, &named_count);
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
			this->named_values[named_count].expr_value = eg->expr->Evaluate(symbols);
			this->named_values[named_count].node_value = NULL;
			this->named_values[named_count].name = std::string(sn->name);
			this->named_values[named_count].line = sn->line;
			this->named_values[named_count].used = false;
			named_count++;
			continue;
		}
	}
	assert(named_count == this->named_count);
	assert(unnamed_count == this->unnamed_count);
}

/**
 * Find the value information named \a fld_name.
 * @param fld_name %Name of the field looking for.
 * @return Reference in the \a vis array.
 */
ValueInformation &Values::FindValue(const char *fld_name)
{
	for (int i = 0; i < this->named_count; i++) {
		ValueInformation &vi = this->named_values[i];
		if (!vi.used && vi.name == fld_name) {
			vi.used = true;
			return vi;
		}
	}
	fprintf(stderr, "Error at line %d: Cannot find a value for field \"%s\" in node \"%s\"\n", this->node_line, fld_name, this->node_name);
	exit(1);
}

/** Verify whether all named values were used in a node. */
void Values::VerifyUsage()
{
	for (int i = 0; i < this->unnamed_count; i++) {
		const ValueInformation &vi = this->unnamed_values[i];
		if (!vi.used) {
			fprintf(stderr, "Warning at line %d: Unnamed value in node \"%s\" was not used\n", vi.line, this->node_name);
		}
	}
	for (int i = 0; i < this->named_count; i++) {
		const ValueInformation &vi = this->named_values[i];
		if (!vi.used) {
			fprintf(stderr, "Warning at line %d: Named value \"%s\" was not used in node \"%s\"\n", vi.line, vi.name.c_str(), this->node_name);
		}
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

	Values vals("TSEL", ng->line);
	vals.PrepareNamedValues(ng->values, true, false);

	/* get the fields and their value. */
	blk->tile_width = vals.FindValue("tile_width").GetNumber(ng->line, "TSEL");
	blk->z_height   = vals.FindValue("z_height").GetNumber(ng->line, "TSEL");

	char buffer[16];
	buffer[0] = 'n';
	for (int i = 0; i < SURFACE_COUNT; i++) {
		strcpy(buffer + 1, _surface_sprite[i]);
		blk->sprites[i] = vals.FindValue(buffer).GetSprite(ng->line, "TSEL");
	}

	vals.VerifyUsage();
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

	Values vals("SURF", ng->line);
	vals.PrepareNamedValues(ng->values, true, false, _surface_types);

	sb->surf_type  = vals.FindValue("surf_type").GetNumber(ng->line, "SURF");
	sb->tile_width = vals.FindValue("tile_width").GetNumber(ng->line, "SURF");
	sb->z_height   = vals.FindValue("z_height").GetNumber(ng->line, "SURF");

	char buffer[16];
	buffer[0] = 'n';
	for (int i = 0; i < SURFACE_COUNT; i++) {
		strcpy(buffer + 1, _surface_sprite[i]);
		sb->sprites[i] = vals.FindValue(buffer).GetSprite(ng->line, "SURF");
	}

	vals.VerifyUsage();
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

	Values vals("FUND", ng->line);
	vals.PrepareNamedValues(ng->values, true, false, _fund_symbols);

	fb->found_type = vals.FindValue("found_type").GetNumber(ng->line, "FUND");
	fb->tile_width = vals.FindValue("tile_width").GetNumber(ng->line, "FUND");
	fb->z_height   = vals.FindValue("z_height").GetNumber(ng->line, "FUND");

	for (int i = 0; i < FOUNDATION_COUNT; i++) {
		fb->sprites[i] = vals.FindValue(_foundation_sprite[i]).GetSprite(ng->line, "FUND");
	}

	vals.VerifyUsage();
	return fb;
}

/** Symbols for the PATH game block. */
static const Symbol _path_symbols[] = {
	{"concrete", 16},
	{NULL, 0},
};

/** Names of the PATH sprites. */
static const char *_path_sprites[] = {
	"empty",
	"ne",
	"se",
	"ne_se",
	"ne_se_e",
	"sw",
	"ne_sw",
	"se_sw",
	"se_sw_s",
	"ne_se_sw",
	"ne_se_sw_e",
	"ne_se_sw_s",
	"ne_se_sw_e_s",
	"nw",
	"ne_nw",
	"ne_nw_n",
	"nw_se",
	"ne_nw_se",
	"ne_nw_se_n",
	"ne_nw_se_e",
	"ne_nw_se_n_e",
	"nw_sw",
	"nw_sw_w",
	"ne_nw_sw",
	"ne_nw_sw_n",
	"ne_nw_sw_w",
	"ne_nw_sw_n_w",
	"nw_se_sw",
	"nw_se_sw_s",
	"nw_se_sw_w",
	"nw_se_sw_s_w",
	"ne_nw_se_sw",
	"ne_nw_se_sw_n",
	"ne_nw_se_sw_e",
	"ne_nw_se_sw_n_e",
	"ne_nw_se_sw_s",
	"ne_nw_se_sw_n_s",
	"ne_nw_se_sw_e_s",
	"ne_nw_se_sw_n_e_s",
	"ne_nw_se_sw_w",
	"ne_nw_se_sw_n_w",
	"ne_nw_se_sw_e_w",
	"ne_nw_se_sw_n_e_w",
	"ne_nw_se_sw_s_w",
	"ne_nw_se_sw_n_s_w",
	"ne_nw_se_sw_e_s_w",
	"ne_nw_se_sw_n_e_s_w",
	"ramp_ne",
	"ramp_nw",
	"ramp_se",
	"ramp_sw",
};

/**
 * Convert a node group to a PATH game block.
 * @param ng Generic tree of nodes to convert.
 * @return The created PATH game block.
 */
static PATHBlock *ConvertPATHNode(NodeGroup *ng)
{
	ExpandNoExpression(ng->exprs, ng->line, "PATH");
	PATHBlock *blk = new PATHBlock;

	Values vals("PATH", ng->line);
	vals.PrepareNamedValues(ng->values, true, false, _path_symbols);

	blk->path_type = vals.FindValue("path_type").GetNumber(ng->line, "PATH");
	blk->tile_width = vals.FindValue("tile_width").GetNumber(ng->line, "PATH");
	blk->z_height = vals.FindValue("z_height").GetNumber(ng->line, "PATH");

	for (int i = 0; i < PTS_COUNT; i++) {
		blk->sprites[i] = vals.FindValue(_path_sprites[i]).GetSprite(ng->line, "PATH");
	}

	vals.VerifyUsage();
	return blk;
}

/** Symbols for the platform game block. */
static const Symbol _platform_symbols[] = {
	{"wood", 16},
	{NULL, 0},
};

/** Sprite names of the platform game block. */
static const char *_platform_sprites[] = {
	"ns",
	"ew",
	"ramp_ne",
	"ramp_se",
	"ramp_sw",
	"ramp_nw",
	"right_ramp_ne",
	"right_ramp_se",
	"right_ramp_sw",
	"right_ramp_nw",
	"left_ramp_ne",
	"left_ramp_se",
	"left_ramp_sw",
	"left_ramp_nw",
};

/**
 * Convert a node group to a PLAT game block.
 * @param ng Generic tree of nodes to convert.
 * @return The created PLAT game block.
 */
static PLATBlock *ConvertPLATNode(NodeGroup *ng)
{
	ExpandNoExpression(ng->exprs, ng->line, "PLAT");
	PLATBlock *blk = new PLATBlock;

	Values vals("PLAT", ng->line);
	vals.PrepareNamedValues(ng->values, true, false, _platform_symbols);

	blk->tile_width = vals.FindValue("tile_width").GetNumber(ng->line, "PLAT");
	blk->z_height = vals.FindValue("z_height").GetNumber(ng->line, "PLAT");
	blk->platform_type = vals.FindValue("platform_type").GetNumber(ng->line, "PLAT");

	for (int i = 0; i < PLA_COUNT; i++) {
		blk->sprites[i] = vals.FindValue(_platform_sprites[i]).GetSprite(ng->line, "PLAT");
	}

	vals.VerifyUsage();
	return blk;
}

/** Symbols for the support game block. */
static const Symbol _support_symbols[] = {
	{"wood", 16},
	{NULL, 0},
};

/** Sprite names of the support game block. */
static const char *_support_sprites[] = {
	"s_ns",
	"s_ew",
	"d_ns",
	"d_ew",
	"p_ns",
	"p_ew",
	"n#n",
	"n#e",
	"n#ne",
	"n#s",
	"n#ns",
	"n#es",
	"n#nes",
	"n#w",
	"n#nw",
	"n#ew",
	"n#new",
	"n#sw",
	"n#nsw",
	"n#esw",
	"n#N",
	"n#E",
	"n#S",
	"n#W",
};

/**
 * Convert a node group to a PLAT game block.
 * @param ng Generic tree of nodes to convert.
 * @return The created PLAT game block.
 */
static SUPPBlock *ConvertSUPPNode(NodeGroup *ng)
{
	ExpandNoExpression(ng->exprs, ng->line, "SUPP");
	SUPPBlock *blk = new SUPPBlock;

	Values vals("SUPP", ng->line);
	vals.PrepareNamedValues(ng->values, true, false, _support_symbols);

	blk->support_type = vals.FindValue("support_type").GetNumber(ng->line, "SUPP");
	blk->tile_width = vals.FindValue("tile_width").GetNumber(ng->line, "SUPP");
	blk->z_height = vals.FindValue("z_height").GetNumber(ng->line, "SUPP");

	for (int i = 0; i < SPP_COUNT; i++) {
		blk->sprites[i] = vals.FindValue(_support_sprites[i]).GetSprite(ng->line, "SUPP");
	}

	vals.VerifyUsage();
	return blk;
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

	Values vals("TCOR", ng->line);
	vals.PrepareNamedValues(ng->values, true, false);

	/* get the fields and their value. */
	blk->tile_width = vals.FindValue("tile_width").GetNumber(ng->line, "TCOR");
	blk->z_height   = vals.FindValue("z_height").GetNumber(ng->line, "TCOR");

	char buffer[16];
	buffer[0] = 'n';
	for (int i = 0; i < SURFACE_COUNT; i++) {
		strcpy(buffer + 1, _surface_sprite[i]);

		buffer[0] = 'n';
		blk->north[i] = vals.FindValue(buffer).GetSprite(ng->line, "TCOR");

		buffer[0] = 'e';
		blk->east[i] = vals.FindValue(buffer).GetSprite(ng->line, "TCOR");

		buffer[0] = 's';
		blk->south[i] = vals.FindValue(buffer).GetSprite(ng->line, "TCOR");

		buffer[0] = 'w';
		blk->west[i] = vals.FindValue(buffer).GetSprite(ng->line, "TCOR");
	}

	vals.VerifyUsage();
	return blk;
}

/**
 * Convert a node group to a PRSG game block.
 * @param ng Generic tree of nodes to convert.
 * @return The created PRSG game block.
 */
static PRSGBlock *ConvertPRSGNode(NodeGroup *ng)
{
	ExpandNoExpression(ng->exprs, ng->line, "PRSG");
	PRSGBlock *blk = new PRSGBlock;

	Values vals("PRSG", ng->line);
	vals.PrepareNamedValues(ng->values, false, true);

	for (int i = 0; i < vals.unnamed_count; i++) {
		ValueInformation &vi = vals.unnamed_values[i];
		if (vi.used) continue;
		PersonGraphics *pg = dynamic_cast<PersonGraphics *>(vi.node_value);
		if (pg == NULL) {
			fprintf(stderr, "Error at line %d: Node is not a person_graphics node\n", vi.line);
			exit(1);
		}
		blk->person_graphics.push_back(*pg);
		if (blk->person_graphics.size() > 255) {
			fprintf(stderr, "Error at line %d: Too many person graphics in a PRSG block\n", vi.line);
			exit(1);
		}
		vi.used = true;
	}

	vals.VerifyUsage();
	return blk;
}

/** Symbols for an ANIM and ANSP blocks. */
static const Symbol _anim_symbols[] = {
	{"pillar",  8},
	{"earth",  16},
	{"walk_ne", 1}, // Walk in north-east direction.
	{"walk_se", 2}, // Walk in south-east direction.
	{"walk_sw", 3}, // Walk in south-west direction.
	{"walk_nw", 4}, // Walk in north-west direction.
	{NULL, 0}
};

/**
 * Convert a node group to an ANIM game block.
 * @param ng Generic tree of nodes to convert.
 * @return The created ANIM game block.
 */
static ANIMBlock *ConvertANIMNode(NodeGroup *ng)
{
	ExpandNoExpression(ng->exprs, ng->line, "ANIM");
	ANIMBlock *blk = new ANIMBlock;

	Values vals("ANIM", ng->line);
	vals.PrepareNamedValues(ng->values, true, true, _anim_symbols);

	blk->person_type = vals.FindValue("person_type").GetNumber(ng->line, "ANIM");
	blk->anim_type = vals.FindValue("anim_type").GetNumber(ng->line, "ANIM");

	for (int i = 0; i < vals.unnamed_count; i++) {
		ValueInformation &vi = vals.unnamed_values[i];
		if (vi.used) continue;
		FrameData *fd = dynamic_cast<FrameData *>(vi.node_value);
		if (fd == NULL) {
			fprintf(stderr, "Error at line %d: Node is not a \"frame_data\" node\n", vi.line);
			exit(1);
		}
		blk->frames.push_back(*fd);
		if (blk->frames.size() > 0xFFFF) {
			fprintf(stderr, "Error at line %d: Too many frames in an ANIM block\n", vi.line);
			exit(1);
		}
		vi.used = true;
	}

	vals.VerifyUsage();
	return blk;
}

/**
 * Convert a node group to an ANSP game block.
 * @param ng Generic tree of nodes to convert.
 * @return The created ANSP game block.
 */
static ANSPBlock *ConvertANSPNode(NodeGroup *ng)
{
	ExpandNoExpression(ng->exprs, ng->line, "ANSP");
	ANSPBlock *blk = new ANSPBlock;

	Values vals("ANSP", ng->line);
	vals.PrepareNamedValues(ng->values, true, true, _anim_symbols);

	blk->tile_width = vals.FindValue("tile_width").GetNumber(ng->line, "ANSP");
	blk->person_type = vals.FindValue("person_type").GetNumber(ng->line, "ANSP");
	blk->anim_type = vals.FindValue("anim_type").GetNumber(ng->line, "ANSP");

	for (int i = 0; i < vals.unnamed_count; i++) {
		ValueInformation &vi = vals.unnamed_values[i];
		if (vi.used) continue;
		SpriteBlock *sp = dynamic_cast<SpriteBlock *>(vi.node_value);
		if (sp == NULL) {
			fprintf(stderr, "Error at line %d: Node is not a \"sprite\" node\n", vi.line);
			exit(1);
		}
		blk->frames.push_back(sp);
		vi.node_value = NULL;
		if (blk->frames.size() > 0xFFFF) {
			fprintf(stderr, "Error at line %d: Too many frames in an ANSP block\n", vi.line);
			exit(1);
		}
		vi.used = true;
	}

	vals.VerifyUsage();
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

	Values vals("sheet", ng->line);
	vals.PrepareNamedValues(ng->values, true, false);

	sb->file     = vals.FindValue("file").GetString(ng->line, "sheet");
	sb->x_base   = vals.FindValue("x_base").GetNumber(ng->line, "sheet");
	sb->y_base   = vals.FindValue("y_base").GetNumber(ng->line, "sheet");
	sb->x_step   = vals.FindValue("x_step").GetNumber(ng->line, "sheet");
	sb->y_step   = vals.FindValue("y_step").GetNumber(ng->line, "sheet");
	sb->x_offset = vals.FindValue("x_offset").GetNumber(ng->line, "sheet");
	sb->y_offset = vals.FindValue("y_offset").GetNumber(ng->line, "sheet");
	sb->width    = vals.FindValue("width").GetNumber(ng->line, "sheet");
	sb->height   = vals.FindValue("height").GetNumber(ng->line, "sheet");

	vals.VerifyUsage();
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

	Values vals("sprite", ng->line);
	vals.PrepareNamedValues(ng->values, true, false);

	std::string file = vals.FindValue("file").GetString(ng->line, "sprite");
	int xbase        = vals.FindValue("x_base").GetNumber(ng->line, "sprite");
	int ybase        = vals.FindValue("y_base").GetNumber(ng->line, "sprite");
	int width        = vals.FindValue("width").GetNumber(ng->line, "sprite");
	int height       = vals.FindValue("height").GetNumber(ng->line, "sprite");
	int xoffset      = vals.FindValue("x_offset").GetNumber(ng->line, "sprite");
	int yoffset      = vals.FindValue("y_offset").GetNumber(ng->line, "sprite");

	vals.VerifyUsage();

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

/** Names of person types and colour ranges. */
static const Symbol _person_graphics_symbols[] = {
	{"pillar",  8},
	{"earth",  16},
	{"grey",        COL_GREY},
	{"green_brown", COL_GREEN_BROWN},
	{"brown",       COL_BROWN},
	{"yellow",      COL_YELLOW},
	{"dark_red",    COL_DARK_RED},
	{"dark_green",  COL_DARK_GREEN},
	{"light_green", COL_LIGHT_GREEN},
	{"green",       COL_GREEN},
	{"light_red",   COL_LIGHT_RED},
	{"dark_blue",   COL_DARK_BLUE},
	{"blue",        COL_BLUE},
	{"light_blue",  COL_LIGHT_BLUE},
	{"purple",      COL_PURPLE},
	{"red",         COL_RED},
	{"orange",      COL_ORANGE},
	{"sea_green",   COL_SEA_GREEN},
	{"pink",        COL_PINK},
	{"beige",       COL_BEIGE},
	{NULL, 0}
};

/** Colour ranges for the recolour node. */
static const Symbol _recolour_symbols[] = {
	{"grey",        COL_GREY},
	{"green_brown", COL_GREEN_BROWN},
	{"brown",       COL_BROWN},
	{"yellow",      COL_YELLOW},
	{"dark_red",    COL_DARK_RED},
	{"dark_green",  COL_DARK_GREEN},
	{"light_green", COL_LIGHT_GREEN},
	{"green",       COL_GREEN},
	{"light_red",   COL_LIGHT_RED},
	{"dark_blue",   COL_DARK_BLUE},
	{"blue",        COL_BLUE},
	{"light_blue",  COL_LIGHT_BLUE},
	{"purple",      COL_PURPLE},
	{"red",         COL_RED},
	{"orange",      COL_ORANGE},
	{"sea_green",   COL_SEA_GREEN},
	{"pink",        COL_PINK},
	{"beige",       COL_BEIGE},
	{NULL, 0}
};

/**
 * Convert a 'person_graphics' node.
 * @param ng Generic tree of nodes to convert.
 * @return The converted sprite block.
 */
static PersonGraphics *ConvertPersonGraphicsNode(NodeGroup *ng)
{
	ExpandNoExpression(ng->exprs, ng->line, "person_graphics");
	PersonGraphics *pg = new PersonGraphics;

	Values vals("person_graphics", ng->line);
	vals.PrepareNamedValues(ng->values, true, true, _person_graphics_symbols);

	pg->person_type = vals.FindValue("person_type").GetNumber(ng->line, "person_graphics");

	for (int i = 0; i < vals.unnamed_count; i++) {
		ValueInformation &vi = vals.unnamed_values[i];
		if (vi.used) continue;
		Recolouring *rc = dynamic_cast<Recolouring *>(vi.node_value);
		if (rc == NULL) {
			fprintf(stderr, "Error at line %d: Node is not a recolour node\n", vi.line);
			exit(1);
		}
		if (!pg->AddRecolour(rc->orig, rc->replace)) {
			fprintf(stderr, "Error at line %d: Recolouring node cannot be stored (maximum is 3)\n", vi.line);
			exit(1);
		}
		vi.used = true;
	}

	vals.VerifyUsage();
	return pg;
}

/**
 * Convert a 'recolour' node.
 * @param ng Generic tree of nodes to convert.
 * @return The converted sprite block.
 */
static Recolouring *ConvertRecolourNode(NodeGroup *ng)
{
	ExpandNoExpression(ng->exprs, ng->line, "recolour");
	Recolouring *rc = new Recolouring;

	Values vals("recolour", ng->line);
	vals.PrepareNamedValues(ng->values, true, false, _recolour_symbols);

	rc->orig = vals.FindValue("original").GetNumber(ng->line, "recolour");
	rc->replace = vals.FindValue("replace").GetNumber(ng->line, "recolour");

	vals.VerifyUsage();
	return rc;
}

/**
 * Convert a 'frame_data' node.
 * @param ng Generic tree of nodes to convert.
 * @return The converted sprite block.
 */
static FrameData *ConvertFrameDataNode(NodeGroup *ng)
{
	ExpandNoExpression(ng->exprs, ng->line, "frame_data");
	FrameData *fd = new FrameData;

	Values vals("frame_data", ng->line);
	vals.PrepareNamedValues(ng->values, true, false);

	fd->duration = vals.FindValue("duration").GetNumber(ng->line, "frame_data");
	fd->change_x = vals.FindValue("change_x").GetNumber(ng->line, "frame_data");
	fd->change_y = vals.FindValue("change_y").GetNumber(ng->line, "frame_data");

	vals.VerifyUsage();
	return fd;
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
	if (strcmp(ng->name, "person_graphics") == 0) return ConvertPersonGraphicsNode(ng);
	if (strcmp(ng->name, "recolour") == 0) return ConvertRecolourNode(ng);
	if (strcmp(ng->name, "frame_data") == 0) return ConvertFrameDataNode(ng);

	/* Game blocks. */
	if (strcmp(ng->name, "TSEL") == 0) return ConvertTSELNode(ng);
	if (strcmp(ng->name, "TCOR") == 0) return ConvertTCORNode(ng);
	if (strcmp(ng->name, "SURF") == 0) return ConvertSURFNode(ng);
	if (strcmp(ng->name, "FUND") == 0) return ConvertFUNDNode(ng);
	if (strcmp(ng->name, "PRSG") == 0) return ConvertPRSGNode(ng);
	if (strcmp(ng->name, "ANIM") == 0) return ConvertANIMNode(ng);
	if (strcmp(ng->name, "ANSP") == 0) return ConvertANSPNode(ng);
	if (strcmp(ng->name, "PATH") == 0) return ConvertPATHNode(ng);
	if (strcmp(ng->name, "PLAT") == 0) return ConvertPLATNode(ng);
	if (strcmp(ng->name, "SUPP") == 0) return ConvertSUPPNode(ng);

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
