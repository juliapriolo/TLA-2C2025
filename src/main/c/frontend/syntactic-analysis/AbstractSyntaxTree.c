#include "AbstractSyntaxTree.h"
#include <stdio.h>
#include <string.h>

/* ESTADO INTERNO DEL MÓDULO */

static Logger * _logger = NULL;

/** Cierra el estado interno del módulo. */
void _shutdownAbstractSyntaxTreeModule() {
	if (_logger != NULL) {
		destroyLogger(_logger);
		_logger = NULL;
	}
}

ModuleDestructor initializeAbstractSyntaxTreeModule() {
	_logger = createLogger("AbstractSyntaxTree");
	return _shutdownAbstractSyntaxTreeModule;
}

/* Constructores */
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

Factor * createAggregateFactor(AggregateFunction function, char * columnName) {
	Factor * factor = calloc(1, sizeof(Factor));
	if (!factor) return NULL;
	factor->aggregate.function = function;
	factor->aggregate.columnName = columnName != NULL ? strdup(columnName) : NULL;
	factor->type = AGGREGATE_FACTOR;
	if (columnName != NULL && factor->aggregate.columnName == NULL) {
		free(factor);
		return NULL;
	}
	return factor;
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

/* FUNCIONES PÚBLICAS */

void destroyConstant(Constant * constant) {
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
	if (factor != NULL) {
		switch (factor->type) {
			case CONSTANT:
				destroyConstant(factor->constant);
				break;
			case EXPRESSION:
				destroyExpression(factor->expression);
				break;
			case AGGREGATE_FACTOR:
				if (factor->aggregate.columnName != NULL) {
					free(factor->aggregate.columnName);
				}
				break;
		}
		free(factor);
	}
}

/* Nuevos constructores para DSL de gráficos */

FilterCondition * createFilterCondition(char * columnName, FilterOperator op, char * stringValue) {
	FilterCondition * fc = calloc(1, sizeof(FilterCondition));
	if (!fc) return NULL;
	fc->columnName = columnName;
	fc->operator = op;
	fc->valueType = FILTER_VALUE_STRING;
	fc->value.stringValue = stringValue;
	fc->next = NULL;
	return fc;
}

FilterCondition * createFilterConditionInt(char * columnName, FilterOperator op, int intValue) {
	FilterCondition * fc = calloc(1, sizeof(FilterCondition));
	if (!fc) return NULL;
	fc->columnName = columnName;
	fc->operator = op;
	fc->valueType = FILTER_VALUE_INT;
	fc->value.intValue = intValue;
	fc->next = NULL;
	return fc;
}

FilterCondition * appendFilterCondition(FilterCondition * head, FilterCondition * tail) {
	if (head == NULL) {
		return tail;
	}
	FilterCondition * current = head;
	while (current->next != NULL) {
		current = current->next;
	}
	current->next = tail;
	return head;
}

Projection * createProjection(char ** columns, size_t columnCount) {
	if (columns == NULL && columnCount > 0) {
		return NULL;
	}
	Projection * p = calloc(1, sizeof(Projection));
	if (!p) {
		return NULL;
	}
	p->columns = columns;
	p->columnCount = columnCount;
	return p;
}

Source * createSource(char * identifier, char * csvFile, char * sourceIdentifier, FilterCondition * filters, Projection * projection) {
	Source * s = calloc(1, sizeof(Source));
	if (!s) return NULL;
	s->identifier = identifier;
	s->csvFile = csvFile;
	s->sourceIdentifier = sourceIdentifier;
	s->filters = filters;
	s->projection = projection;
	s->next = NULL;
	return s;
}

Chart * createChart(char * title, ChartType type, Source * sources, char * xColumn, Expression * yExpression, char * yAlias) {
	Chart * c = calloc(1, sizeof(Chart));
	if (!c) return NULL;
	c->title = title;
	c->type = type;
	c->sources = sources;
	c->xColumn = xColumn;
	c->yExpression = yExpression;
	c->yAlias = yAlias;
	c->colors = NULL;
	c->colorCount = 0;
	c->singleColor = NULL;
	c->legendPosition = NULL;
	c->orientation = NULL;
	c->hole = 0.0;
	c->id = NULL;
	c->xRange = NULL;
	c->yRange = NULL;
	return c;
}

Statement * createSourceStatement(Source * source) {
	if (source == NULL) {
		return NULL;
	}
	Statement * s = calloc(1, sizeof(Statement));
	if (!s) return NULL;
	s->type = STMT_SOURCE;
	s->source = source;
	s->next = NULL;
	return s;
}

Statement * createChartStatement(Chart * chart) {
	Statement * s = calloc(1, sizeof(Statement));
	if (!s) return NULL;
	s->type = STMT_CHART;
	s->chart = chart;
	s->next = NULL;
	return s;
}

Program * createProgramFromStatements(Statement * statements) {
	Program * p = calloc(1, sizeof(Program));
	if (!p) return NULL;
	p->statements = statements;
	p->expression = NULL;
	return p;
}

SourceOptions * createSourceOptions(FilterCondition * filters, Projection * projection) {
	SourceOptions * options = calloc(1, sizeof(SourceOptions));
	if (options == NULL) {
		destroyFilterCondition(filters);
		destroyProjection(projection);
		return NULL;
	}
	options->filters = filters;
	options->projection = projection;
	return options;
}

/* Destructores */

void destroyFilterCondition(FilterCondition * filter) {
	if (filter != NULL) {
		if (filter->columnName != NULL) {
			free(filter->columnName);
		}
		if (filter->valueType == FILTER_VALUE_STRING && filter->value.stringValue != NULL) {
			free(filter->value.stringValue);
		}
		destroyFilterCondition(filter->next);
		free(filter);
	}
}

void destroyProjection(Projection * projection) {
	if (projection != NULL) {
		if (projection->columns != NULL) {
			for (size_t i = 0; i < projection->columnCount; ++i) {
				if (projection->columns[i] != NULL) {
					free(projection->columns[i]);
				}
			}
			free(projection->columns);
		}
		free(projection);
	}
}

void destroySource(Source * source) {
	if (source != NULL) {
		if (source->identifier != NULL) {
			free(source->identifier);
		}
		if (source->csvFile != NULL) {
			free(source->csvFile);
		}
		if (source->sourceIdentifier != NULL) {
			free(source->sourceIdentifier);
		}
		destroyFilterCondition(source->filters);
		destroyProjection(source->projection);
		destroySource(source->next);
		free(source);
	}
}

void destroyChart(Chart * chart) {
	if (chart != NULL) {
		if (chart->title != NULL) {
			free(chart->title);
		}
		destroySource(chart->sources);
		if (chart->xColumn != NULL) {
			free(chart->xColumn);
		}
		destroyExpression(chart->yExpression);
		if (chart->yAlias != NULL) {
			free(chart->yAlias);
		}
		if (chart->colors != NULL) {
			for (size_t i = 0; i < chart->colorCount; ++i) {
				if (chart->colors[i] != NULL) {
					free(chart->colors[i]);
				}
			}
			free(chart->colors);
		}
		if (chart->singleColor != NULL) {
			free(chart->singleColor);
		}
		if (chart->legendPosition != NULL) {
			free(chart->legendPosition);
		}
		if (chart->orientation != NULL) {
			free(chart->orientation);
		}
		if (chart->id != NULL) {
			free(chart->id);
		}
		if (chart->xRange != NULL) {
			free(chart->xRange);
		}
		if (chart->yRange != NULL) {
			free(chart->yRange);
		}
		free(chart);
	}
}

void destroyStatement(Statement * statement) {
	if (statement != NULL) {
		switch (statement->type) {
			case STMT_SOURCE:
				destroySource(statement->source);
				break;
			case STMT_CHART:
				destroyChart(statement->chart);
				break;
		}
		destroyStatement(statement->next);
		free(statement);
	}
}

void destroyProgram(Program * program) {
	if (program != NULL) {
		destroyStatement(program->statements);
		destroyExpression(program->expression);
		free(program);
	}
}

/* Funciones de depuración */

static void _printIndent(int indent) {
	for (int i = 0; i < indent; ++i) {
		printf("  ");
	}
}

void printSource(Source * source, int indent) {
	if (source == NULL) {
		_printIndent(indent);
		printf("Source: NULL\n");
		return;
	}
	_printIndent(indent);
	printf("Source:\n");
	_printIndent(indent + 1);
	printf("identifier: %s\n", source->identifier ? source->identifier : "NULL");
	_printIndent(indent + 1);
	printf("csvFile: %s\n", source->csvFile ? source->csvFile : "NULL");
	_printIndent(indent + 1);
	printf("sourceIdentifier: %s\n", source->sourceIdentifier ? source->sourceIdentifier : "NULL");
	if (source->filters != NULL) {
		_printIndent(indent + 1);
		printf("filters:\n");
		FilterCondition * f = source->filters;
		while (f != NULL) {
			_printIndent(indent + 2);
			printf("- %s ", f->columnName ? f->columnName : "NULL");
			switch (f->operator) {
				case FILTER_EQ: printf("=="); break;
				case FILTER_GT: printf(">"); break;
				case FILTER_LT: printf("<"); break;
				case FILTER_GE: printf(">="); break;
				case FILTER_LE: printf("<="); break;
			}
			if (f->valueType == FILTER_VALUE_STRING) {
				printf(" \"%s\"\n", f->value.stringValue ? f->value.stringValue : "NULL");
			} else if (f->valueType == FILTER_VALUE_INT) {
				printf(" %d\n", f->value.intValue);
			} else {
				printf(" (unknown type)\n");
			}
			f = f->next;
		}
	}
	if (source->projection != NULL) {
		_printIndent(indent + 1);
		printf("projection: [");
		for (size_t i = 0; i < source->projection->columnCount; ++i) {
			if (i > 0) printf(", ");
			printf("\"%s\"", source->projection->columns[i] ? source->projection->columns[i] : "NULL");
		}
		printf("]\n");
	}
}

static void _printExpression(Expression * expr, int indent);

/* Función auxiliar para detectar si una expresión es solo un identificador simple */
static char * _extractSimpleIdentifier(Expression * expr) {
	if (expr == NULL || expr->type != FACTOR) {
		return NULL;
	}
	if (expr->factor == NULL) {
		return NULL;
	}
	if (expr->factor->type == CONSTANT && expr->factor->constant != NULL && expr->factor->constant->string != NULL) {
		return expr->factor->constant->string;
	}
	return NULL;
}

void printChart(Chart * chart, int indent) {
	if (chart == NULL) {
		_printIndent(indent);
		printf("Chart: NULL\n");
		return;
	}
	_printIndent(indent);
	printf("Chart:\n");
	_printIndent(indent + 1);
	printf("title: %s\n", chart->title ? chart->title : "NULL");
	_printIndent(indent + 1);
	printf("type: ");
	switch (chart->type) {
		case CHART_BAR: printf("BAR"); break;
		case CHART_PIE: printf("PIE"); break;
		case CHART_DONUT: printf("DONUT"); break;
		case CHART_SCATTER: printf("SCATTER"); break;
		case CHART_LINE: printf("LINE"); break;
	}
	printf("\n");
	_printIndent(indent + 1);
	printf("xColumn: %s\n", chart->xColumn ? chart->xColumn : "NULL");
	if (chart->yExpression != NULL) {
		char * simpleId = _extractSimpleIdentifier(chart->yExpression);
		if (simpleId != NULL) {
			_printIndent(indent + 1);
			printf("yColumn: %s\n", simpleId);
		} else {
			_printIndent(indent + 1);
			printf("yExpression:\n");
			_printExpression(chart->yExpression, indent + 2);
		}
	}
	_printIndent(indent + 1);
	printf("yAlias: %s\n", chart->yAlias ? chart->yAlias : "NULL");
	if (chart->id != NULL) {
		_printIndent(indent + 1);
		printf("id: %s\n", chart->id);
	}
	if (chart->orientation != NULL) {
		_printIndent(indent + 1);
		printf("orientation: %s\n", chart->orientation);
	}
	if (chart->legendPosition != NULL) {
		_printIndent(indent + 1);
		printf("legend: %s\n", chart->legendPosition);
	}
	if (chart->hole > 0.0) {
		_printIndent(indent + 1);
		printf("hole: %.2f\n", chart->hole);
	}
	if (chart->colors != NULL && chart->colorCount > 0) {
		_printIndent(indent + 1);
		printf("colors: [");
		for (size_t i = 0; i < chart->colorCount; ++i) {
			if (i > 0) printf(", ");
			printf("%s", chart->colors[i] ? chart->colors[i] : "NULL");
		}
		printf("]\n");
	}
	if (chart->singleColor != NULL) {
		_printIndent(indent + 1);
		printf("color: %s\n", chart->singleColor);
	}
	if (chart->xRange != NULL) {
		_printIndent(indent + 1);
		printf("xRange: [%.2f, %.2f]\n", chart->xRange[0], chart->xRange[1]);
	}
	if (chart->yRange != NULL) {
		_printIndent(indent + 1);
		printf("yRange: [%.2f, %.2f]\n", chart->yRange[0], chart->yRange[1]);
	}
	if (chart->sources != NULL) {
		_printIndent(indent + 1);
		printf("sources:\n");
		Source * s = chart->sources;
		while (s != NULL) {
			printSource(s, indent + 2);
			s = s->next;
		}
	}
}

static void _printExpression(Expression * expr, int indent) {
	if (expr == NULL) {
		_printIndent(indent);
		printf("NULL\n");
		return;
	}
	switch (expr->type) {
		case ADDITION:
		case SUBTRACTION:
		case MULTIPLICATION:
		case DIVISION:
			_printIndent(indent);
			printf("(");
			_printExpression(expr->leftExpression, 0);
			switch (expr->type) {
				case ADDITION: printf(" + "); break;
				case SUBTRACTION: printf(" - "); break;
				case MULTIPLICATION: printf(" * "); break;
				case DIVISION: printf(" / "); break;
				default: break;
			}
			_printExpression(expr->rightExpression, 0);
			printf(")\n");
			break;
		case FACTOR:
			if (expr->factor != NULL) {
				if (expr->factor->type == CONSTANT && expr->factor->constant != NULL) {
					_printIndent(indent);
					if (expr->factor->constant->string != NULL) {
						printf("\"%s\"\n", expr->factor->constant->string);
					} else if (expr->factor->constant->color != NULL) {
						printf("%s\n", expr->factor->constant->color);
					} else {
						printf("%d\n", expr->factor->constant->value);
					}
				} else if (expr->factor->type == EXPRESSION) {
					_printExpression(expr->factor->expression, indent);
				} else if (expr->factor->type == AGGREGATE_FACTOR) {
					_printIndent(indent);
					const char * funcName = NULL;
					switch (expr->factor->aggregate.function) {
						case AGG_AVERAGE: funcName = "average"; break;
						case AGG_MIN: funcName = "min"; break;
						case AGG_MAX: funcName = "max"; break;
						case AGG_COUNT: funcName = "count"; break;
						case AGG_SUM: funcName = "sum"; break;
					}
					if (funcName != NULL && expr->factor->aggregate.columnName != NULL) {
						printf("%s(\"%s\")\n", funcName, expr->factor->aggregate.columnName);
					} else {
						printf("NULL\n");
					}
				} else {
					_printIndent(indent);
					printf("NULL\n");
				}
			} else {
				_printIndent(indent);
				printf("NULL\n");
			}
			break;
	}
}

void printStatement(Statement * statement, int indent) {
	if (statement == NULL) {
		_printIndent(indent);
		printf("Statement: NULL\n");
		return;
	}
	_printIndent(indent);
	printf("Statement:\n");
	switch (statement->type) {
		case STMT_SOURCE:
			printSource(statement->source, indent + 1);
			break;
		case STMT_CHART:
			printChart(statement->chart, indent + 1);
			break;
	}
}

void printAST(Program * program) {
	if (program == NULL) {
		printf("AST: NULL\n");
		return;
	}
	printf("=== AST ===\n");
	if (program->statements != NULL) {
		Statement * s = program->statements;
		int count = 0;
		while (s != NULL) {
			printf("\n--- Statement %d ---\n", count++);
			printStatement(s, 0);
			s = s->next;
		}
	} else if (program->expression != NULL) {
		printf("Expression-based program (calculator mode)\n");
	} else {
		printf("Empty program\n");
	}
	printf("==========\n");
}
