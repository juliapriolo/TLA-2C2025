/* Simple AST pretty-printer for debugging and visualization.
 * Prints the AST to stdout with indentation.
 */

#include "ASTPrinter.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void _printIndent(int indent) {
    for (int i = 0; i < indent; ++i) putchar(' ');
}

void printConstantAST(Constant * constant, int indent) {
    _printIndent(indent);
    if (constant == NULL) {
        printf("Constant: (null)\n");
        return;
    }

    /* Print whichever field appears set. For safety we print all present. */
    if (constant->string != NULL) {
        printf("Constant (STRING): \"%s\"\n", constant->string);
        return;
    }
    if (constant->color != NULL) {
        printf("Constant (COLOR): %s\n", constant->color);
        return;
    }
    /* number vs integer: show both if present; prefer number when non-zero or when fractional */
    if (constant->number != 0.0) {
        printf("Constant (NUMBER): %g\n", constant->number);
        return;
    }
    /* fallback to integer */
    printf("Constant (INTEGER): %d\n", constant->value);
}

void printFactorAST(Factor * factor, int indent) {
    if (factor == NULL) {
        _printIndent(indent);
        printf("Factor: (null)\n");
        return;
    }

    switch (factor->type) {
        case CONSTANT:
            _printIndent(indent);
            printf("Factor: CONSTANT\n");
            printConstantAST(factor->constant, indent + 2);
            break;
        case EXPRESSION:
            _printIndent(indent);
            printf("Factor: EXPRESSION\n");
            printExpressionAST(factor->expression, indent + 2);
            break;
        default:
            _printIndent(indent);
            printf("Factor: <unknown type %d>\n", (int)factor->type);
    }
}

static const char * _exprTypeName(ExpressionType t) {
    switch (t) {
        case ADDITION: return "ADDITION";
        case SUBTRACTION: return "SUBTRACTION";
        case MULTIPLICATION: return "MULTIPLICATION";
        case DIVISION: return "DIVISION";
        case FACTOR: return "FACTOR";
        default: return "UNKNOWN";
    }
}

void printExpressionAST(Expression * expression, int indent) {
    if (expression == NULL) {
        _printIndent(indent);
        printf("Expression: (null)\n");
        return;
    }

    _printIndent(indent);
    printf("Expression: %s\n", _exprTypeName(expression->type));

    switch (expression->type) {
        case FACTOR:
            printFactorAST(expression->factor, indent + 2);
            break;
        case ADDITION:
        case SUBTRACTION:
        case MULTIPLICATION:
        case DIVISION:
            _printIndent(indent + 2);
            printf("Left:\n");
            printExpressionAST(expression->leftExpression, indent + 4);
            _printIndent(indent + 2);
            printf("Right:\n");
            printExpressionAST(expression->rightExpression, indent + 4);
            break;
        default:
            _printIndent(indent + 2);
            printf("<unknown expression type>\n");
    }
}

void printProgramAST(Program * program) {
    if (program == NULL) {
        printf("Program: (null)\n");
        return;
    }
    printf("Program\n");
    if (program->expression == NULL) {
        printf("  (empty program)\n");
        return;
    }
    printExpressionAST(program->expression, 2);
}
