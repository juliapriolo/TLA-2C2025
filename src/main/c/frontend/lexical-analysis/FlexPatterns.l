%top{

#include "../../support/type/CompilationStatus.h"
#include "../../support/type/FlexContext.h"
#include "../../support/type/TokenLabel.h"
#include "../syntactic-analysis/AbstractSyntaxTree.h"
#include "../syntactic-analysis/BisonParser.h"

}

%{
/* === Declaraciones de funciones externas === */

extern CompilationStatus ArithmeticOperatorLexemeAction(TokenLabel label);
extern CompilationStatus EnterImportExpressionLexemeAction(FlexContext context);
extern CompilationStatus EnterMultilineCommentLexemeAction(FlexContext context);
extern CompilationStatus EOFLexemeAction();
extern CompilationStatus IgnoredLexemeAction();
extern CompilationStatus IntegerLexemeAction();
extern CompilationStatus LeaveImportExpressionLexemeAction();
extern CompilationStatus LeaveMultilineCommentLexemeAction();
extern CompilationStatus ParenthesisLexemeAction(TokenLabel label);
extern CompilationStatus SubexpressionLexemeAction();
extern CompilationStatus UnknownLexemeAction();

extern CompilationStatus KeywordLexemeAction(TokenLabel label);
extern CompilationStatus OperatorLexemeAction(TokenLabel label);
extern CompilationStatus PunctuationLexemeAction(TokenLabel label);
extern CompilationStatus StringLexemeAction(void);
extern CompilationStatus NumberLexemeAction(void);
extern CompilationStatus ColorLexemeAction(void);
extern CompilationStatus IdentifierLexemeAction(void);

%}

/* === Opciones generales === */
/* === Contextos === */

%x IMPORT_EXPRESSION
%x MULTILINE_COMMENT

/* === Patrones reutilizables === */

WHITESPACE_PATTERN  [[:space:]]+
IDENTIFIER_PATTERN  [a-zA-Z_][a-zA-Z0-9_]*
STRING_PATTERN      \"[^\"]*\"
NUMBER_PATTERN      [0-9]+\.[0-9]+
INTEGER_PATTERN     [0-9]+
COLOR_PATTERN       #[0-9a-fA-F]{6}

%%

%{

/* ===========================================================
   BLOQUES ESPECIALES (IMPORT / COMENTARIOS)
   =========================================================== */

%}

"{"                                 { return EnterImportExpressionLexemeAction(IMPORT_EXPRESSION); }
<IMPORT_EXPRESSION>[^{}]+           { return SubexpressionLexemeAction(); }
<IMPORT_EXPRESSION>"}"              { return LeaveImportExpressionLexemeAction(); }
<IMPORT_EXPRESSION>.                { return UnknownLexemeAction(); }

"/*"                                { return EnterMultilineCommentLexemeAction(MULTILINE_COMMENT); }
<MULTILINE_COMMENT>"*/"             { return LeaveMultilineCommentLexemeAction(); }
<MULTILINE_COMMENT>[^*]+            { return IgnoredLexemeAction(); }
<MULTILINE_COMMENT>.                { return IgnoredLexemeAction(); }


"source"        { return KeywordLexemeAction(SOURCE); }
"chart"         { return KeywordLexemeAction(CHART); }
"from"          { return KeywordLexemeAction(FROM); }
"select"        { return KeywordLexemeAction(SELECT); }
"where"         { return KeywordLexemeAction(WHERE); }
"filter"        { return KeywordLexemeAction(FILTER); }
"project"       { return KeywordLexemeAction(PROJECT); }
"aggregate"     { return KeywordLexemeAction(AGGREGATE); }
"as"            { return KeywordLexemeAction(AS); }
"type"          { return KeywordLexemeAction(TYPE); }
"x"             { return KeywordLexemeAction(X); }
"y"             { return KeywordLexemeAction(Y); }
"colors"        { return KeywordLexemeAction(COLORS); }
"color"         { return KeywordLexemeAction(COLOR_KW); }
"legend"        { return KeywordLexemeAction(LEGEND); }
"hole"          { return KeywordLexemeAction(HOLE); }
"id"            { return KeywordLexemeAction(ID_KW); }
"orientation"   { return KeywordLexemeAction(ORIENTATION); }
"range"         { return KeywordLexemeAction(RANGE); }
"title"         { return KeywordLexemeAction(TITLE); }

"avg"           { return KeywordLexemeAction(AVG); }
"average"       { return KeywordLexemeAction(AVERAGE); }
"min"           { return KeywordLexemeAction(MIN); }
"max"           { return KeywordLexemeAction(MAX); }
"count"         { return KeywordLexemeAction(COUNT); }
"sum"           { return KeywordLexemeAction(SUM); }

"pie"           { return KeywordLexemeAction(PIE); }
"donut"         { return KeywordLexemeAction(DONUT); }
"bar"           { return KeywordLexemeAction(BAR); }
"scatter"       { return KeywordLexemeAction(SCATTER); }
"line"          { return KeywordLexemeAction(LINE); }

"vertical"      { return KeywordLexemeAction(VERTICAL); }
"horizontal"    { return KeywordLexemeAction(HORIZONTAL); }
"top"           { return KeywordLexemeAction(TOP); }
"bottom"        { return KeywordLexemeAction(BOTTOM); }
"left"          { return KeywordLexemeAction(LEFT); }
"right"         { return KeywordLexemeAction(RIGHT); }
"off"           { return KeywordLexemeAction(OFF); }


">="            { return OperatorLexemeAction(GE); }
"<="            { return OperatorLexemeAction(LE); }
"=="            { return OperatorLexemeAction(EQEQ); }
"="             { return OperatorLexemeAction(EQ); }
">"             { return OperatorLexemeAction(GT); }
"<"             { return OperatorLexemeAction(LT); }
"+"             { return ArithmeticOperatorLexemeAction(ADD); }
"-"             { return ArithmeticOperatorLexemeAction(SUB); }
"*"             { return ArithmeticOperatorLexemeAction(MUL); }
"/"             { return ArithmeticOperatorLexemeAction(DIV); }

","             { return PunctuationLexemeAction(COMMA); }
";"             { return PunctuationLexemeAction(SEMI); }
":"             { return PunctuationLexemeAction(COLON); }
"."             { return PunctuationLexemeAction(DOT); }
"["             { return PunctuationLexemeAction(LBRACK); }
"]"             { return PunctuationLexemeAction(RBRACK); }
"("             { return ParenthesisLexemeAction(OPEN_PARENTHESIS); }
")"             { return ParenthesisLexemeAction(CLOSE_PARENTHESIS); }


{STRING_PATTERN}        { return StringLexemeAction(); }
{NUMBER_PATTERN}        { return NumberLexemeAction(); }
{INTEGER_PATTERN}       { return IntegerLexemeAction(); }
{COLOR_PATTERN}         { return ColorLexemeAction(); }
{IDENTIFIER_PATTERN}    { return IdentifierLexemeAction(); }
{WHITESPACE_PATTERN}    { return IgnoredLexemeAction(); }



<<EOF>>             { return EOFLexemeAction(); }
.                   { return UnknownLexemeAction(); }

%%

/**
 * @see https://westes.github.io/flex/manual/User-Code-Section.html#User-Code-Section
 */

#include "FlexActions.h"
#include "FlexExport.h"
