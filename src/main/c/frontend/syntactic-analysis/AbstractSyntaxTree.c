#include "AbstractSyntaxTree.h"

/* MODULE INTERNAL STATE */

static Logger * _logger = NULL;

/** Shutdown module's internal state. */
void _shutdownAbstractSyntaxTreeModule() {
	if (_logger != NULL) {
		logDebugging(_logger, "Destroying module: AbstractSyntaxTree...");
		destroyLogger(_logger);
		_logger = NULL;
	}
}

ModuleDestructor initializeAbstractSyntaxTreeModule() {
	_logger = createLogger("AbstractSyntaxTree");
	return _shutdownAbstractSyntaxTreeModule;
}

/* Constructors */
Constant * createIntegerConstant(int value) {
	Constant * c = calloc(1, sizeof(Constant));
	if (!c) return NULL;
	c->value = value;
	return c;
}

Constant * createColorConstant(char * color) {
	Constant * c = calloc(1, sizeof(Constant));
	if (!c) return NULL;
	c->color = color;
	return c;
}

Constant * createStringConstant(char * string) {
	Constant * c = calloc(1, sizeof(Constant));
	if (!c) return NULL;
	c->string = string;
	return c;
}

Constant * createNumberConstant(double number) {
	Constant * c = calloc(1, sizeof(Constant));
	if (!c) return NULL;
	c->number = number;
	return c;
}

Factor * createConstantFactor(Constant * constant) {
	Factor * f = calloc(1, sizeof(Factor));
	if (!f) return NULL;
	f->constant = constant;
	f->type = CONSTANT;
	return f;
}

Factor * createExpressionFactor(Expression * expression) {
	Factor * f = calloc(1, sizeof(Factor));
	if (!f) return NULL;
	f->expression = expression;
	f->type = EXPRESSION;
	return f;
}

Expression * createArithmeticExpression(Expression * left, Expression * right, ExpressionType type) {
	Expression * e = calloc(1, sizeof(Expression));
	if (!e) return NULL;
	e->leftExpression = left;
	e->rightExpression = right;
	e->type = type;
	return e;
}

Expression * createFactorExpression(Factor * factor) {
	Expression * e = calloc(1, sizeof(Expression));
	if (!e) return NULL;
	e->factor = factor;
	e->type = FACTOR;
	return e;
}

Program * createProgramFromExpression(Expression * expression) {
	Program * p = calloc(1, sizeof(Program));
	if (!p) return NULL;
	p->expression = expression;
	return p;
}

/* PUBLIC FUNCTIONS */

void destroyConstant(Constant * constant) {
	logDebugging(_logger, "Executing destructor: %s", __FUNCTION__);
	if (constant != NULL) {
		if (constant->string != NULL) {
			free(constant->string);
			constant->string = NULL;
		}
		if (constant->color != NULL) {
			free(constant->color);
			constant->color = NULL;
		}
		free(constant);
	}
}

void destroyExpression(Expression * expression) {
	logDebugging(_logger, "Executing destructor: %s", __FUNCTION__);
	if (expression != NULL) {
		switch (expression->type) {
			case ADDITION:
			case DIVISION:
			case MULTIPLICATION:
			case SUBTRACTION:
				destroyExpression(expression->leftExpression);
				destroyExpression(expression->rightExpression);
				break;
			case FACTOR:
				destroyFactor(expression->factor);
				break;
		}
		free(expression);
	}
}

void destroyFactor(Factor * factor) {
	logDebugging(_logger, "Executing destructor: %s", __FUNCTION__);
	if (factor != NULL) {
		switch (factor->type) {
			case CONSTANT:
				destroyConstant(factor->constant);
				break;
			case EXPRESSION:
				destroyExpression(factor->expression);
				break;
		}
		free(factor);
	}
}

void destroyProgram(Program * program) {
	logDebugging(_logger, "Executing destructor: %s", __FUNCTION__);
	if (program != NULL) {
		destroyExpression(program->expression);
		free(program);
	}
}
