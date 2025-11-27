#include "backend/code-generation/Generator.h"
#include "backend/domain-specific/Calculator.h"
#include "backend/data-processing/CSVProcessor.h"
#include "backend/data-processing/DataProcessor.h"
#include "backend/chart-processing/ChartProcessor.h"
#include "frontend/Frontend.h"
#include "frontend/lexical-analysis/FlexActions.h"
#include "frontend/syntactic-analysis/BisonActions.h"
#include "frontend/syntactic-analysis/AbstractSyntaxTree.h"
#include "support/logging/Logger.h"
#include "support/type/CompilationStatus.h"
#include "support/type/CompilerState.h"
#include "support/type/ModuleDestructor.h"
#include <stdio.h>

/**
 * The main entry-point of the entire application. If you use "strtok" to
 * parse anything inside this project instead of using Flex and Bison, I will
 * find you, and I will kill you (Bryan Mills; "Taken", 2008).
 */
const int main(const int length, const char ** arguments) {
	LexicalAnalyzer * lexicalAnalyzer = createLexicalAnalyzer();
	Logger * logger = createLogger("EntryPoint");
	for (int k = 0; k < length; ++k) {
//		logDebugging(logger, "Argument %d: \"%s\"", k, arguments[k]);
	}
	CompilerState compilerState = {
		.abstractSyntaxtTree = NULL,
		.value = 0
	};
	ModuleDestructor moduleDestructors[] = {
		initializeAbstractSyntaxTreeModule(),
		initializeFlexActionsModule(lexicalAnalyzer),
		initializeBisonActionsModule(&compilerState),
		initializeFrontendModule(lexicalAnalyzer),
		initializeCalculatorModule(),
		initializeCSVProcessorModule(),
		initializeDataProcessorModule(),
		initializeChartProcessorModule(),
		initializeGeneratorModule()
	};
	CompilationStatus compilationStatus = executeSyntacticAnalysis();
	Program * program = compilerState.abstractSyntaxtTree;

	if (compilationStatus == SUCCEEDED && program != NULL) {
		/* Validaciones semánticas adicionales (duplicados, etc.) */
		const bool semanticValidationOk = ValidateProgramSemantics(program);
		const size_t semanticErrorCount = bisonSemanticErrorCount();
		const char * const * semanticErrors = bisonSemanticErrors();
		if (semanticErrorCount > 0 && semanticErrors != NULL) {
			logError(logger, "Semantic errors detected (%zu):", semanticErrorCount);
			for (size_t i = 0; i < semanticErrorCount; ++i) {
				logError(logger, "  %s", semanticErrors[i]);
			}
		}
		if (!semanticValidationOk || semanticErrorCount > 0) {
			compilationStatus = FAILED;
		}
	}

	if (compilationStatus == SUCCEEDED && program != NULL) {
		printAST(program);
	}
	
	if (compilationStatus == SUCCEEDED) {
		// ----------------------------------------------------------------------------------------
		// Beginning of the Backend... ------------------------------------------------------------
		if (program != NULL && program->statements != NULL) {
			// Programa DSL de gráficos: generar HTML/JavaScript
//			logDebugging(logger, "DSL program detected (charts/sources). Generating HTML/JavaScript...");
			executeGenerator(&compilerState);
		}
		else if (program != NULL && program->expression != NULL) {
			// Programa de calculadora: mantener comportamiento original
//			logDebugging(logger, "Computing expression value...");
			ComputationResult computationResult = executeCalculator(&compilerState);
			if (computationResult.succeeded) {
				compilerState.value = computationResult.value;
				executeGenerator(&compilerState);
			}
			else {
				logError(logger, "The computation phase rejects the input program.");
				compilationStatus = FAILED;
			}
		}
		else {
//			logDebugging(logger, "Empty program - nothing to execute.");
		}
		// ...end of the Backend. -----------------------------------------------------------------
		// ----------------------------------------------------------------------------------------
	}
	else {
		logError(logger, "The syntactic-analysis phase rejects the input program.");
		compilationStatus = FAILED;
	}
//	logDebugging(logger, "Releasing AST resources...");
	destroyProgram(program);
	for (int k = (sizeof(moduleDestructors)/sizeof(ModuleDestructor)) - 1; 0 <= k; --k) {
		moduleDestructors[k]();
	}
//	logDebugging(logger, "Compilation is done.");
	destroyLogger(logger);
	destroyLexicalAnalyzer(lexicalAnalyzer);
	return compilationStatus;
}
