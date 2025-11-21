#include "FlexActions.h"
#include "FlexScanner.h"
#include <errno.h>
#include <stdlib.h>
#include <string.h>

/* MODULE INTERNAL STATE */

static bool _logIgnoredLexemes = true;
static InputBuffer * _inputBuffer = NULL;
static LexicalAnalyzer * _lexicalAnalyzer = NULL;
static Logger * _logger = NULL;

/** limpia y libera recursos del módulo (logger, input buffer) */
void _shutdownFlexActionsModule() {
	if (_logger != NULL) {
		// logDebugging(_logger, "Destroying module: FlexActions...");
		destroyLogger(_logger);
		_logger = NULL;
	}
	if (_inputBuffer != NULL) {
		destroyInputBuffer(_inputBuffer);
		_inputBuffer = NULL;
	}
	_lexicalAnalyzer = NULL;
}


/* inicializa el modulo gy devuelve la funcion destructora (la de arriba)*/
ModuleDestructor initializeFlexActionsModule(LexicalAnalyzer * lexicalAnalyzer) {
	if (lexicalAnalyzer == NULL) {
        /* No tiene sentido inicializar sin el analizador léxico. */
        return NULL;
    }
    _inputBuffer = NULL;
    _lexicalAnalyzer = lexicalAnalyzer;
    _logger = createLogger("FlexActions");
    if (_logger == NULL) {
        /* Intentamos seguir, pero loger es NULL: lo anotamos en stderr para debug. */
        fprintf(stderr, "Warning: createLogger returned NULL in initializeFlexActionsModule\n");
    }
    _logIgnoredLexemes = getBooleanOrDefault("LOG_IGNORED_LEXEMES", _logIgnoredLexemes);
    return _shutdownFlexActionsModule;
}

/* PRIVATE FUNCTIONS */

static void _logTokenAction(const char * actionName, Token * token);

/**
 * Logs a lexical-analyzer action over a token in DEBUGGING level.
 */
 /* imprime en el log info del token */
static void _logTokenAction(const char * actionName, Token * token) {
    // Logging disabled
    (void)actionName;
    (void)token;
}


/* Helper: safe create token wrapper */
static Token * _safeCreateToken(TokenLabel label) {
    if (_lexicalAnalyzer == NULL) {
        if (_logger) logError(_logger, "Internal error: lexical analyzer is NULL in _safeCreateToken.");
        return NULL;
    }
    Token * t = createToken(_lexicalAnalyzer, label);
    if (t == NULL) {
        if (_logger) logError(_logger, "createToken returned NULL (label=%d).", label);
        return NULL;
    }
    if (_lexicalAnalyzer->scanner != NULL) {
        yyscan_t scanner = (yyscan_t)_lexicalAnalyzer->scanner;
        const char * yy_text = yyget_text(scanner);
        int yy_len = yyget_leng(scanner);
        int yy_line = yyget_lineno(scanner);

        if (yy_text != NULL) {
            size_t copy_len = (yy_len >= 0) ? (size_t)yy_len : strlen(yy_text);
            char * duplicated = (char *)malloc(copy_len + 1);
            if (duplicated != NULL) {
                memcpy(duplicated, yy_text, copy_len);
                duplicated[copy_len] = '\0';
                t->lexeme = duplicated;
                t->length = (unsigned int)copy_len;
            }
        }

        if (yy_line >= 0) {
            t->line = (unsigned int)yy_line;
        }

        t->context = currentLexicalAnalyzerContext(_lexicalAnalyzer);
    }
    return t;
}


/* Helper: extract string without quotes if present */
static char * _unquote_string(const char * s) {
    if (s == NULL) return NULL;
    size_t len = strlen(s);
    if (len >= 2 && s[0] == '"' && s[len-1] == '"') {
        char * out = (char*)malloc(len - 1);
        if (out == NULL) return NULL;
        memcpy(out, s + 1, len - 2);
        out[len-2] = '\0';
        return out;
    } else {
        return strdup(s);
    }
}


/* PUBLIC FUNCTIONS */
/* cada una crea un token con la etiqueta LABEL correspondiente
 (ADD, SOURCE, GE, COMMA, etc)
*/

CompilationStatus ArithmeticOperatorLexemeAction(TokenLabel label) {
    Token * token = _safeCreateToken(label);
    if (token == NULL) return FAILED;
    _logTokenAction(__FUNCTION__, token);
    CompilationStatus status = pushToken(_lexicalAnalyzer, token);
    destroyToken(token);
    return status;
}

CompilationStatus KeywordLexemeAction(TokenLabel label) {
    Token * token = _safeCreateToken(label);
    if (token == NULL) return FAILED;
    _logTokenAction(__FUNCTION__, token);
    CompilationStatus status = pushToken(_lexicalAnalyzer, token);
    destroyToken(token);
    return status;
}

CompilationStatus OperatorLexemeAction(TokenLabel label) {
    Token * token = _safeCreateToken(label);
    if (token == NULL) return FAILED;
    _logTokenAction(__FUNCTION__, token);
    CompilationStatus status = pushToken(_lexicalAnalyzer, token);
    destroyToken(token);
    return status;
}

CompilationStatus PunctuationLexemeAction(TokenLabel label) {
    Token * token = _safeCreateToken(label);
    if (token == NULL) return FAILED;
    _logTokenAction(__FUNCTION__, token);
    CompilationStatus status = pushToken(_lexicalAnalyzer, token);
    destroyToken(token);
    return status;
}

CompilationStatus StringLexemeAction(TokenLabel label) {
    Token * token = _safeCreateToken(label);
    if (token == NULL) return FAILED;

    /* Guardar valor semántico (sin comillas) */
    if (token->lexeme != NULL && token->semanticValue != NULL) {
        char * unq = _unquote_string(token->lexeme);
        if (unq != NULL) {
            /* >>> Asumo que destroyToken libera semanticValue->string; si no, ajustar */
            token->semanticValue->string = unq;
        } else {
            /* memoria insuficiente */
            if (_logger) logError(_logger, "Out of memory while duplicating string lexeme.");
            destroyToken(token);
            return FAILED;
        }
    }

    _logTokenAction(__FUNCTION__, token);
    CompilationStatus status = pushToken(_lexicalAnalyzer, token);
    destroyToken(token);
    return status;
}

CompilationStatus NumberLexemeAction(TokenLabel label) {
    Token * token = _safeCreateToken(label);
    if (token == NULL) return FAILED;

    if (token->lexeme != NULL && token->semanticValue != NULL) {
        errno = 0;
        char * endptr = NULL;
        double val = strtod(token->lexeme, &endptr);
        if (endptr == token->lexeme || errno == ERANGE) {
            if (_logger) logError(_logger, "Invalid number literal: %s", token->lexeme);
            destroyToken(token);
            return FAILED;
        }
        token->semanticValue->real = val;
    }

    _logTokenAction(__FUNCTION__, token);
    CompilationStatus status = pushToken(_lexicalAnalyzer, token);
    destroyToken(token);
    return status;
}

CompilationStatus ColorLexemeAction(TokenLabel label) {
    Token * token = _safeCreateToken(label);
    if (token == NULL) return FAILED;

    if (token->lexeme != NULL && token->semanticValue != NULL) {
        /* Guardar color como texto (ej: "#FFAABB") */
        token->semanticValue->string = strdup(token->lexeme);
        if (token->semanticValue->string == NULL) {
            if (_logger) logError(_logger, "Out of memory while duplicating color lexeme.");
            destroyToken(token);
            return FAILED;
        }
    }

    _logTokenAction(__FUNCTION__, token);
    CompilationStatus status = pushToken(_lexicalAnalyzer, token);
    destroyToken(token);
    return status;
}

CompilationStatus IdentifierLexemeAction(TokenLabel label) {
    Token * token = _safeCreateToken(label);
    if (token == NULL) return FAILED;

    if (token->lexeme != NULL && token->semanticValue != NULL) {
        /* identifers no entrecomillados: duplicar tal cual */
        token->semanticValue->string = strdup(token->lexeme);
        if (token->semanticValue->string == NULL) {
            if (_logger) logError(_logger, "Out of memory while duplicating identifier lexeme.");
            destroyToken(token);
            return FAILED;
        }
    }

    _logTokenAction(__FUNCTION__, token);
    CompilationStatus status = pushToken(_lexicalAnalyzer, token);
    destroyToken(token);
    return status;
}


/* cambian el modo del scanner al contexto solicitado (coment o import_expresion)*/
CompilationStatus EnterImportExpressionLexemeAction(FlexContext context) {
    if (_logIgnoredLexemes && _logger) {
        Token * token = _safeCreateToken(OPEN_BRACE);
        if (token) {
            _logTokenAction(__FUNCTION__, token);
            destroyToken(token);
        }
    }
    enterLexicalAnalyzerContext(_lexicalAnalyzer, context);
    return IN_PROGRESS;
}

CompilationStatus EnterMultilineCommentLexemeAction(FlexContext context) {
    if (_logIgnoredLexemes && _logger) {
        Token * token = _safeCreateToken(OPEN_COMMENT);
        if (token) {
            _logTokenAction(__FUNCTION__, token);
            destroyToken(token);
        }
    }
    enterLexicalAnalyzerContext(_lexicalAnalyzer, context);
    return IN_PROGRESS;
}

CompilationStatus EOFLexemeAction() {
    CompilationStatus status = IN_PROGRESS;
    /* >>> Preferible usar constante EOF_TOKEN si existe; aquí mantenemos 0 como fallback */
    Token * token = _safeCreateToken(0);
    if (token == NULL) return FAILED;
    _logTokenAction(__FUNCTION__, token);

    /* Intentar consumir input buffer si existe; popInputBuffer devuelve true si hubo buffer */
    bool hadBuffer = false;
    if (_lexicalAnalyzer != NULL && popInputBuffer != NULL) {
        /* Si popInputBuffer es función miembro, ajusta esta llamada según tu API */
        hadBuffer = popInputBuffer(_lexicalAnalyzer);
    }

    if (!hadBuffer) {
        status = pushToken(_lexicalAnalyzer, token);
        FlexContext context = currentLexicalAnalyzerContext(_lexicalAnalyzer);
        if (0 < context) {
            if (_logger) logError(_logger, "The final context is not closed (context=%d).", context);
            status = FAILED;
        }
    }

    destroyToken(token);
    return status;
}

CompilationStatus IgnoredLexemeAction() {
    if (_logIgnoredLexemes && _logger) {
        Token * token = _safeCreateToken(IGNORED);
        if (token) {
            _logTokenAction(__FUNCTION__, token);
            destroyToken(token);
        }
    }
    return IN_PROGRESS;
}

CompilationStatus IntegerLexemeAction() {
    Token * token = _safeCreateToken(INTEGER);
    if (token == NULL) return FAILED;

    if (token->lexeme != NULL && token->semanticValue != NULL) {
        errno = 0;
        char * endptr = NULL;
        long v = strtol(token->lexeme, &endptr, 10);
        if (endptr == token->lexeme || errno == ERANGE) {
            if (_logger) logError(_logger, "Invalid integer literal: %s", token->lexeme);
            destroyToken(token);
            return FAILED;
        }
        token->semanticValue->integer = (int)v;
    }

    _logTokenAction(__FUNCTION__, token);
    CompilationStatus status = pushToken(_lexicalAnalyzer, token);
    destroyToken(token);
    return status;
}

CompilationStatus LeaveImportExpressionLexemeAction() {
	pushInputBuffer(_inputBuffer);
	leaveLexicalAnalyzerContext(_lexicalAnalyzer);
	if (_logIgnoredLexemes) {
		Token * token = createToken(_lexicalAnalyzer, CLOSE_BRACE);
		_logTokenAction(__FUNCTION__, token);
		destroyToken(token);
	}
	return IN_PROGRESS;
}

CompilationStatus LeaveMultilineCommentLexemeAction() {
    leaveLexicalAnalyzerContext(_lexicalAnalyzer);
    if (_logIgnoredLexemes && _logger) {
        Token * token = _safeCreateToken(CLOSE_COMMENT);
        if (token) {
            _logTokenAction(__FUNCTION__, token);
            destroyToken(token);
        }
    }
    return IN_PROGRESS;
}

CompilationStatus ParenthesisLexemeAction(TokenLabel label) {
    Token * token = _safeCreateToken(label);
    if (token == NULL) return FAILED;
    _logTokenAction(__FUNCTION__, token);
    CompilationStatus status = pushToken(_lexicalAnalyzer, token);
    destroyToken(token);
    return status;
}

CompilationStatus SubexpressionLexemeAction() {
    Token * token = _safeCreateToken(IGNORED);
    if (token == NULL) return FAILED;

    InputBuffer * newBuf = createInputBuffer(_lexicalAnalyzer, token->lexeme);
    if (newBuf == NULL) {
        if (_logger) logError(_logger, "Failed to create input buffer for subexpression.");
        destroyToken(token);
        return FAILED;
    }

    /* reemplazo seguro del buffer global */
    if (_inputBuffer != NULL) {
        destroyInputBuffer(_inputBuffer);
        _inputBuffer = NULL;
    }
    _inputBuffer = newBuf;

    if (_logIgnoredLexemes && _logger) {
        _logTokenAction(__FUNCTION__, token);
    }

    destroyToken(token);
    return IN_PROGRESS;
}

CompilationStatus UnknownLexemeAction() {
    Token * token = _safeCreateToken(UNKNOWN);
    if (token) {
        _logTokenAction(__FUNCTION__, token);
        destroyToken(token);
    }
    return FAILED;
}
