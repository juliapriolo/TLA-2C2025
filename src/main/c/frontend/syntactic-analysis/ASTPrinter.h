#ifndef AST_PRINTER_H
#define AST_PRINTER_H

#include "AbstractSyntaxTree.h"

/* Print a program's AST to stdout in a readable, indented form. */
void printProgramAST(Program * program);

/* Lower-level helpers (exported in case they're useful elsewhere) */
void printExpressionAST(Expression * expression, int indent);
void printFactorAST(Factor * factor, int indent);
void printConstantAST(Constant * constant, int indent);

#endif
