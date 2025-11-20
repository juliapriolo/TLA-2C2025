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
Factor * AggregateFactorSemanticAction(AggregateFunction function, char * columnName);
AggregateFunction AggregateFunctionSemanticAction(TokenLabel token);
Program * ExpressionProgramSemanticAction(Expression * expression);

/* Semantic actions for DSL constructs */
Statement * SourceStatementSemanticAction(char * sourceId, char * csvFile, char * sourceIdentifier, FilterCondition * filters, Projection * projection);
Statement * ChartStatementSemanticAction(char * title, ChartType type, Source * sources, char * xColumn, Expression * yExpression, char * yAlias);
Program * ProgramFromStatementsSemanticAction(Statement * statements);
FilterCondition * FilterConditionSemanticAction(char * columnName, TokenLabel operator, char * stringValue);
FilterCondition * FilterConditionIntSemanticAction(char * columnName, TokenLabel operator, int intValue);
Projection * ProjectionSemanticAction(char ** columns, size_t columnCount);
Source * SourceSemanticAction(char * identifier, char * csvFile, char * sourceIdentifier, FilterCondition * filters, Projection * projection);
Chart * ChartSemanticAction(char * title, ChartType type, Source * sources, char * xColumn, Expression * yExpression, char * yAlias);
ChartType ChartTypeSemanticAction(TokenLabel token);
FilterOperator FilterOperatorSemanticAction(TokenLabel token);

void SetCurrentChart(Chart * chart);
void SetChartId(char * id);
void SetChartYAlias(char * alias);
void SetChartOrientation(TokenLabel orientation);
void SetChartColors(char ** colors, size_t colorCount);
void SetChartSingleColor(char * color);
void SetChartXRange(double * range);
void SetChartYRange(double * range);
void SetChartLegendPosition(TokenLabel position);
void SetChartHole(double hole);

bool bisonHasSemanticErrors(void);

#endif
