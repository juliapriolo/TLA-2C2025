#include "BisonActions.h"
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

/* MODULE INTERNAL STATE */

static CompilerState * _compilerState = NULL;
static Logger * _logger = NULL;
static char ** _sourceIdentifiers = NULL;
static size_t _sourceIdentifiersCount = 0;
static Chart * _currentChart = NULL;  // Chart actual mientras se procesan opciones
static char * _pendingYAlias = NULL;  // Alias Y pendiente de asignar al chart cuando se cree
static char ** _semanticErrors = NULL;
static size_t _semanticErrorsCount = 0;
static size_t _semanticErrorsCapacity = 0;

static void _clearSourceIdentifiers(void) {
	if (_sourceIdentifiers != NULL) {
		for (size_t i = 0; i < _sourceIdentifiersCount; ++i) {
			free(_sourceIdentifiers[i]);
		}
		free(_sourceIdentifiers);
		_sourceIdentifiers = NULL;
	}
	_sourceIdentifiersCount = 0;
}

static void _clearSemanticErrors(void) {
	if (_semanticErrors != NULL) {
		for (size_t i = 0; i < _semanticErrorsCount; ++i) {
			free(_semanticErrors[i]);
		}
		free(_semanticErrors);
		_semanticErrors = NULL;
	}
	_semanticErrorsCount = 0;
	_semanticErrorsCapacity = 0;
}

static bool _sourceIdentifierExists(const char * identifier) {
	if (identifier == NULL) {
		return false;
	}
	for (size_t i = 0; i < _sourceIdentifiersCount; ++i) {
		if (_sourceIdentifiers[i] != NULL && strcmp(_sourceIdentifiers[i], identifier) == 0) {
			return true;
		}
	}
	return false;
}

static void _rememberSourceIdentifier(const char * identifier) {
	if (identifier == NULL) {
		return;
	}
	char * duplicated = strdup(identifier);
	if (duplicated == NULL) {
		if (_logger) {
			logError(_logger, "Out of memory while tracking source identifier \"%s\".", identifier);
		}
		return;
	}
	char ** resized = (char **) realloc(_sourceIdentifiers, sizeof(char *) * (_sourceIdentifiersCount + 1));
	if (resized == NULL) {
		if (_logger) {
			logError(_logger, "Out of memory while registering source identifier \"%s\".", identifier);
		}
		free(duplicated);
		return;
	}
	_sourceIdentifiers = resized;
	_sourceIdentifiers[_sourceIdentifiersCount] = duplicated;
	++_sourceIdentifiersCount;
}

static void _registerSemanticError(const char * format, ...) {
	if (format == NULL) {
		return;
	}
	va_list args;
	va_start(args, format);
	va_list argsCopy;
	va_copy(argsCopy, args);
	int required = vsnprintf(NULL, 0, format, argsCopy);
	va_end(argsCopy);
	if (required < 0) {
		va_end(args);
		return;
	}
	char * message = (char *) calloc((size_t) required + 1, sizeof(char));
	if (message == NULL) {
		if (_logger) {
			logError(_logger, "Out of memory while saving semantic error.");
		}
		va_end(args);
		return;
	}
	vsnprintf(message, (size_t) required + 1, format, args);
	va_end(args);

	if (_semanticErrorsCount == _semanticErrorsCapacity) {
		size_t newCapacity = _semanticErrorsCapacity == 0 ? 4 : _semanticErrorsCapacity * 2;
		char ** resized = (char **) realloc(_semanticErrors, newCapacity * sizeof(char *));
		if (resized == NULL) {
			if (_logger) {
				logError(_logger, "Out of memory while tracking semantic errors.");
			}
			free(message);
			return;
		}
		_semanticErrors = resized;
		_semanticErrorsCapacity = newCapacity;
	}

	_semanticErrors[_semanticErrorsCount++] = message;

	if (_logger) {
		logError(_logger, "%s", message);
	}
}

/** Cierra el estado interno del módulo. */
void _shutdownBisonActionsModule() {
	if (_logger != NULL) {
		destroyLogger(_logger);
		_logger = NULL;
	}
	_clearSourceIdentifiers();
	_clearSemanticErrors();
	_compilerState = NULL;
	if (_pendingYAlias != NULL) {
		free(_pendingYAlias);
		_pendingYAlias = NULL;
	}
}

ModuleDestructor initializeBisonActionsModule(CompilerState * compilerState) {
	_compilerState = compilerState;
	_logger = createLogger("BisonActions");
	_clearSourceIdentifiers();
	_clearSemanticErrors();
	return _shutdownBisonActionsModule;
}

/* FUNCIONES IMPORTADAS */

/* FUNCIONES PRIVADAS */

static void _logSyntacticAnalyzerAction(const char * functionName);

/**
 * Registra una acción del analizador sintáctico en nivel DEBUGGING.
 */
static void _logSyntacticAnalyzerAction(const char * functionName) {
}

/* FUNCIONES PÚBLICAS */

Constant * IntegerConstantSemanticAction(const int value) {
	_logSyntacticAnalyzerAction(__FUNCTION__);
	Constant * constant = calloc(1, sizeof(Constant));
	constant->value = value;
	return constant;
}

Constant * ColorConstantSemanticAction(char * value) {
    _logSyntacticAnalyzerAction(__FUNCTION__);
    Constant * constant = calloc(1, sizeof(Constant));
    constant->color = value; 
	return constant;
}

Constant * StringConstantSemanticAction(char * value) {
    _logSyntacticAnalyzerAction(__FUNCTION__);
    Constant * constant = calloc(1, sizeof(Constant));
    constant->string = value; 
    return constant;
}

Constant * NumberConstantSemanticAction(double value) {
    _logSyntacticAnalyzerAction(__FUNCTION__);
    Constant * constant = calloc(1, sizeof(Constant));
    constant->number = value; 
    return constant;
}

Expression * ArithmeticExpressionSemanticAction(Expression * leftExpression, Expression * rightExpression, ExpressionType type) {
	_logSyntacticAnalyzerAction(__FUNCTION__);
	Expression * expression = calloc(1, sizeof(Expression));
	expression->leftExpression = leftExpression;
	expression->rightExpression = rightExpression;
	expression->type = type;
	return expression;
}

Expression * FactorExpressionSemanticAction(Factor * factor) {
	_logSyntacticAnalyzerAction(__FUNCTION__);
	Expression * expression = calloc(1, sizeof(Expression));
	expression->factor = factor;
	expression->type = FACTOR;
	return expression;
}

Factor * ConstantFactorSemanticAction(Constant * constant) {
	_logSyntacticAnalyzerAction(__FUNCTION__);
	Factor * factor = calloc(1, sizeof(Factor));
	factor->constant = constant;
	factor->type = CONSTANT;
	return factor;
}

Factor * ExpressionFactorSemanticAction(Expression * expression) {
	_logSyntacticAnalyzerAction(__FUNCTION__);
	Factor * factor = calloc(1, sizeof(Factor));
	factor->expression = expression;
	factor->type = EXPRESSION;
	return factor;
}

Factor * AggregateFactorSemanticAction(AggregateFunction function, char * columnName) {
	_logSyntacticAnalyzerAction(__FUNCTION__);
	return createAggregateFactor(function, columnName);
}

AggregateFunction AggregateFunctionSemanticAction(TokenLabel token) {
	switch (token) {
		case AVG:
		case AVERAGE:
			return AGG_AVERAGE;
		case MIN:
			return AGG_MIN;
		case MAX:
			return AGG_MAX;
		case COUNT:
			return AGG_COUNT;
		case SUM:
			return AGG_SUM;
		default:
			return AGG_AVERAGE;
	}
}

Program * ExpressionProgramSemanticAction(Expression * expression) {
	_logSyntacticAnalyzerAction(__FUNCTION__);
	Program * program = calloc(1, sizeof(Program));
	program->expression = expression;
	_compilerState->abstractSyntaxtTree = program;
	return program;
}

/* Funciones auxiliares para convertir tokens a enums */

ChartType ChartTypeSemanticAction(TokenLabel token) {
	switch (token) {
		case BAR: return CHART_BAR;
		case PIE: return CHART_PIE;
		case DONUT: return CHART_DONUT;
		case SCATTER: return CHART_SCATTER;
		case LINE: return CHART_LINE;
		default: return CHART_BAR;
	}
}

FilterOperator FilterOperatorSemanticAction(TokenLabel token) {
	switch (token) {
		case EQEQ: return FILTER_EQ;
		case GT: return FILTER_GT;
		case LT: return FILTER_LT;
		case GE: return FILTER_GE;
		case LE: return FILTER_LE;
		default: return FILTER_EQ;
	}
}

/* Construcción de nodos del AST */

FilterCondition * FilterConditionSemanticAction(char * columnName, TokenLabel operator, char * stringValue) {
	_logSyntacticAnalyzerAction(__FUNCTION__);
	FilterOperator op = FilterOperatorSemanticAction(operator);
	return createFilterCondition(columnName, op, stringValue);
}

FilterCondition * FilterConditionIntSemanticAction(char * columnName, TokenLabel operator, int intValue) {
	_logSyntacticAnalyzerAction(__FUNCTION__);
	FilterOperator op = FilterOperatorSemanticAction(operator);
	return createFilterConditionInt(columnName, op, intValue);
}

Projection * ProjectionSemanticAction(char ** columns, size_t columnCount) {
	_logSyntacticAnalyzerAction(__FUNCTION__);
	return createProjection(columns, columnCount);
}

Source * SourceSemanticAction(char * identifier, char * csvFile, char * sourceIdentifier, FilterCondition * filters, Projection * projection) {
	_logSyntacticAnalyzerAction(__FUNCTION__);
	if (identifier != NULL) {
		if (_sourceIdentifierExists(identifier)) {
			_registerSemanticError("Duplicate source identifier \"%s\".", identifier);
			return NULL;
		} else {
			_rememberSourceIdentifier(identifier);
		}
	}
	if (sourceIdentifier != NULL && !_sourceIdentifierExists(sourceIdentifier)) {
		_registerSemanticError("Source identifier \"%s\" not found.", sourceIdentifier);
		return NULL;
	}
	return createSource(identifier, csvFile, sourceIdentifier, filters, projection);
}

Chart * ChartSemanticAction(char * title, ChartType type, Source * sources, char * xColumn, Expression * yExpression, char * yAlias) {
	_logSyntacticAnalyzerAction(__FUNCTION__);
	if (sources == NULL) {
		_registerSemanticError("Chart must have at least one source.");
		return NULL;
	}
	return createChart(title, type, sources, xColumn, yExpression, yAlias);
}

Statement * SourceStatementSemanticAction(char * sourceId, char * csvFile, char * sourceIdentifier, FilterCondition * filters, Projection * projection) {
	_logSyntacticAnalyzerAction(__FUNCTION__);
	Source * source = SourceSemanticAction(sourceId, csvFile, sourceIdentifier, filters, projection);
	if (source == NULL) {
		_registerSemanticError("Failed to create source node.");
		return NULL;
	}
	Statement * stmt = createSourceStatement(source);
	if (stmt == NULL) {
		_registerSemanticError("Failed to create source statement.");
		destroySource(source);
		return NULL;
	}
	return stmt;
}

Statement * ChartStatementSemanticAction(char * title, ChartType type, Source * sources, char * xColumn, Expression * yExpression, char * yAlias) {
	_logSyntacticAnalyzerAction(__FUNCTION__);
	Chart * chart = ChartSemanticAction(title, type, sources, xColumn, yExpression, yAlias);
	if (chart == NULL) {
		_registerSemanticError("Failed to create chart node.");
		return NULL;
	}
	Statement * stmt = createChartStatement(chart);
	if (stmt == NULL) {
		_registerSemanticError("Failed to create chart statement.");
		destroyChart(chart);
		return NULL;
	}
	return stmt;
}

Program * ProgramFromStatementsSemanticAction(Statement * statements) {
	_logSyntacticAnalyzerAction(__FUNCTION__);
	Program * program = createProgramFromStatements(statements);
	if (program == NULL) {
		// Si falla la creación, intentar crear un programa vacío
		program = calloc(1, sizeof(Program));
		if (program != NULL) {
			program->statements = statements;
			program->expression = NULL;
		} else {
			_registerSemanticError("Out of memory while creating program.");
		}
	}
	if (program != NULL) {
		_compilerState->abstractSyntaxtTree = program;
	}
	return program;
}

void SetCurrentChart(Chart * chart) {
	_currentChart = chart;
	if (_currentChart != NULL && _pendingYAlias != NULL) {
		_currentChart->yAlias = _pendingYAlias;
		_pendingYAlias = NULL;
	}
}

void SetChartId(char * id) {
	if (_currentChart != NULL && id != NULL) {
		_currentChart->id = id;
	} else if (id != NULL) {
		free(id);
	}
}

void SetChartYAlias(char * alias) {
	if (_currentChart != NULL && alias != NULL) {
		_currentChart->yAlias = strdup(alias);
		free(alias);
	} else if (alias != NULL) {
		if (_pendingYAlias != NULL) {
			free(_pendingYAlias);
		}
		_pendingYAlias = strdup(alias);
		free(alias);
	}
}

void SetChartOrientation(TokenLabel orientation) {
	if (_currentChart != NULL) {
		const char * orientationStr = NULL;
		if (orientation == VERTICAL) {
			orientationStr = "vertical";
		} else if (orientation == HORIZONTAL) {
			orientationStr = "horizontal";
		}
		if (orientationStr != NULL) {
			_currentChart->orientation = strdup(orientationStr);
		}
	}
}

void SetChartColors(char ** colors, size_t colorCount) {
	if (_currentChart != NULL && colors != NULL && colorCount > 0) {
		char ** copiedColors = calloc(colorCount + 1, sizeof(char*));
		if (copiedColors != NULL) {
			for (size_t i = 0; i < colorCount; ++i) {
				if (colors[i] != NULL) {
					copiedColors[i] = strdup(colors[i]);
					if (copiedColors[i] == NULL) {
						for (size_t j = 0; j < i; ++j) {
							free(copiedColors[j]);
						}
						free(copiedColors);
						return;
					}
				} else {
					copiedColors[i] = NULL;
				}
			}
			copiedColors[colorCount] = NULL;
			_currentChart->colors = copiedColors;
			_currentChart->colorCount = colorCount;
		}
	}
}

void SetChartSingleColor(char * color) {
	if (_currentChart != NULL && color != NULL) {
		_currentChart->singleColor = strdup(color);
	}
}

void SetChartXRange(double * range) {
	if (_currentChart != NULL && range != NULL) {
		if (_currentChart->xRange != NULL) {
			free(_currentChart->xRange);
		}
		double * copiedRange = calloc(2, sizeof(double));
		if (copiedRange != NULL) {
			copiedRange[0] = range[0];
			copiedRange[1] = range[1];
			_currentChart->xRange = copiedRange;
		}
	}
}

void SetChartYRange(double * range) {
	if (_currentChart != NULL && range != NULL) {
		if (_currentChart->yRange != NULL) {
			free(_currentChart->yRange);
		}
		double * copiedRange = calloc(2, sizeof(double));
		if (copiedRange != NULL) {
			copiedRange[0] = range[0];
			copiedRange[1] = range[1];
			_currentChart->yRange = copiedRange;
		}
	}
}

void SetChartLegendPosition(TokenLabel position) {
	if (_currentChart != NULL) {
		const char * positionStr = NULL;
		switch (position) {
			case TOP: positionStr = "top"; break;
			case BOTTOM: positionStr = "bottom"; break;
			case LEFT: positionStr = "left"; break;
			case RIGHT: positionStr = "right"; break;
			case OFF: positionStr = "off"; break;
			default: break;
		}
		if (positionStr != NULL) {
			_currentChart->legendPosition = strdup(positionStr);
		}
	}
}

void SetChartHole(double hole) {
	if (_currentChart != NULL) {
		_currentChart->hole = hole;
	}
}

const char * const * bisonSemanticErrors(void) {
	return (const char * const *) _semanticErrors;
}

size_t bisonSemanticErrorCount(void) {
	return _semanticErrorsCount;
}

bool bisonHasSemanticErrors(void) {
	return _semanticErrorsCount > 0;
}

bool ValidateProgramSemantics(Program * program) {
	bool ok = !bisonHasSemanticErrors();
	if (program == NULL || program->statements == NULL) {
		return ok;
	}

	size_t idCount = 0;
	size_t idCapacity = 4;
	char ** seenIds = (char **) calloc(idCapacity, sizeof(char *));
	if (seenIds == NULL) {
		_registerSemanticError("Out of memory while validating chart identifiers.");
		return false;
	}

	Statement * stmt = program->statements;
	while (stmt != NULL) {
		if (stmt->type == STMT_CHART && stmt->chart != NULL && stmt->chart->id != NULL) {
			const char * currentId = stmt->chart->id;
			bool duplicateFound = false;
			for (size_t i = 0; i < idCount; ++i) {
				if (seenIds[i] != NULL && strcmp(seenIds[i], currentId) == 0) {
					_registerSemanticError("Duplicate chart identifier \"%s\".", currentId);
					ok = false;
					duplicateFound = true;
					break;
				}
			}
			if (!duplicateFound) {
				if (idCount == idCapacity) {
					size_t newCapacity = idCapacity * 2;
					char ** resized = (char **) realloc(seenIds, newCapacity * sizeof(char *));
					if (resized == NULL) {
						_registerSemanticError("Out of memory while tracking chart identifiers.");
						ok = false;
						break;
					}
					seenIds = resized;
					for (size_t j = idCapacity; j < newCapacity; ++j) {
						seenIds[j] = NULL;
					}
					idCapacity = newCapacity;
				}
				seenIds[idCount++] = (char *) currentId;
			}
		}
		stmt = stmt->next;
	}

	if (seenIds != NULL) {
		free(seenIds);
	}

	return ok && !bisonHasSemanticErrors();
}
