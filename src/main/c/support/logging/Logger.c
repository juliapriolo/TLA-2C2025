#include "Logger.h"

/* FUNCIONES PRIVADAS */

static void _log(const Logger * logger, const LoggingLevel loggingLevel, const char * const format, va_list arguments);
static LoggingLevel _loggingLevelFromString(const char * loggingLevel);
static void _logInStream(FILE * const stream, const char * const format, va_list arguments);
static const char * _toContextString(const LoggingLevel loggingLevel);

/**
 * Registra un nuevo mensaje en el nivel especificado, usando una cadena de formato.
 */
static void _log(const Logger * logger, const LoggingLevel loggingLevel, const char * const format, va_list arguments) {
	if (logger->loggingLevel <= loggingLevel) {
		const char * context = _toContextString(loggingLevel);
		char * effectiveFormat = concatenate(6, context, "[", logger->name, "] ", format, "\n");
		if (ERROR <= loggingLevel) {
			_logInStream(stderr, effectiveFormat, arguments);
		}
		else {
			_logInStream(stdout, effectiveFormat, arguments);
		}
		free(effectiveFormat);
	}
}

/**
 * Obtiene el nivel de logging desde la cadena especificada.
 * Retorna CRITICAL si el valor proporcionado es desconocido.
 */
static LoggingLevel _loggingLevelFromString(const char * loggingLevel) {
	if (strcmp(loggingLevel, "ALL") == 0) return ALL;
	else if (strcmp(loggingLevel, "DEBUGGING") == 0) return DEBUGGING;
	else if (strcmp(loggingLevel, "INFORMATION") == 0) return INFORMATION;
	else if (strcmp(loggingLevel, "WARNING") == 0) return WARNING;
	else if (strcmp(loggingLevel, "ERROR") == 0) return ERROR;
	else return CRITICAL;
}

/**
 * Función de logging de bajo nivel.
 */
static void _logInStream(FILE * const stream, const char * const format, va_list arguments) {
	vfprintf(stream, format, arguments);
}

/**
 * Obtiene la cadena de contexto del nivel de logging especificado.
 */
static const char * _toContextString(const LoggingLevel loggingLevel) {
	switch (loggingLevel) {
		case ALL:
			return "[ALL  ]";
		case DEBUGGING:
			return "[" DEBUGGING_COLOR "DEBUG" DEFAULT_COLOR "]";
		case INFORMATION:
			return "[" INFORMATION_COLOR "INFO " DEFAULT_COLOR "]";
		case WARNING:
			return "[" WARNING_COLOR "WARN " DEFAULT_COLOR "]";
		case ERROR:
			return "[" ERROR_COLOR "ERROR" DEFAULT_COLOR "]";
		default:
			return "[" CRITICAL_COLOR "FATAL" DEFAULT_COLOR "]";
	}
}

/* FUNCIONES PÚBLICAS */

Logger * createLogger(char * name) {
	Logger * logger = calloc(1, sizeof(Logger));
	logger->loggingLevel = _loggingLevelFromString(getStringOrDefault("LOGGING_LEVEL", "INFORMATION"));
	logger->name = calloc(1 + strlen(name), sizeof(char));
	strcpy(logger->name, name);
	return logger;
}

void destroyLogger(Logger * logger) {
	if (logger != NULL) {
		if (logger->name != NULL) {
			free(logger->name);
		}
		free(logger);
	}
}

void logCritical(const Logger * logger, const char * const format, ...) {
	va_list arguments;
	va_start(arguments, format);
	_log(logger, CRITICAL, format, arguments);
	va_end(arguments);
}

void logDebugging(const Logger * logger, const char * const format, ...) {
	va_list arguments;
	va_start(arguments, format);
	_log(logger, DEBUGGING, format, arguments);
	va_end(arguments);
}

void logError(const Logger * logger, const char * const format, ...) {
	va_list arguments;
	va_start(arguments, format);
	_log(logger, ERROR, format, arguments);
	va_end(arguments);
}

void logInformation(const Logger * logger, const char * const format, ...) {
	va_list arguments;
	va_start(arguments, format);
	_log(logger, INFORMATION, format, arguments);
	va_end(arguments);
}

void logWarning(const Logger * logger, const char * const format, ...) {
	va_list arguments;
	va_start(arguments, format);
	_log(logger, WARNING, format, arguments);
	va_end(arguments);
}
