%top{

#include "../../support/type/CompilationStatus.h"
#include "../../support/type/FlexContext.h"
#include "../../support/type/TokenLabel.h"
#include "../syntactic-analysis/AbstractSyntaxTree.h"
#include "../syntactic-analysis/BisonParser.h"
#include <stddef.h>
#include <string.h>

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
extern CompilationStatus StringLexemeAction(const char * text, size_t length);
extern CompilationStatus NumberLexemeAction(void);
extern CompilationStatus ColorLexemeAction(void);
extern CompilationStatus IdentifierLexemeAction(void);

static TokenLabel _keywordLabel(const char * text) {
	if (text == NULL) return UNKNOWN;
	/* Palabras clave */
	if (strcmp(text, "source") == 0) return SOURCE;
	if (strcmp(text, "chart") == 0) return CHART;
	if (strcmp(text, "from") == 0) return FROM;
	if (strcmp(text, "select") == 0) return SELECT;
	if (strcmp(text, "where") == 0) return WHERE;
	if (strcmp(text, "filter") == 0) return FILTER;
	if (strcmp(text, "project") == 0) return PROJECT;
	if (strcmp(text, "aggregate") == 0) return AGGREGATE;
	if (strcmp(text, "as") == 0) return AS;
	if (strcmp(text, "type") == 0) return TYPE;
	if (strcmp(text, "x") == 0) return X;
	if (strcmp(text, "y") == 0) return Y;
	if (strcmp(text, "colors") == 0) return COLORS;
	if (strcmp(text, "color") == 0) return COLOR_KW;
	if (strcmp(text, "legend") == 0) return LEGEND;
	if (strcmp(text, "hole") == 0) return HOLE;
	if (strcmp(text, "id") == 0) return ID_KW;
	if (strcmp(text, "orientation") == 0) return ORIENTATION;
	if (strcmp(text, "range") == 0) return RANGE;
	if (strcmp(text, "title") == 0) return TITLE;
	/* Agregaciones */
	if (strcmp(text, "avg") == 0) return AVG;
	if (strcmp(text, "average") == 0) return AVERAGE;
	if (strcmp(text, "min") == 0) return MIN;
	if (strcmp(text, "max") == 0) return MAX;
	if (strcmp(text, "count") == 0) return COUNT;
	if (strcmp(text, "sum") == 0) return SUM;
	/* Tipos de chart */
	if (strcmp(text, "pie") == 0) return PIE;
	if (strcmp(text, "donut") == 0) return DONUT;
	if (strcmp(text, "bar") == 0) return BAR;
	if (strcmp(text, "scatter") == 0) return SCATTER;
	if (strcmp(text, "line") == 0) return LINE;
	/* Opciones de chart */
	if (strcmp(text, "vertical") == 0) return VERTICAL;
	if (strcmp(text, "horizontal") == 0) return HORIZONTAL;
	if (strcmp(text, "top") == 0) return TOP;
	if (strcmp(text, "bottom") == 0) return BOTTOM;
	if (strcmp(text, "left") == 0) return LEFT;
	if (strcmp(text, "right") == 0) return RIGHT;
	if (strcmp(text, "off") == 0) return OFF;
	return UNKNOWN;
}

static TokenLabel _arithmeticLabel(char c) {
	switch (c) {
		case '+': return ADD;
		case '-': return SUB;
		case '*': return MUL;
		case '/': return DIV;
		default: return UNKNOWN;
	}
}

static TokenLabel _operatorLabel(const char * text) {
	if (text == NULL) return UNKNOWN;
	if (strcmp(text, ">=") == 0) return GE;
	if (strcmp(text, "<=") == 0) return LE;
	if (strcmp(text, "==") == 0) return EQEQ;
	if (strcmp(text, "=") == 0) return EQ;
	if (strcmp(text, ">") == 0) return GT;
	if (strcmp(text, "<") == 0) return LT;
	return UNKNOWN;
}

static TokenLabel _punctuationLabel(char c) {
	switch (c) {
		case ',': return COMMA;
		case ';': return SEMI;
		case ':': return COLON;
		case '.': return DOT;
		case '[': return LBRACK;
		case ']': return RBRACK;
		default: return UNKNOWN;
	}
}

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
KEYWORD_PATTERN     (source|chart|from|select|where|filter|project|aggregate|as|type|x|y|colors|color|legend|hole|id|orientation|range|title|avg|average|min|max|count|sum|pie|donut|bar|scatter|line|vertical|horizontal|top|bottom|left|right|off)
ARITHMETIC_PATTERN  [+\-*/]
OPERATOR_PATTERN    (>=|<=|==|=|>|<)
PUNCTUATION_PATTERN ([,;:\.\[\]])

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


{KEYWORD_PATTERN}   { TokenLabel label = _keywordLabel(yytext); return label == UNKNOWN ? UnknownLexemeAction() : KeywordLexemeAction(label); }
{OPERATOR_PATTERN}      { TokenLabel label = _operatorLabel(yytext); return label == UNKNOWN ? UnknownLexemeAction() : OperatorLexemeAction(label); }
{ARITHMETIC_PATTERN}    { TokenLabel label = _arithmeticLabel(yytext[0]); return label == UNKNOWN ? UnknownLexemeAction() : ArithmeticOperatorLexemeAction(label); }
{PUNCTUATION_PATTERN}   { TokenLabel label = _punctuationLabel(yytext[0]); return label == UNKNOWN ? UnknownLexemeAction() : PunctuationLexemeAction(label); }
"("             { return ParenthesisLexemeAction(OPEN_PARENTHESIS); }
")"             { return ParenthesisLexemeAction(CLOSE_PARENTHESIS); }


{STRING_PATTERN}        { return StringLexemeAction(yytext + 1, yyleng >= 2 ? (size_t)(yyleng - 2) : 0); }
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
