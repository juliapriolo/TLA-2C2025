#include "BisonActions.h"
#include <stdio.h>
#include <string.h>

/* MODULE INTERNAL STATE */

static CompilerState * _compilerState = NULL;
static Logger * _logger = NULL;
static bool _semanticErrorDetected = false;
static char ** _sourceIdentifiers = NULL;
static size_t _sourceIdentifiersCount = 0;
static Chart * _currentChart = NULL;  // Chart actual mientras se procesan opciones
static char * _pendingYAlias = NULL;  // Alias Y pendiente de asignar al chart cuando se cree

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

/** Shutdown module's internal state. */
void _shutdownBisonActionsModule() {
	if (_logger != NULL) {
		logDebugging(_logger, "Destroying module: BisonActions...");
		destroyLogger(_logger);
		_logger = NULL;
	}
	_clearSourceIdentifiers();
	_semanticErrorDetected = false;
	_compilerState = NULL;
	// Liberar alias pendiente si existe
	if (_pendingYAlias != NULL) {
		free(_pendingYAlias);
		_pendingYAlias = NULL;
	}
}

ModuleDestructor initializeBisonActionsModule(CompilerState * compilerState) {
	_compilerState = compilerState;
	_logger = createLogger("BisonActions");
	_clearSourceIdentifiers();
	_semanticErrorDetected = false;
	return _shutdownBisonActionsModule;
}

/* IMPORTED FUNCTIONS */

/* PRIVATE FUNCTIONS */

static void _logSyntacticAnalyzerAction(const char * functionName);

/**
 * Logs a syntactic-analyzer action in DEBUGGING level.
 */
static void _logSyntacticAnalyzerAction(const char * functionName) {
	logDebugging(_logger, "%s", functionName);
}

/* PUBLIC FUNCTIONS */

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
			return AGG_AVERAGE; // default
	}
}

Program * ExpressionProgramSemanticAction(Expression * expression) {
	_logSyntacticAnalyzerAction(__FUNCTION__);
	Program * program = calloc(1, sizeof(Program));
	program->expression = expression;
	_compilerState->abstractSyntaxtTree = program;
	return program;
}

// Funciones auxiliares para convertir tokens a enums

ChartType ChartTypeSemanticAction(TokenLabel token) {
	switch (token) {
		case BAR: return CHART_BAR;
		case PIE: return CHART_PIE;
		case DONUT: return CHART_DONUT;
		case SCATTER: return CHART_SCATTER;
		case LINE: return CHART_LINE;
		default: return CHART_BAR; // default
	}
}

FilterOperator FilterOperatorSemanticAction(TokenLabel token) {
	switch (token) {
		case EQEQ: return FILTER_EQ;
		case GT: return FILTER_GT;
		case LT: return FILTER_LT;
		case GE: return FILTER_GE;
		case LE: return FILTER_LE;
		default: return FILTER_EQ; // default
	}
}

// Construcción de nodos del AST

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
			_semanticErrorDetected = true;
			if (_logger) {
				logError(_logger, "Duplicate source identifier \"%s\".", identifier);
			}
			return NULL;
		} else {
			_rememberSourceIdentifier(identifier);
		}
	}
	// Validar que si se usa sourceIdentifier, ese source existe
	if (sourceIdentifier != NULL && !_sourceIdentifierExists(sourceIdentifier)) {
		_semanticErrorDetected = true;
		if (_logger) {
			logError(_logger, "Source identifier \"%s\" not found.", sourceIdentifier);
		}
		return NULL;
	}
	return createSource(identifier, csvFile, sourceIdentifier, filters, projection);
}

Chart * ChartSemanticAction(char * title, ChartType type, Source * sources, char * xColumn, Expression * yExpression, char * yAlias) {
	_logSyntacticAnalyzerAction(__FUNCTION__);
	// Validar que el chart tenga al menos un source
	if (sources == NULL) {
		_semanticErrorDetected = true;
		if (_logger) {
			logError(_logger, "Chart must have at least one source.");
		}
		return NULL;
	}
	return createChart(title, type, sources, xColumn, yExpression, yAlias);
}

Statement * SourceStatementSemanticAction(char * sourceId, char * csvFile, char * sourceIdentifier, FilterCondition * filters, Projection * projection) {
	_logSyntacticAnalyzerAction(__FUNCTION__);
	Source * source = SourceSemanticAction(sourceId, csvFile, sourceIdentifier, filters, projection);
	if (source == NULL) {
		_semanticErrorDetected = true;
		if (_logger) {
			logError(_logger, "Failed to create source node.");
		}
		return NULL;
	}
	Statement * stmt = createSourceStatement(source);
	if (stmt == NULL) {
		_semanticErrorDetected = true;
		if (_logger) {
			logError(_logger, "Failed to create source statement.");
		}
		destroySource(source);
		return NULL;
	}
	return stmt;
}

Statement * ChartStatementSemanticAction(char * title, ChartType type, Source * sources, char * xColumn, Expression * yExpression, char * yAlias) {
	_logSyntacticAnalyzerAction(__FUNCTION__);
	Chart * chart = ChartSemanticAction(title, type, sources, xColumn, yExpression, yAlias);
	if (chart == NULL) {
		_semanticErrorDetected = true;
		if (_logger) {
			logError(_logger, "Failed to create chart node.");
		}
		return NULL;
	}
	Statement * stmt = createChartStatement(chart);
	if (stmt == NULL) {
		_semanticErrorDetected = true;
		if (_logger) {
			logError(_logger, "Failed to create chart statement.");
		}
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
			_semanticErrorDetected = true;
			if (_logger) {
				logError(_logger, "Out of memory while creating program.");
			}
		}
	}
	if (program != NULL) {
		_compilerState->abstractSyntaxtTree = program;
	}
	return program;
}

void SetCurrentChart(Chart * chart) {
	_currentChart = chart;
	// Si hay un alias Y pendiente, asignarlo ahora
	if (_currentChart != NULL && _pendingYAlias != NULL) {
		_currentChart->yAlias = _pendingYAlias;
		_pendingYAlias = NULL;
	}
}

void SetChartId(char * id) {
	if (_currentChart != NULL && id != NULL) {
		_currentChart->id = id;
	} else if (id != NULL) {
		// Si no hay chart actual, liberar el id para evitar leak
		free(id);
	}
}

void SetChartYAlias(char * alias) {
	if (_currentChart != NULL && alias != NULL) {
		// Copiar el alias para que sea propiedad del chart
		_currentChart->yAlias = strdup(alias);
		// Liberar el original ya que lo copiamos
		free(alias);
	} else if (alias != NULL) {
		// Si no hay chart actual todavía, guardar el alias para asignarlo después
		// Liberar cualquier alias pendiente anterior
		if (_pendingYAlias != NULL) {
			free(_pendingYAlias);
		}
		_pendingYAlias = strdup(alias);
		// Liberar el original ya que lo copiamos
		free(alias);
	}
}

void SetChartOrientation(TokenLabel orientation) {
	if (_currentChart != NULL) {
		// Convertir el token a string
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
		// Copiar el array de colores
		char ** copiedColors = calloc(colorCount + 1, sizeof(char*));
		if (copiedColors != NULL) {
			for (size_t i = 0; i < colorCount; ++i) {
				if (colors[i] != NULL) {
					copiedColors[i] = strdup(colors[i]);
					if (copiedColors[i] == NULL) {
						// Si falla, liberar lo que ya se copió
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
		// Liberar rango anterior si existe
		if (_currentChart->xRange != NULL) {
			free(_currentChart->xRange);
		}
		// Copiar el rango
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
		// Liberar rango anterior si existe
		if (_currentChart->yRange != NULL) {
			free(_currentChart->yRange);
		}
		// Copiar el rango
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
		// Convertir el token a string
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

bool bisonHasSemanticErrors(void) {
	return _semanticErrorDetected;
}
