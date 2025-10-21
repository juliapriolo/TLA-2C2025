#ifndef BISON_ACTIONS_HEADER
#define BISON_ACTIONS_HEADER

#include "../../support/logging/Logger.h"
#include "../../support/type/CompilerState.h"
#include "../../support/type/ModuleDestructor.h"
#include "../../support/type/TokenLabel.h"
#include "AbstractSyntaxTree.h"
#include "BisonParser.h"
#include <stdbool.h>
#include <stdlib.h>

/** Initialize module's internal state. */
ModuleDestructor initializeBisonActionsModule();

/**
 * Bison semantic actions.
 */

Constant * IntegerConstantSemanticAction(const int value);
Constant * ColorConstantSemanticAction(char * value);
Constant * StringConstantSemanticAction(char * value);
Constant * NumberConstantSemanticAction(double value); 

Expression * ArithmeticExpressionSemanticAction(Expression * leftExpression, Expression * rightExpression, ExpressionType type);
Expression * FactorExpressionSemanticAction(Factor * factor);
Factor * ConstantFactorSemanticAction(Constant * constant);
Factor * ExpressionFactorSemanticAction(Expression * expression);
Program * ExpressionProgramSemanticAction(Expression * expression);

/* Stubs for higher-level language constructs (chart/source) --- implement later */
Program * SourceProgramSemanticAction(char * sourceId, char * fromId);
Program * ChartProgramSemanticAction(char * chartId /*, more args */);

bool bisonHasSemanticErrors(void);

#endif
