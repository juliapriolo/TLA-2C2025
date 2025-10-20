#ifndef SEMANTIC_VALUE_HEADER
#define SEMANTIC_VALUE_HEADER

#include "TokenLabel.h"
#include "../../frontend/syntactic-analysis/AbstractSyntaxTree.h"
#include "../../frontend/syntactic-analysis/BisonParser.h"

/**
 * The type of a Bison semantic value, that is, the meaning attached to a
 * token.
 */
/**
 * La unión que almacena el valor semántico de un token.
 * La idea: dependiendo del token usamos uno u otro campo.
 */
typedef union SemanticValue {
    int integer;        /* para INTEGER */
    double real;        /* para NUMBER (decimales) */
    char *string;       /* para STRING, IDENTIFIER, COLOR si se guarda como texto */
    void *ptr;          /* uso genérico: por ejemplo para almacenar Constant*, AST nodes, etc. */
} SemanticValue;

#endif
