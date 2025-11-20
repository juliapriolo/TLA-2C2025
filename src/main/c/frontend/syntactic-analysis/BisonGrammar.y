%{

#include "../../support/type/TokenLabel.h"
#include "AbstractSyntaxTree.h"
#include "BisonActions.h"
#include <string.h>

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
    Statement * statement;
    Source * source;
    Chart * chart;
    FilterCondition * filterCondition;
    Projection * projection;
    ChartType chartType;
    FilterOperator filterOperator;
    char ** stringArray;
    size_t sizeValue;
    double * rangeValue;  // Array de 2 doubles [min, max]
}

%destructor { destroyConstant($$); } <constant>
%destructor { destroyExpression($$); } <expression>
%destructor { destroyFactor($$); } <factor>
%destructor { destroyStatement($$); } <statement>
%destructor { destroySource($$); } <source>
%destructor { destroyChart($$); } <chart>
%destructor { destroyFilterCondition($$); } <filterCondition>
%destructor { destroyProjection($$); } <projection>
%destructor { free($$); } <stringValue>
%destructor { free($$); } <colorValue>
%destructor { free($$); } <rangeValue>
%destructor { 
    // Liberar los strings dentro del array primero, luego el array
    // Los strings son stringValue que tienen destructor, pero cuando están en un array
    // Bison no los destruye automáticamente, así que debemos liberarlos manualmente
    if ($$ != NULL) {
        char ** arr = (char **)$$;
        size_t i = 0;
        while (arr[i] != NULL) {
            free(arr[i]);
            i++;
        }
        free(arr);
    }
} <stringArray>

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
%type <factor> aggregate_function
%type <program> program
%type <statement> stmt
%type <statement> stmt_list
%type <statement> source_decl chart_decl
%type <source> from_source
%type <stringValue> string_or_identifier
%type <stringArray> string_list
%type <stringArray> color_list
%type <chart> chart_body
%type <filterCondition> filter_clause
%type <projection> project_clause
%type <chartType> chart_type
%type <token> orientation legend_position
%type <source> required_from
%type <stringValue> required_x
%type <expression> required_y
%type <token> optional_options optional_option opt_comma opt_semi orientation_option colors_option color_option legend_option hole_option id_option range_option x_range_option y_range_option
%type <numberValue> number_literal
%type <rangeValue> range_values

%%

program: stmt_list
         { $$ = ProgramFromStatementsSemanticAction($1); }
       ;

stmt_list: /* empty */                     { $$ = NULL; }
         | stmt_list stmt                   { 
                                               // Agregar statement a la lista
                                               if ($2 != NULL) {
                                                   $2->next = NULL;
                                                   if ($1 == NULL) {
                                                       $$ = $2;
                                                   } else {
                                                       Statement * last = $1;
                                                       while (last->next != NULL) last = last->next;
                                                       last->next = $2;
                                                       $$ = $1;
                                                   }
                                               } else {
                                                   $$ = $1;
                                               }
                                             }
         ;

stmt: source_decl                      { $$ = $1; }
    | chart_decl                       { $$ = $1; }
    ;

source_decl: SOURCE IDENTIFIER EQ FROM STRING SEMI { 
               Source * s = SourceSemanticAction($2, $5, NULL, NULL, NULL);
               $$ = createSourceStatement(s);
             }
           | SOURCE IDENTIFIER EQ FROM IDENTIFIER SEMI { 
               Source * s = SourceSemanticAction($2, NULL, $5, NULL, NULL);
               $$ = createSourceStatement(s);
             }
           | SOURCE IDENTIFIER EQ FROM STRING filter_clause SEMI { 
               Source * s = SourceSemanticAction($2, $5, NULL, $6, NULL);
               $$ = createSourceStatement(s);
             }
           | SOURCE IDENTIFIER EQ FROM IDENTIFIER filter_clause SEMI { 
               Source * s = SourceSemanticAction($2, NULL, $5, $6, NULL);
               $$ = createSourceStatement(s);
             }
           | SOURCE IDENTIFIER EQ FROM STRING project_clause SEMI { 
               Source * s = SourceSemanticAction($2, $5, NULL, NULL, $6);
               $$ = createSourceStatement(s);
             }
           | SOURCE IDENTIFIER EQ FROM IDENTIFIER project_clause SEMI { 
               Source * s = SourceSemanticAction($2, NULL, $5, NULL, $6);
               $$ = createSourceStatement(s);
             }
           | SOURCE IDENTIFIER EQ FROM STRING filter_clause project_clause SEMI { 
               Source * s = SourceSemanticAction($2, $5, NULL, $6, $7);
               if (s == NULL) {
                   $$ = NULL;
               } else {
                   $$ = createSourceStatement(s);
               }
             }
           | SOURCE IDENTIFIER EQ FROM IDENTIFIER filter_clause project_clause SEMI { 
               Source * s = SourceSemanticAction($2, NULL, $5, $6, $7);
               $$ = createSourceStatement(s);
             }
           | SOURCE IDENTIFIER EQ FROM STRING project_clause filter_clause SEMI { 
               Source * s = SourceSemanticAction($2, $5, NULL, $7, $6);
               $$ = createSourceStatement(s);
             }
           | SOURCE IDENTIFIER EQ FROM IDENTIFIER project_clause filter_clause SEMI { 
               Source * s = SourceSemanticAction($2, NULL, $5, $7, $6);
               $$ = createSourceStatement(s);
             }
           ;

filter_clause: FILTER STRING EQEQ STRING    { $$ = FilterConditionSemanticAction($2, EQEQ, $4); }
             | FILTER STRING GT STRING       { $$ = FilterConditionSemanticAction($2, GT, $4); }
             | FILTER STRING LT STRING       { $$ = FilterConditionSemanticAction($2, LT, $4); }
             | FILTER STRING GE STRING       { $$ = FilterConditionSemanticAction($2, GE, $4); }
             | FILTER STRING LE STRING       { $$ = FilterConditionSemanticAction($2, LE, $4); }
             | FILTER STRING GT INTEGER      { $$ = FilterConditionIntSemanticAction($2, GT, $4); }
             | FILTER STRING LT INTEGER      { $$ = FilterConditionIntSemanticAction($2, LT, $4); }
             | FILTER STRING GE INTEGER      { $$ = FilterConditionIntSemanticAction($2, GE, $4); }
             | FILTER STRING LE INTEGER      { $$ = FilterConditionIntSemanticAction($2, LE, $4); }
             | FILTER STRING EQEQ INTEGER    { $$ = FilterConditionIntSemanticAction($2, EQEQ, $4); }
             | FILTER IDENTIFIER EQEQ STRING { $$ = FilterConditionSemanticAction($2, EQEQ, $4); }
             | FILTER IDENTIFIER GT STRING   { $$ = FilterConditionSemanticAction($2, GT, $4); }
             | FILTER IDENTIFIER LT STRING   { $$ = FilterConditionSemanticAction($2, LT, $4); }
             | FILTER IDENTIFIER GE STRING   { $$ = FilterConditionSemanticAction($2, GE, $4); }
             | FILTER IDENTIFIER LE STRING   { $$ = FilterConditionSemanticAction($2, LE, $4); }
             | FILTER IDENTIFIER GT INTEGER  { $$ = FilterConditionIntSemanticAction($2, GT, $4); }
             | FILTER IDENTIFIER LT INTEGER  { $$ = FilterConditionIntSemanticAction($2, LT, $4); }
             | FILTER IDENTIFIER GE INTEGER  { $$ = FilterConditionIntSemanticAction($2, GE, $4); }
             | FILTER IDENTIFIER LE INTEGER  { $$ = FilterConditionIntSemanticAction($2, LE, $4); }
             | FILTER IDENTIFIER EQEQ INTEGER { $$ = FilterConditionIntSemanticAction($2, EQEQ, $4); }
             ;

project_clause: PROJECT LBRACK string_list RBRACK { 
                                                   size_t count = 0;
                                                   if ($3 != NULL) {
                                                       while ($3[count] != NULL) count++;
                                                   }
                                                   // Hacer copias de los strings antes de pasar a ProjectionSemanticAction
                                                   // para evitar problemas con destructores de Bison
                                                   char ** copiedColumns = NULL;
                                                   if ($3 != NULL && count > 0) {
                                                       copiedColumns = calloc(count + 1, sizeof(char*));
                                                       if (copiedColumns != NULL) {
                                                           for (size_t i = 0; i < count; ++i) {
                                                               if ($3[i] != NULL) {
                                                                   copiedColumns[i] = strdup($3[i]);
                                                                   if (copiedColumns[i] == NULL) {
                                                                       // Si falla strdup, liberar lo que ya se copió
                                                                       for (size_t j = 0; j < i; ++j) {
                                                                           free(copiedColumns[j]);
                                                                       }
                                                                       free(copiedColumns);
                                                                       copiedColumns = NULL;
                                                                       break;
                                                                   }
                                                               } else {
                                                                   copiedColumns[i] = NULL;
                                                               }
                                                           }
                                                           if (copiedColumns != NULL) {
                                                               copiedColumns[count] = NULL;
                                                           }
                                                       }
                                                   }
                                                   // Liberar manualmente el string_list original después de copiar los strings
                                                   // Los strings dentro del array necesitan ser liberados antes de liberar el array
                                                   if ($3 != NULL) {
                                                       size_t i = 0;
                                                       while ($3[i] != NULL) {
                                                           free($3[i]);
                                                           i++;
                                                       }
                                                       free($3);
                                                   }
                                                   $$ = ProjectionSemanticAction(copiedColumns, count);
                                                   if ($$ == NULL) {
                                                       // Liberar las copias si falló
                                                       if (copiedColumns != NULL) {
                                                           for (size_t i = 0; i < count; ++i) {
                                                               if (copiedColumns[i] != NULL) {
                                                                   free(copiedColumns[i]);
                                                               }
                                                           }
                                                           free(copiedColumns);
                                                       }
                                                   }
                                                 }
              ;

string_list: STRING                          { 
                                                 char ** arr = calloc(2, sizeof(char*));
                                                 arr[0] = $1;
                                                 arr[1] = NULL;
                                                 $$ = arr;
                                               }
           | IDENTIFIER                      { 
                                                 char ** arr = calloc(2, sizeof(char*));
                                                 arr[0] = $1;
                                                 arr[1] = NULL;
                                                 $$ = arr;
                                               }
           | string_list COMMA STRING        { 
                                                 size_t count = 0;
                                                 while ($1[count] != NULL) count++;
                                                 char ** arr = realloc($1, sizeof(char*) * (count + 2));
                                                 if (arr == NULL) {
                                                     // Si realloc falla, liberar el array original
                                                     free($1);
                                                     $$ = NULL;
                                                 } else {
                                                     arr[count] = $3;
                                                     arr[count + 1] = NULL;
                                                     $$ = arr;
                                                 }
                                               }
           | string_list COMMA IDENTIFIER    { 
                                                 size_t count = 0;
                                                 while ($1[count] != NULL) count++;
                                                 char ** arr = realloc($1, sizeof(char*) * (count + 2));
                                                 if (arr == NULL) {
                                                     // Si realloc falla, liberar el array original
                                                     free($1);
                                                     $$ = NULL;
                                                 } else {
                                                     arr[count] = $3;
                                                     arr[count + 1] = NULL;
                                                     $$ = arr;
                                                 }
                                               }
           ;

chart_decl: CHART STRING TYPE chart_type COLON chart_body opt_semi { 
             // Validar que el chart no sea NULL
             if ($6 == NULL) {
                 // El chart es NULL, probablemente debido a un error semántico
                 // No crear el statement, retornar NULL
                 $$ = NULL;
             } else {
                 // Actualizar el chart con el título y tipo
                 $6->title = $2;
                 $6->type = $4;
                 Statement * stmt = createChartStatement($6);
                 $$ = stmt;
             }
           }
          ;

opt_semi: SEMI { $$ = 0; }
        | /* empty */ { $$ = 0; }
        ;

chart_body: required_from required_x required_y { 
             // Crear el chart ANTES de procesar las opciones para que las opciones puedan actualizarlo
             Chart * c = ChartSemanticAction(NULL, CHART_BAR, $1, $2, $3, NULL);
             // Actualizar xColumn y yExpression si están disponibles
             if (c != NULL) {
                 c->xColumn = $2;
                 c->yExpression = $3;
                 // Establecer el chart actual para que las opciones puedan actualizarlo
                 SetCurrentChart(c);
             }
             $$ = c;
           }
         | chart_body optional_option {
             // Procesar opciones adicionales después de crear el chart
             $$ = $1;
           }
          ;

required_from: FROM from_source opt_comma { $$ = $2; }
             ;

from_source: STRING { 
               Source * s = SourceSemanticAction(NULL, $1, NULL, NULL, NULL);
               $$ = s;
             }
           | IDENTIFIER { 
               Source * s = SourceSemanticAction(NULL, NULL, $1, NULL, NULL);
               $$ = s;
             }
           | LBRACK string_list RBRACK { 
               // Múltiples sources: crear una lista enlazada
               Source * first = NULL;
               Source * last = NULL;
               if ($2 != NULL) {
                   size_t i = 0;
                   while ($2[i] != NULL) {
                       // Hacer copia del string para evitar problemas con destructores
                       char * sourceId = strdup($2[i]);
                       if (sourceId == NULL) {
                           // Si falla la copia, liberar lo que ya se creó
                           if (first != NULL) {
                               destroySource(first);
                           }
                           $$ = NULL;
                           break;
                       }
                       Source * s = SourceSemanticAction(NULL, NULL, sourceId, NULL, NULL);
                       // sourceId ahora es propiedad del Source, no liberarlo aquí
                       if (s == NULL) {
                           // Si algún source falla, liberar los que ya se crearon y retornar NULL
                           free(sourceId);  // Liberar la copia si falla
                           if (first != NULL) {
                               destroySource(first);
                           }
                           $$ = NULL;
                           break;
                       }
                       if (first == NULL) {
                           first = s;
                           last = s;
                       } else {
                           last->next = s;
                           last = s;
                       }
                       i++;
                   }
                   // Liberar manualmente el string_list original después de copiar los strings
                   if ($2 != NULL) {
                       size_t j = 0;
                       while ($2[j] != NULL) {
                           free($2[j]);
                           j++;
                       }
                       free($2);
                   }
               }
               $$ = first;
             }
           ;

required_x: X EQ string_or_identifier opt_comma { $$ = $3; }
          ;

string_or_identifier: STRING { $$ = $1; }
                    | IDENTIFIER { $$ = $1; }
                    ;

required_y: Y EQ expression opt_comma { $$ = $3; }
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
               ;

orientation_option: ORIENTATION EQ orientation opt_comma { 
                      SetChartOrientation($3);
                      $$ = 0;
                    }
                  ;

colors_option: COLORS EQ LBRACK color_list RBRACK opt_comma { 
               size_t count = 0;
               if ($4 != NULL) {
                   while ($4[count] != NULL) count++;
               }
               SetChartColors($4, count);
               // Liberar el array de colores y los strings (serán copiados por SetChartColors)
               if ($4 != NULL) {
                   size_t i = 0;
                   while ($4[i] != NULL) {
                       free($4[i]);
                       i++;
                   }
                   free($4);
               }
               $$ = 0;
             }
             ;

color_option: COLOR_KW EQ COLOR opt_comma { 
              SetChartSingleColor($3);
              // $3 será copiado por SetChartSingleColor, así que lo liberamos
              free($3);
              $$ = 0;
            }
            ;

legend_option: LEGEND EQ legend_position opt_comma { 
               SetChartLegendPosition($3);
               $$ = 0;
             }
             ;

hole_option: HOLE EQ number_literal opt_comma { 
             SetChartHole($3);
             $$ = 0;
           }
           ;

id_option: ID_KW EQ string_or_identifier opt_comma { SetChartId($3); $$ = 0; }
         ;

range_option: RANGE EQ range_values opt_comma { $$ = 0; }
            ;

x_range_option: X DOT RANGE EQ range_values opt_comma { 
                 SetChartXRange($5);
                 if ($5 != NULL) {
                     free($5);
                 }
                 $$ = 0;
               }
              ;

y_range_option: Y DOT RANGE EQ range_values opt_comma { 
                 SetChartYRange($5);
                 if ($5 != NULL) {
                     free($5);
                 }
                 $$ = 0;
               }
              ;


range_values: LBRACK number_literal COMMA number_literal RBRACK { 
               double * range = calloc(2, sizeof(double));
               if (range != NULL) {
                   range[0] = $2;  // min
                   range[1] = $4;  // max
               }
               $$ = range;
             }
            ;

number_literal: NUMBER { $$ = $1; }
              | INTEGER { $$ = (double)$1; }
              ;

opt_comma: COMMA { $$ = 0; }
         | /* empty */ { $$ = 0; }
         ;

chart_type: PIE                              { $$ = ChartTypeSemanticAction(PIE); }
          | DONUT                            { $$ = ChartTypeSemanticAction(DONUT); }
          | BAR                              { $$ = ChartTypeSemanticAction(BAR); }
          | SCATTER                          { $$ = ChartTypeSemanticAction(SCATTER); }
          | LINE                             { $$ = ChartTypeSemanticAction(LINE); }
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

color_list: COLOR                            { 
              char ** arr = calloc(2, sizeof(char*));
              arr[0] = $1;  // El color será copiado en SetChartColors
              arr[1] = NULL;
              $$ = arr;
            }
          | color_list COMMA COLOR           { 
              size_t count = 0;
              while ($1[count] != NULL) count++;
              char ** arr = realloc($1, sizeof(char*) * (count + 2));
              if (arr == NULL) {
                  free($1);
                  $$ = NULL;
              } else {
                  arr[count] = $3;  // El color será copiado en SetChartColors
                  arr[count + 1] = NULL;
                  $$ = arr;
              }
            }
          ;

expression: expression AS STRING             { 
              // Preservar la expresión y establecer el alias en el chart actual
              SetChartYAlias($3);  // $3 será copiado o liberado por SetChartYAlias
              $$ = $1;  // Retornar la expresión preservada
            }
          | expression ADD expression       { $$ = ArithmeticExpressionSemanticAction($1, $3, ADDITION); }
          | expression SUB expression       { $$ = ArithmeticExpressionSemanticAction($1, $3, SUBTRACTION); }
          | expression MUL expression       { $$ = ArithmeticExpressionSemanticAction($1, $3, MULTIPLICATION); }
          | expression DIV expression       { $$ = ArithmeticExpressionSemanticAction($1, $3, DIVISION); }
          | aggregate_function               { $$ = FactorExpressionSemanticAction($1); }
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

aggregate_function: AVG OPEN_PARENTHESIS STRING CLOSE_PARENTHESIS    { 
                     AggregateFunction func = AggregateFunctionSemanticAction(AVG);
                     $$ = AggregateFactorSemanticAction(func, $3);
                     // $3 será copiado por AggregateFactorSemanticAction, así que lo liberamos
                     free($3);
                   }
                  | MIN OPEN_PARENTHESIS STRING CLOSE_PARENTHESIS    { 
                     AggregateFunction func = AggregateFunctionSemanticAction(MIN);
                     $$ = AggregateFactorSemanticAction(func, $3);
                     free($3);
                   }
                  | MAX OPEN_PARENTHESIS STRING CLOSE_PARENTHESIS    { 
                     AggregateFunction func = AggregateFunctionSemanticAction(MAX);
                     $$ = AggregateFactorSemanticAction(func, $3);
                     free($3);
                   }
                  | COUNT OPEN_PARENTHESIS STRING CLOSE_PARENTHESIS   { 
                     AggregateFunction func = AggregateFunctionSemanticAction(COUNT);
                     $$ = AggregateFactorSemanticAction(func, $3);
                     free($3);
                   }
                  | SUM OPEN_PARENTHESIS STRING CLOSE_PARENTHESIS    { 
                     AggregateFunction func = AggregateFunctionSemanticAction(SUM);
                     $$ = AggregateFactorSemanticAction(func, $3);
                     free($3);
                   }
                  | AVERAGE OPEN_PARENTHESIS STRING CLOSE_PARENTHESIS { 
                     AggregateFunction func = AggregateFunctionSemanticAction(AVERAGE);
                     $$ = AggregateFactorSemanticAction(func, $3);
                     free($3);
                   }
                  ;

%%
