%{

#include "../../support/type/TokenLabel.h"
#include "AbstractSyntaxTree.h"
#include "BisonActions.h"

void yyerror(const YYLTYPE * location, const char * message) {}

%}

%define api.pure full
%define api.push-pull push
%define api.value.union.name SemanticValue
%define parse.error detailed
%locations

%union {
    signed int integer;
    TokenLabel token;
    char * stringValue;
    double numberValue;
    char * colorValue;

    Constant * constant;
    Expression * expression;
    Factor * factor;
    Program * program;
}

%destructor { destroyConstant($$); } <constant>
%destructor { destroyExpression($$); } <expression>
%destructor { destroyFactor($$); } <factor>

%token <integer> INTEGER
%token <token> ADD SUB MUL DIV OPEN_PARENTHESIS CLOSE_PARENTHESIS
%token <token> IGNORED UNKNOWN
%token <token> SOURCE CHART FROM SELECT WHERE AS TYPE X Y COLORS COLOR_KW LEGEND HOLE ID_KW ORIENTATION RANGE AVG MIN MAX COUNT SUM
%token <token> PIE DONUT BAR SCATTER LINE VERTICAL HORIZONTAL TOP BOTTOM LEFT RIGHT
%token <stringValue> IDENTIFIER
%token <stringValue> STRING
%token <numberValue> NUMBER
%token <colorValue> COLOR
%token <token> EQ COMMA SEMI COLON LBRACK RBRACK GT LT GE LE EQEQ DOT

%type <constant> constant
%type <expression> expression
%type <factor> factor
%type <program> program

%left ADD SUB
%left MUL DIV

%%

program: stmt_list
       | expression                       { $$ = ExpressionProgramSemanticAction($1); }
       ;

stmt_list: /* empty */
         | stmt_list stmt
         ;

stmt: source_decl SEMI                  { $$ = SourceProgramSemanticAction($1->string, NULL); /* stub */ }
    | chart_decl SEMI                    { $$ = ChartProgramSemanticAction($1->string); /* stub */ }
    ;

source_decl: SOURCE IDENTIFIER FROM IDENTIFIER { /* stub: return a program wrapping source */ $$ = NULL; }
           ;

chart_decl: CHART IDENTIFIER COLON chart_body { /* stub */ $$ = NULL; }
          ;

chart_body: /* zero or more chart options */
          | chart_body chart_option
          ;

chart_option: TYPE EQ IDENTIFIER
            | FROM EQ IDENTIFIER
            | SELECT EQ LBRACK select_list RBRACK
            | FILTER EQ expression
            | PROJECT EQ LBRACK select_list RBRACK
            | AGGREGATE EQ aggregate_clause
            | COLORS EQ LBRACK color_list RBRACK
            | LEGEND EQ STRING
            | ORIENTATION EQ IDENTIFIER
            | RANGE EQ LBRACK NUMBER COMMA NUMBER RBRACK
            | HOLE EQ NUMBER
            | ID_KW EQ IDENTIFIER
            ;

select_list: IDENTIFIER
           | select_list COMMA IDENTIFIER
           ;

aggregate_clause: AVG LBRACK IDENTIFIER RBRACK
                | MIN LBRACK IDENTIFIER RBRACK
                | MAX LBRACK IDENTIFIER RBRACK
                | COUNT LBRACK IDENTIFIER RBRACK
                | SUM LBRACK IDENTIFIER RBRACK
                ;

color_list: COLOR
          | color_list COMMA COLOR
          ;

expression: expression ADD expression    { $$ = ArithmeticExpressionSemanticAction($1, $3, ADDITION); }
          | expression DIV expression    { $$ = ArithmeticExpressionSemanticAction($1, $3, DIVISION); }
          | expression MUL expression    { $$ = ArithmeticExpressionSemanticAction($1, $3, MULTIPLICATION); }
          | expression SUB expression    { $$ = ArithmeticExpressionSemanticAction($1, $3, SUBTRACTION); }
          | factor                        { $$ = FactorExpressionSemanticAction($1); }
          ;

factor: OPEN_PARENTHESIS expression CLOSE_PARENTHESIS { $$ = ExpressionFactorSemanticAction($2); }
      | constant                                      { $$ = ConstantFactorSemanticAction($1); }
      ;

constant: INTEGER                { $$ = IntegerConstantSemanticAction($1); }
        | COLOR                  { $$ = ColorConstantSemanticAction($1); }
        | STRING                 { $$ = StringConstantSemanticAction($1); }
        | NUMBER                 { $$ = NumberConstantSemanticAction($1); }
        ;

%%
%{

#include "../../support/type/TokenLabel.h"
#include "AbstractSyntaxTree.h"
#include "BisonActions.h"

/**
 * The error reporting function for Bison parser.
 *
 * @todo Add location to the grammar and "pushToken" API function.
 *
 * @see https://www.gnu.org/software/bison/manual/html_node/Error-Reporting-Function.html
 * @see https://www.gnu.org/software/bison/manual/html_node/Tracking-Locations.html
 */
void yyerror(const YYLTYPE * location, const char * message) {}

%}

// You touch this, and you die.
%define api.pure full
%define api.push-pull push
%define api.value.union.name SemanticValue
%define parse.error detailed
%locations

%union {
	/** Terminals. */

	signed int integer;
	TokenLabel token;
	char * stringValue;
	double numberValue;
	char * colorValue;

	/** Non-terminals. */

	Constant * constant;
	Expression * expression;
	Factor * factor;
	Program * program;
}

/**
 * Destructors. This functions are executed after the parsing ends, so if the
 * AST must be used in the following phases of the compiler you shouldn't used
 * this approach for the AST root node ("program" non-terminal, in this
 * grammar), or it will drop the entire tree even if the parsing succeeds.
 *
 * @see https://www.gnu.org/software/bison/manual/html_node/Destructor-Decl.html
 */
%destructor { destroyConstant($$); } <constant>
%destructor { destroyExpression($$); } <expression>
%destructor { destroyFactor($$); } <factor>

/** Terminals. */
%token <integer> INTEGER
%token <token> ADD
%token <token> CLOSE_BRACE
%token <token> CLOSE_COMMENT
%token <token> CLOSE_PARENTHESIS
%token <token> DIV
%token <token> MUL
%token <token> OPEN_BRACE
%token <token> OPEN_COMMENT
%token <token> OPEN_PARENTHESIS
%token <token> SUB

%token <token> IGNORED
%token <token> UNKNOWN

%token <token> SOURCE CHART FROM SELECT WHERE AS TYPE X Y COLORS COLOR_KW LEGEND HOLE ID_KW ORIENTATION RANGE AVG MIN MAX COUNT SUM
%token <token> PIE DONUT BAR SCATTER LINE VERTICAL HORIZONTAL TOP BOTTOM LEFT RIGHT
%token <stringValue> IDENTIFIER
%token <stringValue> STRING
%token <numberValue> NUMBER
%token <colorValue> COLOR
%token <token> EQ COMMA SEMI COLON LBRACK RBRACK GT LT GE LE EQEQ DOT

/** Non-terminals. */
%type <constant> constant
%type <expression> expression
%type <factor> factor
%type <program> program

/**
 * Precedence and associativity.
 *
 * @see https://en.cppreference.com/w/cpp/language/operator_precedence.html
 * @see https://www.gnu.org/software/bison/manual/html_node/Precedence.html
 */
%left ADD SUB
%left MUL DIV

%%

// IMPORTANT: To use λ in the following grammar, use the %empty symbol.

program: expression											{ $$ = ExpressionProgramSemanticAction($1); }
	;

expression: expression[left] ADD expression[right]			{ $$ = ArithmeticExpressionSemanticAction($left, $right, ADDITION); }
	| expression[left] DIV expression[right]				{ $$ = ArithmeticExpressionSemanticAction($left, $right, DIVISION); }
	| expression[left] MUL expression[right]				{ $$ = ArithmeticExpressionSemanticAction($left, $right, MULTIPLICATION); }
	| expression[left] SUB expression[right]				{ $$ = ArithmeticExpressionSemanticAction($left, $right, SUBTRACTION); }
	| factor												{ $$ = FactorExpressionSemanticAction($1); }
	;

factor: OPEN_PARENTHESIS expression CLOSE_PARENTHESIS		{ $$ = ExpressionFactorSemanticAction($2); }
	| constant												{ $$ = ConstantFactorSemanticAction($1); }
	;

constant: INTEGER                { $$ = IntegerConstantSemanticAction($1); }
        | COLOR                  { $$ = ColorConstantSemanticAction($1); }
        | STRING                 { $$ = StringConstantSemanticAction($1); }
        | NUMBER                 { $$ = NumberConstantSemanticAction($1); }
        ;
%%
