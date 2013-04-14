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
#include "nodes.h"
#include "string_names.h"

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

	for (std::list<BaseNamedValue *>::iterator iter = ng->values->values.begin(); iter != ng->values->values.end(); iter++) {
		NamedValue *nv = dynamic_cast<NamedValue *>(*iter);
		assert(nv != NULL); // Should always hold, as ImportValue has been eliminated.
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
	Strings *GetStrings(int line, const char *node);

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
 * Get a set of strings from the given node value.
 * @param line Line number of the node (for reporting errors).
 * @param node %Name of the node.
 * @return The set of strings.
 */
Strings *ValueInformation::GetStrings(int line, const char *node)
{
	Strings *st = dynamic_cast<Strings *>(this->node_value);
	if (st != NULL) {
		this->node_value = NULL;
		return st;
	}
	fprintf(stderr, "Error at line %d: Field \"%s\" of node \"%s\" is not a strings node\n", line, this->name.c_str(), node);
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
	long long GetNumber(const char *fld_name, const Symbol *symbols = NULL);
	std::string GetString(const char *fld_name);
	SpriteBlock *GetSprite(const char *fld_name);
	Strings *GetStrings(const char *fld_name);
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
	for (std::list<BaseNamedValue *>::iterator iter = values->values.begin(); iter != values->values.end(); iter++) {
		NamedValue *nv = dynamic_cast<NamedValue *>(*iter);
		assert(nv != NULL); // Should always hold, as ImportValue has been eliminated.
		if (nv->name == NULL) { // Unnamed value.
			if (!allow_unnamed) {
				fprintf(stderr, "Error at line %d: Value should have a name\n", nv->group->GetLine());
				exit(1);
			}
			unnamed_count++;
		} else {
			if (!allow_named) {
				fprintf(stderr, "Error at line %d: Value should not have a name\n", nv->group->GetLine());
				exit(1);
			}
			int count = nv->name->GetNameCount();
			named_count += count;
		}
	}

	this->CreateValues(named_count, unnamed_count);

	named_count = 0;
	unnamed_count = 0;
	for (std::list<BaseNamedValue *>::iterator iter = values->values.begin(); iter != values->values.end(); iter++) {
		NamedValue *nv = dynamic_cast<NamedValue *>(*iter);
		assert(nv != NULL); // Should always hold, as ImportValue has been eliminated.
		if (nv->name == NULL) { // Unnamed value.
			NodeGroup *ng = nv->group->CastToNodeGroup();
			if (ng != NULL) {
				this->unnamed_values[unnamed_count].expr_value = NULL;
				this->unnamed_values[unnamed_count].node_value = ConvertNodeGroup(ng);
				this->unnamed_values[unnamed_count].name = "???";
				this->unnamed_values[unnamed_count].line = ng->GetLine();
				this->unnamed_values[unnamed_count].used = false;
				unnamed_count++;
				continue;
			}
			ExpressionGroup *eg = nv->group->CastToExpressionGroup();
			assert(eg != NULL);
			this->unnamed_values[unnamed_count].expr_value = eg->expr->Evaluate(symbols);
			this->unnamed_values[unnamed_count].node_value = NULL;
			this->unnamed_values[unnamed_count].name = "???";
			this->unnamed_values[unnamed_count].line = ng->GetLine();
			this->unnamed_values[unnamed_count].used = false;
			unnamed_count++;
			continue;
		} else { // Named value.
			NodeGroup *ng = nv->group->CastToNodeGroup();
			if (ng != NULL) {
				BlockNode *bn = ConvertNodeGroup(ng);
				SingleName *sn = dynamic_cast<SingleName *>(nv->name);
				if (sn != NULL) {
					this->named_values[named_count].expr_value = NULL;
					this->named_values[named_count].node_value = bn;
					this->named_values[named_count].name = std::string(sn->name);
					this->named_values[named_count].line = sn->line;
					this->named_values[named_count].used = false;
					named_count++;
					continue;
				}
				NameTable *nt = dynamic_cast<NameTable *>(nv->name);
				assert(nt != NULL);
				AssignNames(bn, nt, this->named_values, &named_count);
				continue;
			}

			/* Expression group. */
			ExpressionGroup *eg = nv->group->CastToExpressionGroup();
			assert(eg != NULL);
			SingleName *sn = dynamic_cast<SingleName *>(nv->name);
			if (sn == NULL) {
				fprintf(stderr, "Error at line %d: Expression must have a single name\n", nv->name->GetLine());
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

/**
 * Get a numeric value from the named expression with the provided name.
 * @param fld_name Name of the field to retrieve.
 * @param symbols Optional set of identifiers recognized as numeric value.
 * @return The numeric value of the expression.
 */
long long Values::GetNumber(const char *fld_name, const Symbol *symbols)
{
	return FindValue(fld_name).GetNumber(this->node_line, this->node_name);
}

/**
 * Get a string value from the named expression with the provided name.
 * @param fld_name Name of the field to retrieve.
 * @return The value of the string.
 */
std::string Values::GetString(const char *fld_name)
{
	return FindValue(fld_name).GetString(this->node_line, this->node_name);
}

/**
 * Get a sprite (#SpriteBlock) from the named value with the provided name.
 * @param fld_name Name of the field to retrieve.
 * @return The sprite.
 */
SpriteBlock *Values::GetSprite(const char *fld_name)
{
	return FindValue(fld_name).GetSprite(this->node_line, this->node_name);
}

/**
 * Get a set of strings from the named value with the provided name.
 * @param fld_name Name of the field to retrieve.
 * @return The set of strings.
 */
Strings *Values::GetStrings(const char *fld_name)
{
	return FindValue(fld_name).GetStrings(this->node_line, this->node_name);
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
	blk->tile_width = vals.GetNumber("tile_width");
	blk->z_height   = vals.GetNumber("z_height");

	char buffer[16];
	buffer[0] = 'n';
	for (int i = 0; i < SURFACE_COUNT; i++) {
		strcpy(buffer + 1, _surface_sprite[i]);
		blk->sprites[i] = vals.GetSprite(buffer);
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

	sb->surf_type  = vals.GetNumber("surf_type");
	sb->tile_width = vals.GetNumber("tile_width");
	sb->z_height   = vals.GetNumber("z_height");

	char buffer[16];
	buffer[0] = 'n';
	for (int i = 0; i < SURFACE_COUNT; i++) {
		strcpy(buffer + 1, _surface_sprite[i]);
		sb->sprites[i] = vals.GetSprite(buffer);
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

	fb->found_type = vals.GetNumber("found_type");
	fb->tile_width = vals.GetNumber("tile_width");
	fb->z_height   = vals.GetNumber("z_height");

	for (int i = 0; i < FOUNDATION_COUNT; i++) {
		fb->sprites[i] = vals.GetSprite(_foundation_sprite[i]);
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

	blk->path_type = vals.GetNumber("path_type");
	blk->tile_width = vals.GetNumber("tile_width");
	blk->z_height = vals.GetNumber("z_height");

	for (int i = 0; i < PTS_COUNT; i++) {
		blk->sprites[i] = vals.GetSprite(_path_sprites[i]);
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

	blk->tile_width = vals.GetNumber("tile_width");
	blk->z_height = vals.GetNumber("z_height");
	blk->platform_type = vals.GetNumber("platform_type");

	for (int i = 0; i < PLA_COUNT; i++) {
		blk->sprites[i] = vals.GetSprite(_platform_sprites[i]);
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

	blk->support_type = vals.GetNumber("support_type");
	blk->tile_width = vals.GetNumber("tile_width");
	blk->z_height = vals.GetNumber("z_height");

	for (int i = 0; i < SPP_COUNT; i++) {
		blk->sprites[i] = vals.GetSprite(_support_sprites[i]);
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
	blk->tile_width = vals.GetNumber("tile_width");
	blk->z_height   = vals.GetNumber("z_height");

	char buffer[16];
	buffer[0] = 'n';
	for (int i = 0; i < SURFACE_COUNT; i++) {
		strcpy(buffer + 1, _surface_sprite[i]);

		buffer[0] = 'n';
		blk->north[i] = vals.GetSprite(buffer);

		buffer[0] = 'e';
		blk->east[i] = vals.GetSprite(buffer);

		buffer[0] = 's';
		blk->south[i] = vals.GetSprite(buffer);

		buffer[0] = 'w';
		blk->west[i] = vals.GetSprite(buffer);
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
		vi.node_value = NULL;
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

	blk->person_type = vals.GetNumber("person_type");
	blk->anim_type = vals.GetNumber("anim_type");

	for (int i = 0; i < vals.unnamed_count; i++) {
		ValueInformation &vi = vals.unnamed_values[i];
		if (vi.used) continue;
		FrameData *fd = dynamic_cast<FrameData *>(vi.node_value);
		if (fd == NULL) {
			fprintf(stderr, "Error at line %d: Node is not a \"frame_data\" node\n", vi.line);
			exit(1);
		}
		blk->frames.push_back(*fd);
		vi.node_value = NULL;
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

	blk->tile_width  = vals.GetNumber("tile_width");
	blk->person_type = vals.GetNumber("person_type");
	blk->anim_type   = vals.GetNumber("anim_type");

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
 * Convert a GBOR game block.
 * @param ng Node group to convert.
 * @return The created game block.
 */
static GBORBlock *ConvertGBORNode(NodeGroup *ng)
{
	ExpandNoExpression(ng->exprs, ng->line, "GBOR");
	GBORBlock *blk = new GBORBlock;

	Values vals("GBOR", ng->line);
	vals.PrepareNamedValues(ng->values, true, false);

	blk->widget_type = vals.GetNumber("widget_type");
	blk->border_top = vals.GetNumber("border_top");
	blk->border_left = vals.GetNumber("border_left");
	blk->border_right = vals.GetNumber("border_right");
	blk->border_bottom = vals.GetNumber("border_bottom");
	blk->min_width = vals.GetNumber("min_width");
	blk->min_height = vals.GetNumber("min_height");
	blk->h_stepsize = vals.GetNumber("h_stepsize");
	blk->v_stepsize = vals.GetNumber("v_stepsize");
	blk->tl = vals.GetSprite("top_left");
	blk->tm = vals.GetSprite("top_middle");
	blk->tr = vals.GetSprite("top_right");
	blk->ml = vals.GetSprite("middle_left");
	blk->mm = vals.GetSprite("middle_middle");
	blk->mr = vals.GetSprite("middle_right");
	blk->bl = vals.GetSprite("bottom_left");
	blk->bm = vals.GetSprite("bottom_middle");
	blk->br = vals.GetSprite("bottom_right");

	vals.VerifyUsage();
	return blk;
}

/**
 * Convert a GCHK game block.
 * @param ng Node group to convert.
 * @return The created game block.
 */
static GCHKBlock *ConvertGCHKNode(NodeGroup *ng)
{
	ExpandNoExpression(ng->exprs, ng->line, "GCHK");
	GCHKBlock *blk = new GCHKBlock;

	Values vals("GCHK", ng->line);
	vals.PrepareNamedValues(ng->values, true, false);

	blk->widget_type = vals.GetNumber("widget_type");
	blk->empty = vals.GetSprite("empty");
	blk->filled = vals.GetSprite("filled");
	blk->empty_pressed = vals.GetSprite("empty_pressed");
	blk->filled_pressed = vals.GetSprite("filled_pressed");
	blk->shaded_empty = vals.GetSprite("shaded_empty");
	blk->shaded_filled = vals.GetSprite("shaded_filled");

	vals.VerifyUsage();
	return blk;
}

/**
 * Convert a GSLI game block.
 * @param ng Node group to convert.
 * @return The created game block.
 */
static GSLIBlock *ConvertGSLINode(NodeGroup *ng)
{
	ExpandNoExpression(ng->exprs, ng->line, "GSLI");
	GSLIBlock *blk = new GSLIBlock;

	Values vals("GSLI", ng->line);
	vals.PrepareNamedValues(ng->values, true, false);

	blk->min_length = vals.GetNumber("min_length");
	blk->step_size = vals.GetNumber("step_size");
	blk->width = vals.GetNumber("width");
	blk->widget_type = vals.GetNumber("widget_type");
	blk->left = vals.GetSprite("left");
	blk->middle = vals.GetSprite("middle");
	blk->right = vals.GetSprite("right");
	blk->slider = vals.GetSprite("slider");

	vals.VerifyUsage();
	return blk;
}

/**
 * Convert a GSCL game block.
 * @param ng Node group to convert.
 * @return The created game block.
 */
static GSCLBlock *ConvertGSCLNode(NodeGroup *ng)
{
	ExpandNoExpression(ng->exprs, ng->line, "GSCL");
	GSCLBlock *blk = new GSCLBlock;

	Values vals("GSCL", ng->line);
	vals.PrepareNamedValues(ng->values, true, false);

	blk->min_length = vals.GetNumber("min_length");
	blk->step_back = vals.GetNumber("step_back");
	blk->min_bar_length = vals.GetNumber("min_bar_length");
	blk->bar_step = vals.GetNumber("bar_step");
	blk->widget_type = vals.GetNumber("widget_type");
	blk->left_button = vals.GetSprite("left_button");
	blk->right_button = vals.GetSprite("right_button");
	blk->left_pressed = vals.GetSprite("left_pressed");
	blk->right_pressed = vals.GetSprite("right_pressed");
	blk->left_bottom = vals.GetSprite("left_bottom");
	blk->middle_bottom = vals.GetSprite("middle_bottom");
	blk->right_bottom = vals.GetSprite("right_bottom");
	blk->left_top = vals.GetSprite("left_top");
	blk->middle_top = vals.GetSprite("middle_top");
	blk->right_top = vals.GetSprite("right_top");
	blk->left_top_pressed = vals.GetSprite("left_top_pressed");
	blk->middle_top_pressed = vals.GetSprite("middle_top_pressed");
	blk->right_top_pressed = vals.GetSprite("right_top_pressed");

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

	sb->file     = vals.GetString("file");
	sb->x_base   = vals.GetNumber("x_base");
	sb->y_base   = vals.GetNumber("y_base");
	sb->x_step   = vals.GetNumber("x_step");
	sb->y_step   = vals.GetNumber("y_step");
	sb->x_offset = vals.GetNumber("x_offset");
	sb->y_offset = vals.GetNumber("y_offset");
	sb->width    = vals.GetNumber("width");
	sb->height   = vals.GetNumber("height");

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

	std::string file = vals.GetString("file");
	int xbase        = vals.GetNumber("x_base");
	int ybase        = vals.GetNumber("y_base");
	int width        = vals.GetNumber("width");
	int height       = vals.GetNumber("height");
	int xoffset      = vals.GetNumber("x_offset");
	int yoffset      = vals.GetNumber("y_offset");

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

	pg->person_type = vals.GetNumber("person_type");

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

	rc->orig = vals.GetNumber("original");
	rc->replace = vals.GetNumber("replace");

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

	fd->duration = vals.GetNumber("duration");
	fd->change_x = vals.GetNumber("change_x");
	fd->change_y = vals.GetNumber("change_y");

	vals.VerifyUsage();
	return fd;
}

/** Symbols of the shop game block. */
static const Symbol _shop_symbols[] = {
	{"ne_entrance", 0},
	{"se_entrance", 1},
	{"sw_entrance", 2},
	{"nw_entrance", 3},
	{"drink", 8},
	{"ice_cream", 9},
	{"non_salt_food", 16},
	{"salt_food", 24},
	{"umbrella", 32},
	{"map", 40},
	{NULL, 0},
};

/**
 * Convert a node group to a SHOP game block.
 * @param ng Generic tree of nodes to convert.
 * @return The created SHOP game block.
 */
static SHOPBlock *ConvertSHOPNode(NodeGroup *ng)
{
	ExpandNoExpression(ng->exprs, ng->line, "SHOP");
	SHOPBlock *sb = new SHOPBlock;

	Values vals("SHOP", ng->line);
	vals.PrepareNamedValues(ng->values, true, true, _shop_symbols);

	sb->tile_width = vals.GetNumber("tile_width");
	sb->height = vals.GetNumber("height");
	sb->flags = vals.GetNumber("flags");
	sb->ne_view = vals.GetSprite("ne");
	sb->se_view = vals.GetSprite("se");
	sb->sw_view = vals.GetSprite("sw");
	sb->nw_view = vals.GetSprite("nw");
	sb->item_cost[0] = vals.GetNumber("cost_item1");
	sb->item_cost[1] = vals.GetNumber("cost_item2");
	sb->ownership_cost = vals.GetNumber("cost_ownership");
	sb->opened_cost = vals.GetNumber("cost_opened");
	sb->item_type[0] = vals.GetNumber("type_item1");
	sb->item_type[1] = vals.GetNumber("type_item2");
	sb->shop_text = vals.GetStrings("texts");
	sb->shop_text->CheckTranslations(_shops_string_names, lengthof(_shops_string_names), ng->line);

	int free_recolour = 0;
	for (int i = 0; i < vals.unnamed_count; i++) {
		ValueInformation &vi = vals.unnamed_values[i];
		if (vi.used) continue;
		Recolouring *rc = dynamic_cast<Recolouring *>(vi.node_value);
		if (rc == NULL) {
			fprintf(stderr, "Error at line %d: Node is not a \"recolour\" node\n", vi.line);
			exit(1);
		}
		if (free_recolour >= 3) {
			fprintf(stderr, "Error at line %d: Recolouring node cannot be stored (maximum is 3)\n", vi.line);
			exit(1);
		}
		sb->recol[free_recolour] = *rc;
		free_recolour++;
		vi.used = true;
	}

	vals.VerifyUsage();
	return sb;
}

/**
 * Convert a 'strings' node.
 * @param ng Generic tree of nodes to convert.
 * @return The created 'strings' node.
 */
Strings *ConvertStringsNode(NodeGroup *ng)
{
	ExpandNoExpression(ng->exprs, ng->line, "strings");
	Strings *strs = new Strings;

	Values vals("strings", ng->line);
	vals.PrepareNamedValues(ng->values, false, true);

	for (int i = 0; i < vals.unnamed_count; i++) {
		ValueInformation &vi = vals.unnamed_values[i];
		if (vi.used) continue;
		TextNode *tn = dynamic_cast<TextNode *>(vi.node_value);
		if (tn == NULL) {
			fprintf(stderr, "Error at line %d: Node is not a \"string\" node\n", vi.line);
			exit(1);
		}
		std::set<TextNode>::iterator iter = strs->texts.find(*tn);
		if (iter != strs->texts.end()) {
			for (int j = 0; j < LNG_COUNT; j++) {
				if (tn->lines[j] >= 0) {
					if ((*iter).lines[j] >= 0) {
						fprintf(stderr, "Error at line %d: \"string\" node conflicts with line %d\n",
								tn->lines[j], (*iter).lines[j]);
						exit(1);
					}
					(*iter).lines[j] = tn->lines[j];
					(*iter).texts[j] = tn->texts[j];
				}
			}
		} else {
			strs->texts.insert(*tn);
		}
		vi.used = true;
	}

	vals.VerifyUsage();
	return strs;
}

/**
 * Convert a 'string' node.
 * @param ng Generic tree of nodes to convert.
 * @return The created 'string' node.
 */
TextNode *ConvertTextNode(NodeGroup *ng)
{
	ExpandNoExpression(ng->exprs, ng->line, "string");
	TextNode *tn = new TextNode;

	Values vals("string", ng->line);
	vals.PrepareNamedValues(ng->values, true, false);

	tn->name = vals.GetString("name");
	ValueInformation &vi = vals.FindValue("lang");
	int lng = GetLanguageIndex(vi.GetString(ng->line, "string").c_str(), vi.line);
	vi = vals.FindValue("text");
	tn->lines[lng] = vi.line;
	tn->texts[lng] = vi.GetString(ng->line, "string");

	vals.VerifyUsage();
	return tn;
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
	if (strcmp(ng->name, "strings") == 0) return ConvertStringsNode(ng);
	if (strcmp(ng->name, "string") == 0) return ConvertTextNode(ng);

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
	if (strcmp(ng->name, "SHOP") == 0) return ConvertSHOPNode(ng);
	if (strcmp(ng->name, "GBOR") == 0) return ConvertGBORNode(ng);
	if (strcmp(ng->name, "GCHK") == 0) return ConvertGCHKNode(ng);
	if (strcmp(ng->name, "GSLI") == 0) return ConvertGSLINode(ng);
	if (strcmp(ng->name, "GSCL") == 0) return ConvertGSCLNode(ng);

	/* Unknown type of node. */
	fprintf(stderr, "Error at line %d: Do not know how to check and simplify node \"%s\"\n", ng->line, ng->name);
	exit(1);
}

/**
 * Check and convert the tree to nodes.
 * @param values Root node of the tree.
 * @return The converted and checked sequence of file data to write.
 */
FileNodeList *CheckTree(NamedValueList *values)
{
	assert(sizeof(_surface_sprite) / sizeof(_surface_sprite[0]) == SURFACE_COUNT);
	assert(sizeof(_foundation_sprite) / sizeof(_foundation_sprite[0]) == FOUNDATION_COUNT);

	FileNodeList *file_nodes = new FileNodeList;
	Values vals("root", 1);
	vals.PrepareNamedValues(values, false, true);

	for (int i = 0; i < vals.unnamed_count; i++) {
		ValueInformation &vi = vals.unnamed_values[i];
		if (vi.used) continue;
		FileNode *fn = dynamic_cast<FileNode *>(vi.node_value);
		if (fn == NULL) {
			fprintf(stderr, "Error at line %d: Node is not a file node\n", vi.line);
			exit(1);
		}
		file_nodes->files.push_back(fn);
		vi.node_value = NULL;
		vi.used = true;
	}
	vals.VerifyUsage();
	return file_nodes;
}
