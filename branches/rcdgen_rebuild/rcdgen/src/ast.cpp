/* $Id$ */

/*
 * This file is part of FreeRCT.
 * FreeRCT is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * FreeRCT is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with FreeRCT. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file ast.cpp AST data structure implementation. */

#include "stdafx.h"
#include "ast.h"

ExpressionList::ExpressionList()
{
}

ExpressionList::~ExpressionList()
{
	for (std::list<Expression *>::iterator iter = this->exprs.begin(); iter != this->exprs.end(); iter++) {
		delete (*iter);
	}
}

/**
 * Constructor of the base expression class.
 * @param line Line number of the expression node.
 */
Expression::Expression(int line)
{
	this->line = line;
}

Expression::~Expression()
{
}

/**
 * \fn  Expression *Expression::Evaluate(const Symbol *symbols) const
 * Evaluation of the expression. Reduces it to its value or throws a fatal error.
 * @param symbols Sequence of known identifier names.
 * @return The computed reduced expression.
 */

/**
 * Unary expression.
 * @param line Line number of the operator.
 * @param oper Unary operator. Only \c '-' is supported currently.
 * @param child Sub-exoression.
 */
UnaryOperator::UnaryOperator(int line, int oper, Expression *child) : Expression(line)
{
	this->oper = oper;
	this->child = child;
}

UnaryOperator::~UnaryOperator()
{
	delete child;
}

Expression *UnaryOperator::Evaluate(const Symbol *symbols) const
{
	Expression *result = child->Evaluate(symbols);
	NumberLiteral *number = dynamic_cast<NumberLiteral *>(result);
	if (number != NULL) {
		number->value = -number->value;
		return number;
	}
	fprintf(stderr, "Evaluate error at line %d: Cannot negate the value of the child expression", this->line);
	exit(1);
}

/**
 * A string literal as elementary expression.
 * @param line Line number of the string literal.
 * @param text String literal content itself.
 */
StringLiteral::StringLiteral(int line, char *text) : Expression(line)
{
	this->text = text;
}

StringLiteral::~StringLiteral()
{
	free(this->text);
}

Expression *StringLiteral::Evaluate(const Symbol *symbols) const
{
	char *copy = this->CopyText();
	return new StringLiteral(this->line, copy);
}

/**
 * Return a copy of the text of the string literal.
 * @return A copy of the string value.
 */
char *StringLiteral::CopyText() const
{
	int length = strlen(this->text);
	char *copy = (char *)malloc(length + 1);
	memcpy(copy, this->text, length + 1);
	return copy;
}

/**
 * An identifier as elementary expression.
 * @param line Line number of the identifier.
 * @param name The identifier to store.
 */
IdentifierLiteral::IdentifierLiteral(int line, char *name) : Expression(line)
{
	this->name = name;
}

IdentifierLiteral::~IdentifierLiteral()
{
	free(this->name);
}

Expression *IdentifierLiteral::Evaluate(const Symbol *symbols) const
{
	if (symbols != NULL) {
		for (;;) {
			if (symbols->name == NULL) break;
			if (strcmp(symbols->name, this->name) == 0) return new NumberLiteral(this->line, symbols->value);
			symbols++;
		}
	}
	fprintf(stderr, "Evaluate error at line %d: Identifier \"%s\" is not known\n", this->line, this->name);
	exit(1);
}

/**
 * A literal number as elementary expression.
 * @param line Line number of the value.
 * @param value The number itself.
 */
NumberLiteral::NumberLiteral(int line, long long value) : Expression(line)
{
	this->value = value;
}

NumberLiteral::~NumberLiteral()
{
}

Expression *NumberLiteral::Evaluate(const Symbol *symbols) const
{
	return new NumberLiteral(this->line, this->value);
}


Name::Name()
{
}

Name::~Name() {
}

/**
 * \fn Name::GetLine() const
 * Get a line number representing the name (group).
 * @return Line number pointing to the name part.
 */

/**
 * \fn Name::GetNameCount() const
 * Get the number of names attached to the 'name' part.
 * @return The number of names in this #Name.
 */

/**
 * A name for a group consisting of a single label.
 * @param line Line number of the label name.
 * @param name The label name itself.
 */
SingleName::SingleName(int line, char *name) : Name()
{
	this->line = line;
	this->name = name;
}

SingleName::~SingleName()
{
	free(this->name);
}

int SingleName::GetLine() const
{
	return this->line;
}

int SingleName::GetNameCount() const
{
	return 1;
}

/**
 * An identifier with a line number.
 * @param line Line number of the name.
 * @param name The identifier to store.
 */
IdentifierLine::IdentifierLine(int line, char *name)
{
	this->line = line;
	this->name = name;
}

/**
 * Copy constructor.
 * @param il Existing identifier line to copy.
 */
IdentifierLine::IdentifierLine(const IdentifierLine &il)
{
	this->line = il.line;
	int length = strlen(il.name);
	this->name = (char *)malloc(length + 1);
	memcpy(this->name, il.name, length + 1);
}

/**
 * Assignment operator of an identifier line.
 * @param il Identifier line being copied.
 * @return The identifier line copied to.
 */
IdentifierLine &IdentifierLine::operator=(const IdentifierLine &il)
{
	if (&il == this) return *this;
	free(this->name);

	this->line = il.line;
	int length = strlen(il.name);
	this->name = (char *)malloc(length + 1);
	memcpy(this->name, il.name, length + 1);
	return *this;
}

IdentifierLine::~IdentifierLine()
{
	free(this->name);
}

/**
 * Retrieve a line number for this identifier.
 * @return Line number.
 */
int IdentifierLine::GetLine() const
{
	return this->line;
}

/**
 * Is it a valid identifier to use?
 * @return Whether the name is valid for use.
 */
bool IdentifierLine::IsValid() const
{
	if (this->name == NULL) return false;
	if (this->name[0] == '\0' || this->name[0] == '_') return false;
	return true;
}

NameRow::NameRow()
{
}

NameRow::~NameRow()
{
	for (std::list<IdentifierLine *>::iterator iter = this->identifiers.begin(); iter != this->identifiers.end(); iter++) {
		delete *iter;
	}
}

/**
 * Get a line number of the row, or \c 0 if none is available.
 * @return Line number of the row.
 */
int NameRow::GetLine() const
{
	if (this->identifiers.size() > 0) return this->identifiers.front()->GetLine();
	return 0;
}

/**
 * Get the number of valid names in this row.
 * @return Number of valid names.
 */
int NameRow::GetNameCount() const
{
	int count = 0;
	for (std::list<IdentifierLine *>::const_iterator iter = this->identifiers.begin(); iter != this->identifiers.end(); iter++) {
		if ((*iter)->IsValid()) count++;
	}
	return count;
}

NameTable::NameTable() : Name()
{
}

NameTable::~NameTable()
{
	for (std::list<NameRow *>::iterator iter = this->rows.begin(); iter != this->rows.end(); iter++) {
		delete *iter;
	}
}

int NameTable::GetLine() const
{
	for (std::list<NameRow *>::const_iterator iter = this->rows.begin(); iter != this->rows.end(); iter++) {
		int line = (*iter)->GetLine();
		if (line > 0) return line;
	}
	return 0;
}

int NameTable::GetNameCount() const
{
	int count = 0;
	for (std::list<NameRow *>::const_iterator iter = this->rows.begin(); iter != this->rows.end(); iter++) {
		count += (*iter)->GetNameCount();
	}
	return count;
}

Group::Group()
{
}

Group::~Group()
{
}

/**
 * \fn Group::GetLine() const
 * Get a line number representing the name (group).
 * @return Line number pointing to the name part.
 */

/**
 * Cast the group to a #NodeGroup.
 * @return a node group if the cast succeeded, else \c NULL.
 */
/* virtual */ NodeGroup *Group::CastToNodeGroup()
{
	return NULL;
}

/**
 * Cast the group to a #ExpressionGroup.
 * @return an expression group if the cast succeeded, else \c NULL.
 */
/* virtual */ ExpressionGroup *Group::CastToExpressionGroup()
{
	return NULL;
}

/**
 * Construct a node.
 * @param line Line number of the label name.
 * @param name The label name itself.
 * @param exprs Actual parameters of the node.
 * @param values Named values of the node.
 */
NodeGroup::NodeGroup(int line, char *name, ExpressionList *exprs, NamedValueList *values) : Group()
{
	this->line = line;
	this->name = name;
	this->exprs = exprs;
	this->values = values;
}

NodeGroup::~NodeGroup()
{
	free(this->name);
	delete this->exprs;
	delete this->values;
}

/* virtual */ int NodeGroup::GetLine() const
{
	return this->line;
}

/* virtual */ NodeGroup *NodeGroup::CastToNodeGroup()
{
	return this;
}

/**
 * Wrap an expression in a group.
 * @param expr %Expression to wrap.
 */
ExpressionGroup::ExpressionGroup(Expression *expr) : Group()
{
	this->expr = expr;
}

ExpressionGroup::~ExpressionGroup()
{
	delete this->expr;
}

/* virtual */ int ExpressionGroup::GetLine() const
{
	return this->expr->line;
}

/* virtual */ ExpressionGroup *ExpressionGroup::CastToExpressionGroup()
{
	return this;
}

GroupList::GroupList()
{
}

GroupList::~GroupList()
{
	for (std::list<Group *>::iterator iter = this->groups.begin(); iter != this->groups.end(); iter++) {
		delete (*iter);
	}
}

/**
 * Construct a value with a name.
 * @param name (may be \c NULL).
 * @param group %Group value.
 */
NamedValue::NamedValue(Name *name, Group *group)
{
	this->name = name;
	this->group = group;
}

NamedValue::~NamedValue()
{
	delete this->name;
	delete this->group;
}

NamedValueList::NamedValueList()
{
}

NamedValueList::~NamedValueList()
{
	for (std::list<NamedValue *>::iterator iter = this->values.begin(); iter != this->values.end(); iter++) {
		delete (*iter);
	}
}

