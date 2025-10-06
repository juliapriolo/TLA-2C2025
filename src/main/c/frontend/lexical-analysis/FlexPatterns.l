%top{

#include "../../support/type/CompilationStatus.h"
#include "../../support/type/FlexContext.h"
#include "../../support/type/TokenLabel.h"
#include "../syntactic-analysis/AbstractSyntaxTree.h"
#include "../syntactic-analysis/BisonParser.h"

}

%{

/**
 * @see https://westes.github.io/flex/manual/Definitions-Section.html#Definitions-Section
 */

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
extern CompilationStatus StringLexemeAction(TokenLabel label);
extern CompilationStatus NumberLexemeAction(TokenLabel label);
extern CompilationStatus ColorLexemeAction(TokenLabel label);
extern CompilationStatus IdentifierLexemeAction(TokenLabel label);

%}

/**
 * Flex contexts (a.k.a. start conditions).
 *
 * @see https://westes.github.io/flex/manual/Start-Conditions.html
 */
%x IMPORT_EXPRESSION
%x MULTILINE_COMMENT

/**
 * Reusable patterns.
 *
 * @see https://westes.github.io/flex/manual/Matching.html
 * @see https://westes.github.io/flex/manual/Patterns.html
 */
whitespace			[[:space:]]+
IDENTIFIER 			[a-zA-Z_][a-zA-Z0-9_]*
STRING 				\"[^\"]*\"
NUMBER 				[0-9]+\.[0-9]+
COLOR 				#[0-9a-fA-F]{6}

%%

%{

/**
 * @see https://westes.github.io/flex/manual/Rules-Section.html#Rules-Section
 */

%}

"{"									{ return EnterImportExpressionLexemeAction(IMPORT_EXPRESSION); }
<IMPORT_EXPRESSION>[^{}]+			{ return SubexpressionLexemeAction(); }
<IMPORT_EXPRESSION>"}"				{ return LeaveImportExpressionLexemeAction(); }
<IMPORT_EXPRESSION>.				{ return UnknownLexemeAction(); }

"/*"								{ return EnterMultilineCommentLexemeAction(MULTILINE_COMMENT); }
<MULTILINE_COMMENT>"*/"				{ return LeaveMultilineCommentLexemeAction(); }
<MULTILINE_COMMENT>[^*]+			{ return IgnoredLexemeAction(); }
<MULTILINE_COMMENT>.				{ return IgnoredLexemeAction(); }

"-"									{ return ArithmeticOperatorLexemeAction(SUB); }
"*"									{ return ArithmeticOperatorLexemeAction(MUL); }
"/"									{ return ArithmeticOperatorLexemeAction(DIV); }
"+"									{ return ArithmeticOperatorLexemeAction(ADD); }

"("									{ return ParenthesisLexemeAction(OPEN_PARENTHESIS); }
")"									{ return ParenthesisLexemeAction(CLOSE_PARENTHESIS); }

[[:digit:]]+						{ return IntegerLexemeAction(); }

{whitespace}						{ return IgnoredLexemeAction(); }
.									{ return UnknownLexemeAction(); }

<<EOF>>								{ return EOFLexemeAction(); }

%%

/*
"source"        { return KeywordLexemeAction(SOURCE); }
"chart"         { return KeywordLexemeAction(CHART); }
"from"          { return KeywordLexemeAction(FROM); }
"select"        { return KeywordLexemeAction(SELECT); }
"where"         { return KeywordLexemeAction(WHERE); }
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
"avg"           { return KeywordLexemeAction(AVG); }
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

{IDENTIFIER}    { return IdentifierLexemeAction(IDENTIFIER); }
{STRING}        { return StringLexemeAction(STRING); }
{NUMBER}        { return NumberLexemeAction(NUMBER); }
{COLOR}         { return ColorLexemeAction(COLOR); }

"="             { return OperatorLexemeAction(EQ); }
","             { return PunctuationLexemeAction(COMMA); }
";"             { return PunctuationLexemeAction(SEMI); }
":"             { return PunctuationLexemeAction(COLON); }
"["             { return PunctuationLexemeAction(LBRACK); }
"]"             { return PunctuationLexemeAction(RBRACK); }
">"             { return OperatorLexemeAction(GT); }
"<"             { return OperatorLexemeAction(LT); }
">="            { return OperatorLexemeAction(GE); }
"<="            { return OperatorLexemeAction(LE); }
"=="            { return OperatorLexemeAction(EQEQ); }
"."             { return PunctuationLexemeAction(DOT); }
*/

/**
 * @see https://westes.github.io/flex/manual/User-Code-Section.html#User-Code-Section
 */

#include "FlexActions.h"
#include "FlexExport.h"
