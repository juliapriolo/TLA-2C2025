%type <filterOperator> comparison_op
%type <stringValue> filter_value
%{

#include "../../support/type/TokenLabel.h"
#include "AbstractSyntaxTree.h"
#include "BisonActions.h"
#include <string.h>

void yyerror(const YYLTYPE * location, const char * message) {
    fprintf(stderr, "Parse error at line %d: %s\n", location->first_line, message);
}

static void freeStringArray(char ** array) {
    if (array == NULL) {
        return;
    }
    for (size_t i = 0; array[i] != NULL; ++i) {
        free(array[i]);
    }
    free(array);
}

static char ** duplicateStringArray(char ** array, size_t count) {
    if (count == 0 || array == NULL) {
        return NULL;
    }
    char ** copy = calloc(count + 1, sizeof(char *));
    if (copy == NULL) {
        return NULL;
    }
    for (size_t i = 0; i < count; ++i) {
        if (array[i] != NULL) {
            copy[i] = strdup(array[i]);
            if (copy[i] == NULL) {
                freeStringArray(copy);
                return NULL;
            }
        }
    }
    copy[count] = NULL;
    return copy;
}

static Projection * buildProjectionFromList(char ** list) {
    size_t count = 0;
    if (list != NULL) {
        while (list[count] != NULL) count++;
    }
    char ** copiedColumns = duplicateStringArray(list, count);
    Projection * projection = ProjectionSemanticAction(copiedColumns, count);
    if (projection == NULL) {
        freeStringArray(copiedColumns);
    }
    return projection;
}

static SourceOptions * mergeSourceOptions(SourceOptions * base, SourceOptions * extra) {
    if (base == NULL) return extra;
    if (extra == NULL) return base;
    if (extra->filters != NULL) {
        base->filters = appendFilterCondition(base->filters, extra->filters);
    }
    if (extra->projection != NULL) {
        if (base->projection != NULL) {
            destroyProjection(base->projection);
        }
        base->projection = extra->projection;
    }
    free(extra);
    return base;
}

static Statement * buildSourceStatement(char * identifier, char * csvFile, char * sourceId, SourceOptions * options) {
    FilterCondition * filters = options ? options->filters : NULL;
    Projection * projection = options ? options->projection : NULL;
    Source * source = SourceSemanticAction(identifier, csvFile, sourceId, filters, projection);
    if (options != NULL) {
        free(options);
    }
    if (source == NULL) {
        destroyFilterCondition(filters);
        destroyProjection(projection);
        if (identifier != NULL) free(identifier);
        if (csvFile != NULL) free(csvFile);
        if (sourceId != NULL) free(sourceId);
        return NULL;
    }
    Statement * stmt = createSourceStatement(source);
    if (stmt == NULL) {
        destroySource(source);
    }
    return stmt;
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
    SourceOptions * sourceOptions;

}

%destructor { destroyConstant($$); } <constant>
%destructor { destroyExpression($$); } <expression>
%destructor { destroyFactor($$); } <factor>
%destructor { destroyStatement($$); } <statement>
%destructor { destroySource($$); } <source>
%destructor { destroyChart($$); } <chart>
%destructor { destroyFilterCondition($$); } <filterCondition>
%destructor { destroyProjection($$); } <projection>

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
%type <token> filter_kw
%type <projection> project_clause
%type <chartType> chart_type
%type <token> orientation legend_position
%type <source> required_from
%type <stringValue> required_x
%type <expression> required_y
%type <token> optional_option opt_comma opt_semi orientation_option colors_option color_option legend_option hole_option id_option range_option x_range_option y_range_option
%type <numberValue> number_literal
%type <rangeValue> range_values
%type <sourceOptions> source_options source_option

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

source_decl: SOURCE IDENTIFIER EQ FROM STRING source_options SEMI {
               /* Tomamos ownership directo de los lexemas del lexer */
               $$ = buildSourceStatement($2, $5, NULL, $6);
             }
           | SOURCE IDENTIFIER EQ FROM IDENTIFIER source_options SEMI {
               /* Tomamos ownership directo de los lexemas del lexer */
               $$ = buildSourceStatement($2, NULL, $5, $6);
             }
           ;

comparison_op: EQEQ { $$ = EQEQ; }
                         | GT  { $$ = GT; }
                         | LT  { $$ = LT; }
                         | GE  { $$ = GE; }
                         | LE  { $$ = LE; }

filter_value: STRING { $$ = $1; } | IDENTIFIER { $$ = $1; };

filter_kw: FILTER { $$ = FILTER; } | WHERE { $$ = WHERE; };

filter_clause:
        filter_kw filter_value comparison_op filter_value { $$ = FilterConditionSemanticAction($2, $3, $4); }
    |   filter_kw filter_value comparison_op INTEGER     { $$ = FilterConditionIntSemanticAction($2, $3, $4); }
;

source_options: /* empty */ { $$ = NULL; }
              | source_options source_option { $$ = mergeSourceOptions($1, $2); }
              ;

source_option: filter_clause    { $$ = createSourceOptions($1, NULL); }
             | project_clause   { $$ = createSourceOptions(NULL, $1); }
             ;

project_clause: PROJECT LBRACK string_list RBRACK { 
                                                   $$ = buildProjectionFromList($3);
                                                   freeStringArray($3);
                                                 }
              | SELECT LBRACK string_list RBRACK {
                                                   $$ = buildProjectionFromList($3);
                                                   freeStringArray($3);
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
                   freeStringArray($2);
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
               freeStringArray($4);
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

range_option: RANGE EQ range_values opt_comma { 
                if ($3 != NULL) {
                    free($3);
                }
                $$ = 0;
              }
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
