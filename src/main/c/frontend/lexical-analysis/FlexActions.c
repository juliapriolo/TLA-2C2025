#include "FlexActions.h"
#include "FlexScanner.h"
#include "../syntactic-analysis/BisonParser.h"
#include <errno.h>
#include <stdlib.h>
#include <string.h>

/* ESTADO INTERNO DEL MÓDULO */

static bool _logIgnoredLexemes = true;
static InputBuffer * _inputBuffer = NULL;
static LexicalAnalyzer * _lexicalAnalyzer = NULL;
static Logger * _logger = NULL;

/** Limpia y libera recursos del módulo. */
void _shutdownFlexActionsModule() {
	if (_logger != NULL) {
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

/* FUNCIONES PRIVADAS */

static void _logTokenAction(const char * actionName, Token * token);

/**
 * Registra una acción del analizador léxico sobre un token en nivel DEBUGGING.
 */
static void _logTokenAction(const char * actionName, Token * token) {
    (void)actionName;
    (void)token;
}


/* Almacena los bytes del valor semántico en el campo del token */
static void _setSemanticValue(Token * token, YYSTYPE value) {
	if (token == NULL) return;
	memcpy(&(token->semanticValue), &value, sizeof(YYSTYPE));
}


/* FUNCIONES PÚBLICAS */

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

CompilationStatus StringLexemeAction(const char * text, size_t length) {
	Token * token = createToken(_lexicalAnalyzer, STRING);
	if (token == NULL) return OUT_OF_MEMORY;

	if (token->lexeme != NULL) {
		free(token->lexeme);
		token->lexeme = NULL;
	}
	char * duplicated = NULL;
	if (text != NULL) {
		duplicated = (char *) calloc(length + 1, sizeof(char));
		if (duplicated == NULL) {
			if (_logger) logError(_logger, "Out of memory while duplicating string lexeme.");
			destroyToken(token);
			return OUT_OF_MEMORY;
		}
		if (length > 0) {
			memcpy(duplicated, text, length);
		}
		duplicated[length] = '\0';
		YYSTYPE semantic;
		memset(&semantic, 0, sizeof(YYSTYPE));
		semantic.stringValue = strdup(duplicated);
		if (semantic.stringValue == NULL) {
			if (_logger) logError(_logger, "Out of memory while setting semantic value for string.");
			free(duplicated);
			destroyToken(token);
			return OUT_OF_MEMORY;
		}
		_setSemanticValue(token, semantic);
	}
	token->lexeme = duplicated;

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


/* Cambian el modo del scanner al contexto solicitado */
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

    bool hadBuffer = false;
    if (_lexicalAnalyzer != NULL && popInputBuffer != NULL) {
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
