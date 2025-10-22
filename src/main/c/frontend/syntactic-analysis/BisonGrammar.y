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
%destructor { free($$); } <stringValue>
%destructor { free($$); } <colorValue>

%token <integer> INTEGER
%token <token> ADD SUB MUL DIV OPEN_PARENTHESIS CLOSE_PARENTHESIS OPEN_BRACE CLOSE_BRACE OPEN_COMMENT CLOSE_COMMENT
%token <token> IGNORED UNKNOWN
%token <token> SOURCE CHART FROM SELECT WHERE AS TYPE X Y COLORS COLOR_KW LEGEND HOLE ID_KW ORIENTATION RANGE AVG MIN MAX COUNT SUM FILTER PROJECT AVERAGE TOP BOTTOM LEFT RIGHT OFF AGGREGATE TITLE
%token <token> PIE DONUT BAR SCATTER LINE VERTICAL HORIZONTAL
%token <stringValue> IDENTIFIER
%token <stringValue> STRING
%token <numberValue> NUMBER
%token <colorValue> COLOR
%token <token> EQ COMMA SEMI COLON LBRACK RBRACK GT LT GE LE EQEQ DOT

%left ADD SUB
%left MUL DIV
%right AS
%right STRING

%type <constant> constant
%type <expression> expression
%type <factor> factor
%type <program> program
%type <program> stmt stmt_list
%type <stringValue> source_decl chart_decl
%type <stringValue> string_list string_or_identifier from_source
%type <colorValue> color_list
%type <token> chart_body filter_clause project_clause chart_type orientation legend_position aggregate_function
%type <token> required_from required_x required_y optional_options optional_option opt_comma opt_semi range_values number_literal orientation_option colors_option color_option legend_option hole_option id_option range_option x_range_option y_range_option title_option

%%

program: stmt_list
       ;

stmt_list: /* empty */                     { $$ = NULL; }
         | stmt_list stmt                   { destroyProgram($1); $$ = $2; }
         ;

stmt: source_decl                      { $$ = SourceProgramSemanticAction($1, NULL); }
    | chart_decl                       { $$ = ChartProgramSemanticAction($1); }
    ;

source_decl: SOURCE IDENTIFIER EQ FROM STRING SEMI { free($5); $$ = $2; }
           | SOURCE IDENTIFIER EQ FROM IDENTIFIER SEMI { free($5); $$ = $2; }
           | SOURCE IDENTIFIER EQ FROM STRING filter_clause SEMI { free($5); $$ = $2; }
           | SOURCE IDENTIFIER EQ FROM IDENTIFIER filter_clause SEMI { free($5); $$ = $2; }
           | SOURCE IDENTIFIER EQ FROM STRING project_clause SEMI { free($5); $$ = $2; }
           | SOURCE IDENTIFIER EQ FROM IDENTIFIER project_clause SEMI { free($5); $$ = $2; }
           | SOURCE IDENTIFIER EQ FROM STRING filter_clause project_clause SEMI { free($5); $$ = $2; }
           | SOURCE IDENTIFIER EQ FROM IDENTIFIER filter_clause project_clause SEMI { free($5); $$ = $2; }
           | SOURCE IDENTIFIER EQ FROM STRING project_clause filter_clause SEMI { free($5); $$ = $2; }
           | SOURCE IDENTIFIER EQ FROM IDENTIFIER project_clause filter_clause SEMI { free($5); $$ = $2; }
           ;

filter_clause: FILTER STRING EQEQ STRING    { free($2); free($4); $$ = FILTER; }
             | FILTER STRING GT STRING       { free($2); free($4); $$ = FILTER; }
             | FILTER STRING LT STRING       { free($2); free($4); $$ = FILTER; }
             | FILTER STRING GE STRING       { free($2); free($4); $$ = FILTER; }
             | FILTER STRING LE STRING       { free($2); free($4); $$ = FILTER; }
             | FILTER STRING GT INTEGER      { free($2); $$ = FILTER; }
             | FILTER STRING LT INTEGER      { free($2); $$ = FILTER; }
             | FILTER STRING GE INTEGER      { free($2); $$ = FILTER; }
             | FILTER STRING LE INTEGER      { free($2); $$ = FILTER; }
             | FILTER STRING EQEQ INTEGER    { free($2); $$ = FILTER; }
             | FILTER IDENTIFIER EQEQ STRING { free($2); free($4); $$ = FILTER; }
             | FILTER IDENTIFIER GT STRING   { free($2); free($4); $$ = FILTER; }
             | FILTER IDENTIFIER LT STRING   { free($2); free($4); $$ = FILTER; }
             | FILTER IDENTIFIER GE STRING   { free($2); free($4); $$ = FILTER; }
             | FILTER IDENTIFIER LE STRING   { free($2); free($4); $$ = FILTER; }
             | FILTER IDENTIFIER GT INTEGER  { free($2); $$ = FILTER; }
             | FILTER IDENTIFIER LT INTEGER  { free($2); $$ = FILTER; }
             | FILTER IDENTIFIER GE INTEGER  { free($2); $$ = FILTER; }
             | FILTER IDENTIFIER LE INTEGER  { free($2); $$ = FILTER; }
             | FILTER IDENTIFIER EQEQ INTEGER { free($2); $$ = FILTER; }
             ;

project_clause: PROJECT LBRACK string_list RBRACK { (void)$3; $$ = PROJECT; }
              ;

string_list: STRING                          { free($1); $$ = NULL; }
           | IDENTIFIER                      { free($1); $$ = NULL; }
           | string_list COMMA STRING        { (void)$1; free($3); $$ = NULL; }
           | string_list COMMA IDENTIFIER    { (void)$1; free($3); $$ = NULL; }
           ;

chart_decl: CHART STRING TYPE chart_type COLON chart_body opt_semi { $$ = $2; }
          ;

opt_semi: SEMI { $$ = 0; }
        | /* empty */ { $$ = 0; }
        ;

chart_body: required_from required_x required_y optional_options { $$ = 0; }
          ;

required_from: FROM from_source opt_comma { (void)$2; $$ = 0; }
             ;

from_source: STRING { free($1); $$ = NULL; }
           | IDENTIFIER { free($1); $$ = NULL; }
           | LBRACK string_list RBRACK { (void)$2; $$ = NULL; }
           ;

required_x: X EQ string_or_identifier opt_comma { (void)$3; $$ = 0; }
          ;

string_or_identifier: STRING { free($1); $$ = NULL; }
                    | IDENTIFIER { free($1); $$ = NULL; }
                    ;

required_y: Y EQ expression opt_comma { destroyExpression($3); $$ = 0; }
          ;

optional_options: /* empty */ { $$ = 0; }
                | optional_options optional_option { $$ = 0; }
                ;

optional_option: orientation_option
               | colors_option
               | color_option
               | legend_option
               | hole_option
               | id_option
               | range_option
               | x_range_option
               | y_range_option
               | title_option
               ;

orientation_option: ORIENTATION EQ orientation opt_comma { $$ = 0; }
                  ;

colors_option: COLORS EQ LBRACK color_list RBRACK opt_comma { (void)$4; $$ = 0; }
             ;

color_option: COLOR_KW EQ COLOR opt_comma { free($3); $$ = 0; }
            ;

legend_option: LEGEND EQ legend_position opt_comma { $$ = 0; }
             ;

hole_option: HOLE EQ number_literal opt_comma { $$ = 0; }
           ;

id_option: ID_KW EQ string_or_identifier opt_comma { (void)$3; $$ = 0; }
         ;

range_option: RANGE EQ range_values opt_comma { $$ = 0; }
            ;

x_range_option: X DOT RANGE EQ range_values opt_comma { $$ = 0; }
              ;

y_range_option: Y DOT RANGE EQ range_values opt_comma { $$ = 0; }
              ;

title_option: TITLE EQ STRING opt_comma { free($3); $$ = 0; }
            ;

range_values: LBRACK number_literal COMMA number_literal RBRACK { $$ = 0; }
            ;

number_literal: NUMBER { $$ = 0; }
              | INTEGER { $$ = 0; }
              ;

opt_comma: COMMA { $$ = 0; }
         | /* empty */ { $$ = 0; }
         ;

chart_type: PIE                              { $$ = PIE; }
          | DONUT                            { $$ = DONUT; }
          | BAR                              { $$ = BAR; }
          | SCATTER                          { $$ = SCATTER; }
          | LINE                             { $$ = LINE; }
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

color_list: COLOR                            { free($1); $$ = NULL; }
          | color_list COMMA COLOR           { (void)$1; free($3); $$ = NULL; }
          ;

expression: expression AS STRING             { destroyExpression($1); free($3); $$ = NULL; /* expression alias */ }
          | expression ADD expression       { $$ = ArithmeticExpressionSemanticAction($1, $3, ADDITION); }
          | expression SUB expression       { $$ = ArithmeticExpressionSemanticAction($1, $3, SUBTRACTION); }
          | expression MUL expression       { $$ = ArithmeticExpressionSemanticAction($1, $3, MULTIPLICATION); }
          | expression DIV expression       { $$ = ArithmeticExpressionSemanticAction($1, $3, DIVISION); }
          | aggregate_function               { $$ = NULL; /* aggregate function */ }
          | factor                          { $$ = FactorExpressionSemanticAction($1); }
          ;

factor: OPEN_PARENTHESIS expression CLOSE_PARENTHESIS { $$ = ExpressionFactorSemanticAction($2); }
      | constant                                      { $$ = ConstantFactorSemanticAction($1); }
      ;

constant: INTEGER                { $$ = IntegerConstantSemanticAction($1); }
        | COLOR                  { $$ = ColorConstantSemanticAction($1); }
        | STRING                 { $$ = StringConstantSemanticAction($1); }
        | NUMBER                 { $$ = NumberConstantSemanticAction($1); }
        | IDENTIFIER             { $$ = StringConstantSemanticAction($1); }
        ;

aggregate_function: AVG OPEN_PARENTHESIS STRING CLOSE_PARENTHESIS    { free($3); $$ = AVG; }
                  | MIN OPEN_PARENTHESIS STRING CLOSE_PARENTHESIS    { free($3); $$ = MIN; }
                  | MAX OPEN_PARENTHESIS STRING CLOSE_PARENTHESIS    { free($3); $$ = MAX; }
                  | COUNT OPEN_PARENTHESIS STRING CLOSE_PARENTHESIS   { free($3); $$ = COUNT; }
                  | SUM OPEN_PARENTHESIS STRING CLOSE_PARENTHESIS    { free($3); $$ = SUM; }
                  | AVERAGE OPEN_PARENTHESIS STRING CLOSE_PARENTHESIS { free($3); $$ = AVERAGE; }
                  ;

%%
