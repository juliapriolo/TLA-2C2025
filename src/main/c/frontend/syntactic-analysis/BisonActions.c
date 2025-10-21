#include "BisonActions.h"
#include <string.h>

/* MODULE INTERNAL STATE */

static CompilerState * _compilerState = NULL;
static Logger * _logger = NULL;
static bool _semanticErrorDetected = false;
static char ** _sourceIdentifiers = NULL;
static size_t _sourceIdentifiersCount = 0;

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

Program * ExpressionProgramSemanticAction(Expression * expression) {
	_logSyntacticAnalyzerAction(__FUNCTION__);
	Program * program = calloc(1, sizeof(Program));
	program->expression = expression;
	_compilerState->abstractSyntaxtTree = program;
	return program;
}

Program * SourceProgramSemanticAction(char * sourceId, char * fromId) {
	_logSyntacticAnalyzerAction(__FUNCTION__);
	if (sourceId != NULL) {
		if (_sourceIdentifierExists(sourceId)) {
			_semanticErrorDetected = true;
			if (_logger) {
				logError(_logger, "Duplicate source identifier \"%s\".", sourceId);
			}
		} else {
			_rememberSourceIdentifier(sourceId);
		}
		free(sourceId);
	}
	if (fromId != NULL) {
		free(fromId);
	}
	/* Minimal stub: create an empty Program node (no expression) and attach to compiler state.
	   Full implementation should create a SourceDecl node and attach it to a statements list. */
	Program * program = calloc(1, sizeof(Program));
	program->expression = NULL;
	_compilerState->abstractSyntaxtTree = program;
	return program;
}

Program * ChartProgramSemanticAction(char * chartId) {
	_logSyntacticAnalyzerAction(__FUNCTION__);
	/* Minimal stub: create an empty Program node. */
	Program * program = calloc(1, sizeof(Program));
	program->expression = NULL;
	_compilerState->abstractSyntaxtTree = program;
	if (chartId != NULL) {
		free(chartId);
	}
	return program;
}

bool bisonHasSemanticErrors(void) {
	return _semanticErrorDetected;
}
