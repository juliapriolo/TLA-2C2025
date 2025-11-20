#ifndef ABSTRACT_SYNTAX_TREE_HEADER
#define ABSTRACT_SYNTAX_TREE_HEADER

#include "../../support/logging/Logger.h"
#include "../../support/type/ModuleDestructor.h"
#include <stdlib.h>

/** Initialize module's internal state. */
ModuleDestructor initializeAbstractSyntaxTreeModule();

/**
 * This type definitions allows self-referencing types (e.g., an expression
 * that is made of another expressions, such as talking about you in 3rd
 * person, but without the madness).
 */

typedef enum ExpressionType ExpressionType;
typedef enum FactorType FactorType;

typedef struct Constant Constant;
typedef struct Expression Expression;
typedef struct Factor Factor;
typedef struct Program Program;

// Nuevos tipos para el DSL de gráficos
typedef enum ChartType ChartType;
typedef enum FilterOperator FilterOperator;
typedef enum FilterValueType FilterValueType;
typedef enum AggregateFunction AggregateFunction;
typedef enum StatementType StatementType;

typedef struct FilterCondition FilterCondition;
typedef struct Projection Projection;
typedef struct Source Source;
typedef struct Chart Chart;
typedef struct Statement Statement;

/**
 * Node types for the Abstract Syntax Tree (AST).
 */

enum ExpressionType {
	ADDITION,
	DIVISION,
	FACTOR,
	MULTIPLICATION,
	SUBTRACTION
};

enum FactorType {
	CONSTANT,
	EXPRESSION,
	AGGREGATE_FACTOR
};

// Funciones de agregación (definidas antes de Factor para que pueda usarlas)
enum AggregateFunction {
	AGG_AVERAGE,
	AGG_MIN,
	AGG_MAX,
	AGG_COUNT,
	AGG_SUM
};

struct Constant {
	int value;         // Para INTEGER
	char *color;       // Para COLOR (#xxxxxx)
	char *string;      // Para STRING
	double number;     // Para NUMBER
};

struct Factor {
	union {
		Constant * constant;
		Expression * expression;
		struct {
			AggregateFunction function;
			char * columnName;
		} aggregate;
	};
	FactorType type;
};

struct Expression {
	union {
		Factor * factor;
		struct {
			Expression * leftExpression;
			Expression * rightExpression;
		};
	};
	ExpressionType type;
};

// Tipos de gráfico
enum ChartType {
	CHART_BAR,
	CHART_PIE,
	CHART_DONUT,
	CHART_SCATTER,
	CHART_LINE
};

// Operadores de filtro
enum FilterOperator {
	FILTER_EQ,      // ==
	FILTER_GT,      // >
	FILTER_LT,      // <
	FILTER_GE,      // >=
	FILTER_LE       // <=
};

// Tipo de valor en FilterCondition
enum FilterValueType {
	FILTER_VALUE_STRING,
	FILTER_VALUE_INT
};

// Funciones de agregación (definidas arriba, antes de Factor)

// Tipo de statement
enum StatementType {
	STMT_SOURCE,
	STMT_CHART
};

// Nodo de condición de filtro
struct FilterCondition {
	char * columnName;
	FilterOperator operator;
	enum FilterValueType valueType;  // Indica el tipo del valor en el union
	union {
		char * stringValue;
		int intValue;
		double numberValue;
	} value;
	struct FilterCondition * next;  // Para múltiples filtros (AND)
};

// Nodo de proyección
struct Projection {
	char ** columns;        // Array de nombres de columnas
	size_t columnCount;
};

// Nodo de Source
struct Source {
	char * identifier;              // Nombre de la source
	char * csvFile;                 // Archivo CSV o NULL si viene de otra source
	char * sourceIdentifier;        // ID de otra source (si es composición)
	FilterCondition * filters;      // Lista de filtros
	Projection * projection;        // Columnas a proyectar
	struct Source * next;            // Para múltiples sources en un chart
};

// Nodo de Chart
struct Chart {
	char * title;                   // Título del gráfico
	ChartType type;                 // Tipo de gráfico
	Source * sources;                // Fuente(s) de datos
	char * xColumn;                  // Columna para eje X
	Expression * yExpression;       // Expresión para eje Y
	char * yAlias;                  // Alias para Y (si tiene "as")
	
	// Opciones visuales
	char ** colors;                 // Array de colores hex
	size_t colorCount;
	char * singleColor;             // Color único (para scatter)
	char * legendPosition;          // "top", "bottom", "left", "right", "off"
	char * orientation;              // "vertical", "horizontal"
	double hole;                    // Para donut charts
	char * id;                      // ID del canvas
	double * xRange;                // [min, max] para scatter
	double * yRange;                // [min, max] para scatter
};

// Nodo de Statement (puede ser Source o Chart)
struct Statement {
	StatementType type;
	union {
		Source * source;
		Chart * chart;
	};
	struct Statement * next;        // Lista de statements
};

// Program extendido
struct Program {
	Statement * statements;          // Lista de sources y charts
	Expression * expression;        // Para compatibilidad con calculadora (puede ser NULL)
};

/**
 * Node recursive super-duper-trambolik-destructors.
 */

void destroyConstant(Constant * constant);
void destroyExpression(Expression * expression);
void destroyFactor(Factor * factor);
void destroyProgram(Program * program);
void destroyFilterCondition(FilterCondition * filter);
void destroyProjection(Projection * projection);
void destroySource(Source * source);
void destroyChart(Chart * chart);
void destroyStatement(Statement * statement);

/* Constructors */
Constant * createIntegerConstant(int value);
Constant * createColorConstant(char * color);
Constant * createStringConstant(char * string);
Constant * createNumberConstant(double number);

Factor * createConstantFactor(Constant * constant);
Factor * createExpressionFactor(Expression * expression);
Factor * createAggregateFactor(AggregateFunction function, char * columnName);

Expression * createArithmeticExpression(Expression * left, Expression * right, ExpressionType type);
Expression * createFactorExpression(Factor * factor);

Program * createProgramFromExpression(Expression * expression);

// Nuevos constructores para DSL de gráficos
FilterCondition * createFilterCondition(char * columnName, FilterOperator op, char * stringValue);
FilterCondition * createFilterConditionInt(char * columnName, FilterOperator op, int intValue);
Projection * createProjection(char ** columns, size_t columnCount);
Source * createSource(char * identifier, char * csvFile, char * sourceIdentifier, FilterCondition * filters, Projection * projection);
Chart * createChart(char * title, ChartType type, Source * sources, char * xColumn, Expression * yExpression, char * yAlias);
Statement * createSourceStatement(Source * source);
Statement * createChartStatement(Chart * chart);
Program * createProgramFromStatements(Statement * statements);

// Funciones de debugging para imprimir el AST
void printAST(Program * program);
void printStatement(Statement * statement, int indent);
void printSource(Source * source, int indent);
void printChart(Chart * chart, int indent);

#endif
