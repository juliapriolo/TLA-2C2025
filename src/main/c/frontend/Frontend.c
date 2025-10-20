#include "Frontend.h"

/* MODULE INTERNAL STATE */

static LexicalAnalyzer * _lexicalAnalyzer = NULL;
static Logger * _logger = NULL;

/** Shutdown module's internal state. */
void _shutdownFrontendModule() {
	if (_logger != NULL) {
		logDebugging(_logger, "Destroying module: Frontend...");
		destroyLogger(_logger);
		_logger = NULL;
	}
	_lexicalAnalyzer = NULL;
}

ModuleDestructor initializeFrontendModule(LexicalAnalyzer * lexicalAnalyzer) {
	_lexicalAnalyzer = lexicalAnalyzer;
	_logger = createLogger("Frontend");
	return _shutdownFrontendModule;
}

/* IMPORTED FUNCTIONS */

extern bool flexHasBuffer(LexicalAnalyzer * lexicalAnalyzer);
extern FlexContext flexCurrentContext(LexicalAnalyzer * lexicalAnalyzer);
extern void flexEnterContext(LexicalAnalyzer * lexicalAnalyzer, FlexContext flexContext);
extern void flexLeaveContext(LexicalAnalyzer * lexicalAnalyzer);

/* PRIVATE FUNCTIONS */

static const char * _compilationStatusAsString(const CompilationStatus compilationStatus) {
	switch (compilationStatus) {
		case FAILED:
			return "FAILED";
		case IN_PROGRESS:
			return "IN_PROGRESS";
		case OUT_OF_MEMORY:
			return "OUT_OF_MEMORY";
		case SUCCEEDED:
			return "SUCCEEDED";
		default:
			return "UNKNOWN_ERROR";
	}
}

/* PUBLIC FUNCTIONS */

InputBuffer * createInputBuffer(LexicalAnalyzer * lexicalAnalyzer, const char * path) {
	InputBuffer * inputBuffer = (InputBuffer *) calloc(1, sizeof(InputBuffer));
	inputBuffer->bufferSizeInBytes = YY_BUF_SIZE;
	inputBuffer->file = fopen(path, "r");
	inputBuffer->lexicalAnalyzer = lexicalAnalyzer;
	inputBuffer->buffer = yy_create_buffer(inputBuffer->file, inputBuffer->bufferSizeInBytes, lexicalAnalyzer->scanner);
	return inputBuffer;
}

LexicalAnalyzer * createLexicalAnalyzer() {
	LexicalAnalyzer * lexicalAnalyzer = (LexicalAnalyzer *) calloc(1, sizeof(LexicalAnalyzer));
	lexicalAnalyzer->location = calloc(1, sizeof(YYLTYPE));
	lexicalAnalyzer->logger = createLogger("LexicalAnalyzer");
	yylex_init(&lexicalAnalyzer->scanner);
	lexicalAnalyzer->parser = yypstate_new();
	flexEnterContext(lexicalAnalyzer, 0);
	return lexicalAnalyzer;
}

Token * createToken(LexicalAnalyzer * lexicalAnalyzer, TokenLabel label) {
    Token * t = (Token *) malloc(sizeof(Token));
    if (!t) return NULL;

    /* Inicializar campos mínimos (lexeme normalmente lo rellena flex internamente) */
    t->lexeme = NULL;                 /* será establecida por quien cree el token o por flex wrapper */
    t->context = 0;
    t->label = label;
    t->length = 0;
    t->line = 0;

    /* Crear y zero-fill la SemanticValue */
    t->semanticValue = (SemanticValue *) malloc(sizeof(SemanticValue));
    if (t->semanticValue == NULL) {
        free(t);
        return NULL;
    }
    /* Inicializar a cero para evitar basura */
    memset(t->semanticValue, 0, sizeof(SemanticValue));

    return t;
}

FlexContext currentLexicalAnalyzerContext(LexicalAnalyzer * lexicalAnalyzer) {
	return flexCurrentContext(lexicalAnalyzer);
}

void destroyInputBuffer(InputBuffer * inputBuffer) {
	if (inputBuffer != NULL) {
		if (inputBuffer->buffer != NULL) {
			/**
			 * @todo
			 *	Because "yypop_buffer_state" in "popInputBuffer" deletes the
			 *	buffer, this line produces a double-free error. However,
			 *	commenting the line produces a memory-leak when a syntax error
			 *	takes place inside a secondary input buffer.
			 */
			// yy_delete_buffer((YY_BUFFER_STATE) inputBuffer->buffer, (yyscan_t) inputBuffer->lexicalAnalyzer->scanner);
			inputBuffer->buffer = NULL;
		}
		if (inputBuffer->file != NULL) {
			fclose(inputBuffer->file);
			inputBuffer->file = NULL;
		}
		inputBuffer->bufferSizeInBytes = 0;
		inputBuffer->lexicalAnalyzer = NULL;
		free(inputBuffer);
	}
}

void destroyLexicalAnalyzer(LexicalAnalyzer * lexicalAnalyzer) {
	if (lexicalAnalyzer != NULL) {
		if (lexicalAnalyzer->parser != NULL) {
			yypstate_delete((yypstate *) lexicalAnalyzer->parser);
			lexicalAnalyzer->parser = NULL;
		}
		if (lexicalAnalyzer->scanner != NULL) {
			yylex_destroy((yyscan_t) lexicalAnalyzer->scanner);
			lexicalAnalyzer->scanner = NULL;
		}
		if (lexicalAnalyzer->logger != NULL) {
			destroyLogger(lexicalAnalyzer->logger);
			lexicalAnalyzer->logger = NULL;
		}
		if (lexicalAnalyzer->location != NULL) {
			free(lexicalAnalyzer->location);
			lexicalAnalyzer->location = NULL;
		}
		free(lexicalAnalyzer);
	}
}

void destroyToken(Token * t) {
    if (!t) return;

    /* Si semanticValue guarda strings, liberarlas */
    if (t->semanticValue != NULL) {
        /* Si semanticValue->string fue asignada por nosotros, la liberamos */
        if (t->semanticValue->string != NULL) {
            free(t->semanticValue->string);
            t->semanticValue->string = NULL;
        }
        free(t->semanticValue);
        t->semanticValue = NULL;
    }

    /* liberamos lexeme si createToken lo duplicó (patch según tu implementación) */
    if (t->lexeme != NULL) {
        free(t->lexeme);
        t->lexeme = NULL;
    }

    free(t);
}

void enterLexicalAnalyzerContext(LexicalAnalyzer * lexicalAnalyzer, FlexContext flexContext) {
	flexEnterContext(lexicalAnalyzer, flexContext);
}

CompilationStatus executeLexicalAnalysis(LexicalAnalyzer * lexicalAnalyzer) {
	return (CompilationStatus) yylex(
		NULL,
		(YYLTYPE *) lexicalAnalyzer->location,
		lexicalAnalyzer->scanner);
}

CompilationStatus executeSyntacticAnalysis() {
	logDebugging(_logger, "Parsing...");
	CompilationStatus status = IN_PROGRESS;
	while (status == IN_PROGRESS) {
		status = executeLexicalAnalysis(_lexicalAnalyzer);
	}
	logDebugging(_logger, "Compilation status: %s.", _compilationStatusAsString(status));
	logDebugging(_logger, "Parsing is done.");
	return status;
}

void leaveLexicalAnalyzerContext(LexicalAnalyzer * lexicalAnalyzer) {
	flexLeaveContext(lexicalAnalyzer);
}

bool popInputBuffer(LexicalAnalyzer * lexicalAnalyzer) {
	yypop_buffer_state((yyscan_t) lexicalAnalyzer->scanner);
	return flexHasBuffer(lexicalAnalyzer);
}

void pushInputBuffer(InputBuffer * inputBuffer) {
	yypush_buffer_state((YY_BUFFER_STATE) inputBuffer->buffer, (yyscan_t) inputBuffer->lexicalAnalyzer->scanner);
}

CompilationStatus pushToken(LexicalAnalyzer * lexicalAnalyzer, Token * token) {
	return (CompilationStatus) yypush_parse(
		(yypstate *) lexicalAnalyzer->parser,
		token->label,
		token->semanticValue,
		(YYLTYPE *) lexicalAnalyzer->location);
}
