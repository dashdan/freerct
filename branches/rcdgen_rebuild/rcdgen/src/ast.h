/* $Id$ */

/*
 * This file is part of FreeRCT.
 * FreeRCT is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * FreeRCT is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with FreeRCT. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file ast.h AST data structures. */

#ifndef AST_H
#define AST_H

#include <list>

/** Base class of expressions. */
class Expression {
public:
	Expression(int line);
	virtual ~Expression();

	virtual Expression *Evaluate() const = 0;

	int line; ///< Line number of the expression.
};

/** A sequence of expressions. */
class ExpressionList {
public:
	ExpressionList();
	~ExpressionList();

	std::list<Expression *>exprs; ///< The sequence of expressions.
};

/** Unary operator expression. */
class UnaryOperator : public Expression {
public:
	UnaryOperator(int line, int oper, Expression *child);
	/* virtual */ ~UnaryOperator();

	/* virtual */ Expression *Evaluate() const;

	int oper; ///< Operation performed, currently only \c '-' (unary negation is supported).
	Expression *child; ///< Child expression (should be numeric).
};

/** String literal elementary expression node. */
class StringLiteral : public Expression {
public:
	StringLiteral(int line, char *text);
	/* virtual */ ~StringLiteral();

	/* virtual */ Expression *Evaluate() const;

	char *text; ///< Text of the string literal (decoded).
};

/** Identifier elementary expression node. */
class IdentifierLiteral : public Expression {
public:
	IdentifierLiteral(int line, char *name);
	/* virtual */ ~IdentifierLiteral();

	/* virtual */ Expression *Evaluate() const;

	char *name; ///< The identifier of the expression.
};

/** Number literal elementary expression node. */
class NumberLiteral : public Expression {
public:
	NumberLiteral(int line, long long value);
	/* virtual */ ~NumberLiteral();

	/* virtual */ Expression *Evaluate() const;

	long long value; ///< Value of the number literal.
};

/** Base class for labels of named values. */
class Name {
public:
	Name();
	virtual ~Name();
};

/** Label of a named value containing a single name. */
class SingleName : public Name {
public:
	SingleName(int line, char *name);
	/* virtual */ ~SingleName();

	int line;   ///< Line number of the label.
	char *name; ///< The label itself.
};

/** Somewhat generic class for storing an identifier and its line number. */
class IdentifierLine {
public:
	IdentifierLine(int line, char *name);
	IdentifierLine(const IdentifierLine &il);
	IdentifierLine &operator=(const IdentifierLine &il);
	~IdentifierLine();

	int line;   ///< Line number of the label.
	char *name; ///< The label itself.
};

/** A row of identifiers. */
class NameRow {
public:
	NameRow();
	~NameRow();

	std::list<IdentifierLine *> identifiers;
};

/** a 2D table of identifiers. */
class NameTable : public Name {
public:
	NameTable();
	/* virtual */ ~NameTable();

	std::list<NameRow *> rows;
};

class NamedValueList;

/** Base class of the value part of a named value. */
class Group {
public:
	Group();
	virtual ~Group();
};

/** Value part consisting of a node. */
class NodeGroup : public Group {
public:
	NodeGroup(int line, char *name, ExpressionList *exprs, NamedValueList *values);
	/* virtual */ ~NodeGroup();

	int line;               ///< Line number of the node name.
	char *name;             ///< Node name itself.
	ExpressionList *exprs;  ///< Parameters of the node.
	NamedValueList *values; ///< Named values of the node.
};

/** Value part of a group consisting of an expression. */
class ExpressionGroup : public Group {
public:
	ExpressionGroup(Expression *expr);
	/* virtual */ ~ExpressionGroup();

	Expression *expr; ///< Expression to store.
};

/** Sequence of groups. */
class GroupList {
public:
	GroupList();
	~GroupList();

	std::list<Group *> groups;
};

/** A value with a name. */
class NamedValue {
public:
	NamedValue(Name *name, Group *group);
	~NamedValue();

	Name *name;   ///< Name part.
	Group *group; ///< Value part.
};

/** Sequence of named values. */
class NamedValueList {
public:
	NamedValueList();
	~NamedValueList();

	std::list<NamedValue *> values;
};

#endif

