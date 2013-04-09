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

Expression::Expression(int line)
{
	this->line = line;
}

Expression::~Expression()
{
}

/**
 * \func  Expression *Expression::Evaluate() const
 * Evaluation of the expression. Reduces it to its value or throws a fatal error.
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

Expression *UnaryOperator::Evaluate() const
{
	Expression *result = child->Evaluate();
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

Expression *StringLiteral::Evaluate() const
{
	int length = strlen(this->text);
	char *copy = (char *)malloc(length + 1);
	memcpy(copy, this->text, length + 1);
	return new StringLiteral(this->line, copy);
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

Expression *IdentifierLiteral::Evaluate() const
{
	fprintf(stderr, "Evaluate error at line %d: Need symbol table to evaluate an identifier", this->line);
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

Expression *NumberLiteral::Evaluate() const
{
	return new NumberLiteral(this->line, this->value);
}


Name::Name()
{
}

Name::~Name() {
}

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

IdentifierLine::IdentifierLine(const IdentifierLine &il)
{
	this->line = il.line;
	int length = strlen(il.name);
	this->name = (char *)malloc(length + 1);
	memcpy(this->name, il.name, length + 1);
}

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


NameRow::NameRow()
{
}

NameRow::~NameRow()
{
	for (std::list<IdentifierLine *>::iterator iter = this->identifiers.begin(); iter != this->identifiers.end(); iter++) {
		delete *iter;
	}
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

Group::Group()
{
}

Group::~Group()
{
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

