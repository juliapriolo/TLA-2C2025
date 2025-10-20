%{

#include "../../support/type/TokenLabel.h"
#include "AbstractSyntaxTree.h"
#include "BisonActions.h"

void yyerror(const YYLTYPE * location, const char * message) {
    fprintf(stderr, "Parse error at line %d: %s\n", location->first_line, message);
}

%}

%define api.pure full
%define api.push-pull push
%define api.value.union.name BisonSemanticValue
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
%token <token> ADD SUB MUL DIV OPEN_PARENTHESIS CLOSE_PARENTHESIS OPEN_BRACE CLOSE_BRACE OPEN_COMMENT CLOSE_COMMENT
%token <token> IGNORED UNKNOWN
%token <token> SOURCE CHART FROM SELECT WHERE AS TYPE X Y COLORS COLOR_KW LEGEND HOLE ID_KW ORIENTATION RANGE AVG MIN MAX COUNT SUM FILTER PROJECT AVERAGE TOP BOTTOM LEFT RIGHT OFF AGGREGATE
%token <token> PIE DONUT BAR SCATTER LINE VERTICAL HORIZONTAL
%token <stringValue> IDENTIFIER
%token <stringValue> STRING
%token <numberValue> NUMBER
%token <colorValue> COLOR
%token <token> EQ COMMA SEMI COLON LBRACK RBRACK GT LT GE LE EQEQ DOT

%type <constant> constant
%type <expression> expression
%type <factor> factor
%type <program> program
%type <program> stmt stmt_list
%type <stringValue> source_decl chart_decl chart_body chart_option string_list color_list
%type <token> filter_clause project_clause chart_type orientation legend_position aggregate_function

%left ADD SUB
%left MUL DIV

%%

program: stmt_list
       | expression                       { $$ = ExpressionProgramSemanticAction($1); }
       ;

stmt_list: /* empty */                     { $$ = NULL; }
         | stmt_list stmt                   { $$ = $2; }
         ;

stmt: source_decl SEMI                      { $$ = SourceProgramSemanticAction($1, NULL); }
    | chart_decl SEMI                       { $$ = ChartProgramSemanticAction($1); }
    ;

source_decl: SOURCE IDENTIFIER EQ FROM STRING { $$ = $2; }
           | SOURCE IDENTIFIER EQ FROM STRING filter_clause { $$ = $2; }
           | SOURCE IDENTIFIER EQ FROM STRING project_clause { $$ = $2; }
           | SOURCE IDENTIFIER EQ FROM STRING filter_clause project_clause { $$ = $2; }
           | SOURCE IDENTIFIER EQ FROM STRING project_clause filter_clause { $$ = $2; }
           ;

filter_clause: FILTER STRING EQEQ STRING    { $$ = FILTER; }
             | FILTER STRING GT STRING       { $$ = FILTER; }
             | FILTER STRING LT STRING       { $$ = FILTER; }
             | FILTER STRING GE STRING       { $$ = FILTER; }
             | FILTER STRING LE STRING       { $$ = FILTER; }
             ;

project_clause: PROJECT LBRACK string_list RBRACK { $$ = PROJECT; }
              ;

string_list: STRING                          { $$ = $1; }
           | string_list COMMA STRING        { $$ = $3; }
           ;

chart_decl: CHART STRING TYPE chart_type COLON chart_body { $$ = $2; }
          ;

chart_type: PIE                              { $$ = PIE; }
          | DONUT                            { $$ = DONUT; }
          | BAR                              { $$ = BAR; }
          | SCATTER                          { $$ = SCATTER; }
          | LINE                             { $$ = LINE; }
          ;

chart_body: /* empty */                      { $$ = NULL; }
          | chart_body chart_option          { $$ = $2; }
          ;

chart_option: FROM STRING COMMA              { $$ = $2; }
            | FROM IDENTIFIER COMMA          { $$ = $2; }
            | X EQ STRING COMMA              { $$ = $3; }
            | Y EQ STRING COMMA              { $$ = $3; }
            | ORIENTATION EQ orientation COMMA { $$ = NULL; }
            | COLORS EQ LBRACK color_list RBRACK COMMA { $$ = $4; }
            | LEGEND EQ legend_position COMMA { $$ = NULL; }
            | HOLE EQ NUMBER COMMA           { $$ = NULL; }
            | ID_KW EQ IDENTIFIER COMMA      { $$ = $3; }
            | RANGE EQ LBRACK NUMBER COMMA NUMBER RBRACK COMMA { $$ = NULL; }
            | FROM STRING                     { $$ = $2; }
            | FROM IDENTIFIER                 { $$ = $2; }
            | X EQ STRING                     { $$ = $3; }
            | Y EQ STRING                     { $$ = $3; }
            | ORIENTATION EQ orientation      { $$ = NULL; }
            | COLORS EQ LBRACK color_list RBRACK { $$ = $4; }
            | LEGEND EQ legend_position       { $$ = NULL; }
            | HOLE EQ NUMBER                  { $$ = NULL; }
            | ID_KW EQ IDENTIFIER             { $$ = $3; }
            | RANGE EQ LBRACK NUMBER COMMA NUMBER RBRACK { $$ = NULL; }
            ;

orientation: VERTICAL                        { $$ = VERTICAL; }
           | HORIZONTAL                      { $$ = HORIZONTAL; }
           ;

legend_position: TOP                         { $$ = TOP; }
                | BOTTOM                     { $$ = BOTTOM; }
                | LEFT                       { $$ = LEFT; }
                | RIGHT                      { $$ = RIGHT; }
                | OFF                        { $$ = OFF; }
                ;

color_list: COLOR                            { $$ = $1; }
          | color_list COMMA COLOR           { $$ = $3; }
          ;

expression: expression ADD expression       { $$ = ArithmeticExpressionSemanticAction($1, $3, ADDITION); }
          | expression SUB expression       { $$ = ArithmeticExpressionSemanticAction($1, $3, SUBTRACTION); }
          | expression MUL expression       { $$ = ArithmeticExpressionSemanticAction($1, $3, MULTIPLICATION); }
          | expression DIV expression       { $$ = ArithmeticExpressionSemanticAction($1, $3, DIVISION); }
          | STRING AS STRING                 { $$ = NULL; /* column alias */ }
          | aggregate_function               { $$ = NULL; /* aggregate function */ }
          | factor                          { $$ = FactorExpressionSemanticAction($1); }
          ;

aggregate_function: AVG OPEN_PARENTHESIS STRING CLOSE_PARENTHESIS    { $$ = AVG; }
                  | MIN OPEN_PARENTHESIS STRING CLOSE_PARENTHESIS    { $$ = MIN; }
                  | MAX OPEN_PARENTHESIS STRING CLOSE_PARENTHESIS    { $$ = MAX; }
                  | COUNT OPEN_PARENTHESIS STRING CLOSE_PARENTHESIS   { $$ = COUNT; }
                  | SUM OPEN_PARENTHESIS STRING CLOSE_PARENTHESIS    { $$ = SUM; }
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