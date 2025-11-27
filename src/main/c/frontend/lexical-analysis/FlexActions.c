#include "FlexActions.h"
#include "FlexScanner.h"
#include "../syntactic-analysis/BisonParser.h"
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


ModuleDestructor initializeFlexActionsModule(LexicalAnalyzer * lexicalAnalyzer) {
	if (lexicalAnalyzer == NULL) {
        return NULL;
    }
    _inputBuffer = NULL;
    _lexicalAnalyzer = lexicalAnalyzer;
    _logger = createLogger("FlexActions");
    if (_logger == NULL) {
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


/* Helper: store the semantic value bytes into the token field */
static void _setSemanticValue(Token * token, YYSTYPE value) {
	if (token == NULL) return;
	memcpy(&(token->semanticValue), &value, sizeof(YYSTYPE));
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
	Token * token = createToken(_lexicalAnalyzer, label);
	if (token == NULL) return OUT_OF_MEMORY;
    _logTokenAction(__FUNCTION__, token);
    CompilationStatus status = pushToken(_lexicalAnalyzer, token);
    destroyToken(token);
    return status;
}

CompilationStatus KeywordLexemeAction(TokenLabel label) {
	Token * token = createToken(_lexicalAnalyzer, label);
	if (token == NULL) return OUT_OF_MEMORY;
    _logTokenAction(__FUNCTION__, token);
    CompilationStatus status = pushToken(_lexicalAnalyzer, token);
    destroyToken(token);
    return status;
}

CompilationStatus OperatorLexemeAction(TokenLabel label) {
	Token * token = createToken(_lexicalAnalyzer, label);
	if (token == NULL) return OUT_OF_MEMORY;
    _logTokenAction(__FUNCTION__, token);
    CompilationStatus status = pushToken(_lexicalAnalyzer, token);
    destroyToken(token);
    return status;
}

CompilationStatus PunctuationLexemeAction(TokenLabel label) {
	Token * token = createToken(_lexicalAnalyzer, label);
	if (token == NULL) return OUT_OF_MEMORY;
    _logTokenAction(__FUNCTION__, token);
    CompilationStatus status = pushToken(_lexicalAnalyzer, token);
    destroyToken(token);
    return status;
}

CompilationStatus StringLexemeAction(void) {
	Token * token = createToken(_lexicalAnalyzer, STRING);
	if (token == NULL) return OUT_OF_MEMORY;

	/* Guardar valor semántico (sin comillas) */
	if (token->lexeme != NULL) {
		char * unq = _unquote_string(token->lexeme);
		if (unq == NULL) {
			if (_logger) logError(_logger, "Out of memory while duplicating string lexeme.");
			destroyToken(token);
			return OUT_OF_MEMORY;
		}
		YYSTYPE semantic;
		memset(&semantic, 0, sizeof(YYSTYPE));
		semantic.stringValue = unq;
		_setSemanticValue(token, semantic);
	}

	_logTokenAction(__FUNCTION__, token);
	CompilationStatus status = pushToken(_lexicalAnalyzer, token);
	destroyToken(token);
	return status;
}

CompilationStatus NumberLexemeAction(void) {
	Token * token = createToken(_lexicalAnalyzer, NUMBER);
	if (token == NULL) return OUT_OF_MEMORY;

    if (token->lexeme != NULL) {
        errno = 0;
        char * endptr = NULL;
        double val = strtod(token->lexeme, &endptr);
        if (endptr == token->lexeme || errno == ERANGE) {
			if (_logger) logError(_logger, "Invalid number literal: %s", token->lexeme);
			destroyToken(token);
			return FAILED;
		}
        YYSTYPE semantic;
        memset(&semantic, 0, sizeof(YYSTYPE));
        semantic.numberValue = val;
        _setSemanticValue(token, semantic);
    }

    _logTokenAction(__FUNCTION__, token);
    CompilationStatus status = pushToken(_lexicalAnalyzer, token);
    destroyToken(token);
    return status;
}

CompilationStatus ColorLexemeAction(void) {
	Token * token = createToken(_lexicalAnalyzer, COLOR);
	if (token == NULL) return OUT_OF_MEMORY;

    if (token->lexeme != NULL) {
        /* Guardar color como texto (ej: "#FFAABB") */
		char * duplicated = strdup(token->lexeme);
		if (duplicated == NULL) {
			if (_logger) logError(_logger, "Out of memory while duplicating color lexeme.");
			destroyToken(token);
			return OUT_OF_MEMORY;
		}
		YYSTYPE semantic;
		memset(&semantic, 0, sizeof(YYSTYPE));
        semantic.colorValue = duplicated;
        _setSemanticValue(token, semantic);
    }

    _logTokenAction(__FUNCTION__, token);
    CompilationStatus status = pushToken(_lexicalAnalyzer, token);
    destroyToken(token);
    return status;
}

CompilationStatus IdentifierLexemeAction(void) {
	Token * token = createToken(_lexicalAnalyzer, IDENTIFIER);
	if (token == NULL) return OUT_OF_MEMORY;

    if (token->lexeme != NULL) {
        /* identifers no entrecomillados: duplicar tal cual */
		char * duplicated = strdup(token->lexeme);
		if (duplicated == NULL) {
			if (_logger) logError(_logger, "Out of memory while duplicating identifier lexeme.");
			destroyToken(token);
			return OUT_OF_MEMORY;
		}
		YYSTYPE semantic;
		memset(&semantic, 0, sizeof(YYSTYPE));
        semantic.stringValue = duplicated;
        _setSemanticValue(token, semantic);
    }

    _logTokenAction(__FUNCTION__, token);
    CompilationStatus status = pushToken(_lexicalAnalyzer, token);
    destroyToken(token);
    return status;
}


/* cambian el modo del scanner al contexto solicitado (coment o import_expresion)*/
CompilationStatus EnterImportExpressionLexemeAction(FlexContext context) {
	if (_logIgnoredLexemes && _logger) {
		Token * token = createToken(_lexicalAnalyzer, OPEN_BRACE);
		if (token) {
			_logTokenAction(__FUNCTION__, token);
			destroyToken(token);
		} else {
			return OUT_OF_MEMORY;
		}
	}
	enterLexicalAnalyzerContext(_lexicalAnalyzer, context);
	return IN_PROGRESS;
}

CompilationStatus EnterMultilineCommentLexemeAction(FlexContext context) {
	if (_logIgnoredLexemes && _logger) {
		Token * token = createToken(_lexicalAnalyzer, OPEN_COMMENT);
		if (token) {
			_logTokenAction(__FUNCTION__, token);
			destroyToken(token);
		} else {
			return OUT_OF_MEMORY;
		}
	}
	enterLexicalAnalyzerContext(_lexicalAnalyzer, context);
	return IN_PROGRESS;
}

CompilationStatus EOFLexemeAction() {
	CompilationStatus status = IN_PROGRESS;
	Token * token = createToken(_lexicalAnalyzer, 0);
	if (token == NULL) return OUT_OF_MEMORY;
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
		Token * token = createToken(_lexicalAnalyzer, IGNORED);
		if (token) {
			_logTokenAction(__FUNCTION__, token);
			destroyToken(token);
		} else {
			return OUT_OF_MEMORY;
		}
	}
	return IN_PROGRESS;
}

CompilationStatus IntegerLexemeAction() {
	Token * token = createToken(_lexicalAnalyzer, INTEGER);
	if (token == NULL) return OUT_OF_MEMORY;

    if (token->lexeme != NULL) {
        errno = 0;
        char * endptr = NULL;
        long v = strtol(token->lexeme, &endptr, 10);
        if (endptr == token->lexeme || errno == ERANGE) {
			if (_logger) logError(_logger, "Invalid integer literal: %s", token->lexeme);
			destroyToken(token);
			return FAILED;
		}
        YYSTYPE semantic;
        memset(&semantic, 0, sizeof(YYSTYPE));
        semantic.integer = (int)v;
        _setSemanticValue(token, semantic);
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
		if (token != NULL) {
			_logTokenAction(__FUNCTION__, token);
			destroyToken(token);
		} else {
			return OUT_OF_MEMORY;
		}
	}
	return IN_PROGRESS;
}

CompilationStatus LeaveMultilineCommentLexemeAction() {
    leaveLexicalAnalyzerContext(_lexicalAnalyzer);
	if (_logIgnoredLexemes && _logger) {
		Token * token = createToken(_lexicalAnalyzer, CLOSE_COMMENT);
		if (token) {
			_logTokenAction(__FUNCTION__, token);
			destroyToken(token);
		} else {
			return OUT_OF_MEMORY;
		}
	}
	return IN_PROGRESS;
}

CompilationStatus ParenthesisLexemeAction(TokenLabel label) {
	Token * token = createToken(_lexicalAnalyzer, label);
	if (token == NULL) return OUT_OF_MEMORY;
	_logTokenAction(__FUNCTION__, token);
	CompilationStatus status = pushToken(_lexicalAnalyzer, token);
	destroyToken(token);
	return status;
}

CompilationStatus SubexpressionLexemeAction() {
	Token * token = createToken(_lexicalAnalyzer, IGNORED);
	if (token == NULL) return OUT_OF_MEMORY;

    InputBuffer * newBuf = createInputBuffer(_lexicalAnalyzer, token->lexeme);
	if (newBuf == NULL) {
		if (_logger) logError(_logger, "Failed to create input buffer for subexpression.");
		destroyToken(token);
		return OUT_OF_MEMORY;
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
	Token * token = createToken(_lexicalAnalyzer, UNKNOWN);
	if (token == NULL) {
		return OUT_OF_MEMORY;
	}
	_logTokenAction(__FUNCTION__, token);
	destroyToken(token);
	return FAILED;
}
