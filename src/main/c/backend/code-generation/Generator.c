#include "Generator.h"
#include "../chart-processing/ChartProcessor.h"
#include "../data-processing/CSVProcessor.h"
#include "../data-processing/DataProcessor.h"
#include <string.h>
#include <stdio.h>

/* MODULE INTERNAL STATE */

const char _indentationCharacter = ' ';
const char _indentationSize = 4;
static Logger * _logger = NULL;

/** Shutdown module's internal state. */
void _shutdownGeneratorModule() {
	if (_logger != NULL) {
		logDebugging(_logger, "Destroying module: Generator...");
		destroyLogger(_logger);
		_logger = NULL;
	}
}

ModuleDestructor initializeGeneratorModule() {
	_logger = createLogger("Generator");
	return _shutdownGeneratorModule;
}

/** PRIVATE FUNCTIONS */

static char * _indentation(const unsigned int indentationLevel);
static const char _expressionTypeToCharacter(const ExpressionType type);
static void _generateConstant(const unsigned int indentationLevel, Constant * constant);
static void _generateEpilogue(const int value);
static void _generateExpression(const unsigned int indentationLevel, Expression * expression);
static void _generateFactor(const unsigned int indentationLevel, Factor * factor);
static void _generateProgram(Program * program);
static void _generatePrologue(void);
static void _output(const unsigned int indentationLevel, const char * const format, ...);
static void _generateChartJS(Chart * chart, ChartData * chartData, const char * canvasId);
static const char * _chartTypeToString(ChartType type);
static void _generateHTMLPrologue(void);
static void _generateHTMLEpilogue(void);

/**
 * Converts and expression type to the proper character of the operation
 * involved, or returns '\0' if that's not possible.
 */
static const char _expressionTypeToCharacter(const ExpressionType type) {
	switch (type) {
		case ADDITION: return '+';
		case DIVISION: return '/';
		case MULTIPLICATION: return '*';
		case SUBTRACTION: return '-';
		default:
			logError(_logger, "The specified expression type cannot be converted into character: %d", type);
			return '\0';
	}
}

/**
 * Generates the output of a constant.
 */
static void _generateConstant(const unsigned int indentationLevel, Constant * constant) {
	_output(indentationLevel, "%s", "[ $C$, circle, draw, black!20\n");
	_output(1 + indentationLevel, "%s%d%s", "[ $", constant->value, "$, circle, draw ]\n");
	_output(indentationLevel, "%s", "]\n");
}

/**
 * Creates the epilogue of the generated output, that is, the final lines that
 * completes a valid Latex document.
 */
static void _generateEpilogue(const int value) {
	_output(0, "%s%d%s",
		"            [ $", value, "$, circle, draw, blue ]\n"
		"        ]\n"
		"    \\end{forest}\n"
		"\\end{document}\n\n"
	);
}

/**
 * Generates the output of an expression.
 */
static void _generateExpression(const unsigned int indentationLevel, Expression * expression) {
	_output(indentationLevel, "%s", "[ $E$, circle, draw, black!20\n");
	switch (expression->type) {
		case ADDITION:
		case DIVISION:
		case MULTIPLICATION:
		case SUBTRACTION:
			_generateExpression(1 + indentationLevel, expression->leftExpression);
			_output(1 + indentationLevel, "%s%c%s", "[ $", _expressionTypeToCharacter(expression->type), "$, circle, draw, purple ]\n");
			_generateExpression(1 + indentationLevel, expression->rightExpression);
			break;
		case FACTOR:
			_generateFactor(1 + indentationLevel, expression->factor);
			break;
		default:
			logError(_logger, "The specified expression type is unknown: %d", expression->type);
			break;
	}
	_output(indentationLevel, "%s", "]\n");
}

/**
 * Generates the output of a factor.
 */
static void _generateFactor(const unsigned int indentationLevel, Factor * factor) {
	_output(indentationLevel, "%s", "[ $F$, circle, draw, black!20\n");
	switch (factor->type) {
		case CONSTANT:
			_generateConstant(1 + indentationLevel, factor->constant);
			break;
		case EXPRESSION:
			_output(1 + indentationLevel, "%s", "[ $($, circle, draw, purple ]\n");
			_generateExpression(1 + indentationLevel, factor->expression);
			_output(1 + indentationLevel, "%s", "[ $)$, circle, draw, purple ]\n");
			break;
		default:
			logError(_logger, "The specified factor type is unknown: %d", factor->type);
			break;
	}
	_output(indentationLevel, "%s", "]\n");
}

/**
 * Generates the output of the program.
 */
static void _generateProgram(Program * program) {
	_generateExpression(3, program->expression);
}

/**
 * Creates the prologue of the generated output, a Latex document that renders
 * a tree thanks to the Forest package.
 *
 * @see https://ctan.dcc.uchile.cl/graphics/pgf/contrib/forest/forest-doc.pdf
 */
static void _generatePrologue(void) {
	_output(0, "%s",
		"\\documentclass{standalone}\n\n"
		"\\usepackage[utf8]{inputenc}\n"
		"\\usepackage[T1]{fontenc}\n"
		"\\usepackage{amsmath}\n"
		"\\usepackage{forest}\n"
		"\\usepackage{microtype}\n\n"
		"\\begin{document}\n"
		"    \\centering\n"
		"    \\begin{forest}\n"
		"        [ \\text{$=$}, circle, draw, purple\n"
	);
}

/**
 * Generates an indentation string for the specified level.
 */
static char * _indentation(const unsigned int level) {
	return indentation(_indentationCharacter, level, _indentationSize);
}

/**
 * Outputs a formatted string to standard output. The "fflush" instruction
 * allows to see the output even close to a failure, because it drops the
 * buffering.
 */
static void _output(const unsigned int indentationLevel, const char * const format, ...) {
	va_list arguments;
	va_start(arguments, format);
	char * indentation = _indentation(indentationLevel);
	char * effectiveFormat = concatenate(2, indentation, format);
	vfprintf(stdout, effectiveFormat, arguments);
	fflush(stdout);
	free(effectiveFormat);
	free(indentation);
	va_end(arguments);
}

/** NEW FUNCTIONS FOR CHART GENERATION */

/**
 * Genera el código JavaScript para un chart usando Chart.js
 */
static void _generateChartJS(Chart * chart, ChartData * chartData, const char * canvasId) {
	if (chart == NULL || chartData == NULL || canvasId == NULL) {
		return;
	}
	
	_output(2, "const ctx_%s = document.getElementById('%s').getContext('2d');\n", canvasId, canvasId);
	_output(2, "new Chart(ctx_%s, {\n", canvasId);
	_output(3, "type: '%s',\n", _chartTypeToString(chart->type));
	_output(3, "data: {\n");
	
	// Generar labels
	_output(4, "labels: [");
	for (size_t i = 0; i < chartData->dataCount; i++) {
		if (i > 0) _output(0, ", ");
		_output(0, "'%s'", chartData->labels[i] != NULL ? chartData->labels[i] : "");
	}
	_output(0, "],\n");
	
	// Generar datasets
	_output(4, "datasets: [{\n");
	_output(5, "label: '%s',\n", chartData->yLabel != NULL ? chartData->yLabel : "Data");
	_output(5, "data: [");
	for (size_t i = 0; i < chartData->dataCount; i++) {
		if (i > 0) _output(0, ", ");
		_output(0, "%.2f", chartData->values[i]);
	}
	_output(0, "],\n");
	
	// Colores
	if (chart->colors != NULL && chart->colorCount > 0) {
		_output(5, "backgroundColor: [");
		for (size_t i = 0; i < chart->colorCount && i < chartData->dataCount; i++) {
			if (i > 0) _output(0, ", ");
			_output(0, "'%s'", chart->colors[i] != NULL ? chart->colors[i] : "#3498db");
		}
		_output(0, "],\n");
	} else if (chart->singleColor != NULL) {
		_output(5, "backgroundColor: '%s',\n", chart->singleColor);
	}
	
	_output(4, "}]\n");
	_output(3, "},\n");
	_output(3, "options: {\n");
	_output(4, "responsive: true,\n");
	_output(4, "plugins: {\n");
	_output(5, "title: {\n");
	_output(6, "display: true,\n");
	_output(6, "text: '%s'\n", chart->title != NULL ? chart->title : "Chart");
	_output(5, "}\n");
	
	if (chart->legendPosition != NULL) {
		_output(5, ",\n");
		_output(5, "legend: {\n");
		_output(6, "position: '%s'\n", chart->legendPosition);
		_output(5, "}\n");
	}
	
	// Verificar si hay opciones adicionales después de plugins
	bool hasAdditionalOptions = (chart->type == CHART_BAR && chart->orientation != NULL) ||
	                            (chart->type == CHART_DONUT && chart->hole > 0.0) ||
	                            (chart->type == CHART_SCATTER && chart->xRange != NULL);
	
	if (hasAdditionalOptions) {
		_output(4, "},\n");
	} else {
		_output(4, "}\n");
	}
	
	// Opciones específicas por tipo
	if (chart->type == CHART_BAR && chart->orientation != NULL) {
		_output(4, "indexAxis: '%s',\n", strcmp(chart->orientation, "horizontal") == 0 ? "y" : "x");
	}
	
	if (chart->type == CHART_DONUT && chart->hole > 0.0) {
		_output(4, "cutout: '%.0f%%',\n", chart->hole * 100);
	}
	
	if (chart->type == CHART_SCATTER) {
		if (chart->xRange != NULL) {
			_output(4, "scales: {\n");
			_output(5, "x: {\n");
			_output(6, "min: %.2f,\n", chart->xRange[0]);
			_output(6, "max: %.2f\n", chart->xRange[1]);
			_output(5, "},\n");
			if (chart->yRange != NULL) {
				_output(5, "y: {\n");
				_output(6, "min: %.2f,\n", chart->yRange[0]);
				_output(6, "max: %.2f\n", chart->yRange[1]);
				_output(5, "}\n");
			}
			_output(4, "}\n");
		}
	}
	
	_output(3, "}\n");
	_output(2, "});\n");
}

/**
 * Convierte ChartType a string para Chart.js
 */
static const char * _chartTypeToString(ChartType type) {
	switch (type) {
		case CHART_BAR: return "bar";
		case CHART_PIE: return "pie";
		case CHART_DONUT: return "doughnut";
		case CHART_SCATTER: return "scatter";
		case CHART_LINE: return "line";
		default: return "bar";
	}
}

/**
 * Genera el HTML completo con Chart.js
 */
static void _generateHTMLPrologue(void) {
	_output(0, "<!DOCTYPE html>\n");
	_output(0, "<html lang=\"es\">\n");
	_output(1, "<head>\n");
	_output(2, "<meta charset=\"UTF-8\">\n");
	_output(2, "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">\n");
	_output(2, "<title>Generated Chart</title>\n");
	_output(2, "<script src=\"https://cdn.jsdelivr.net/npm/chart.js\"></script>\n");
	_output(2, "<style>\n");
	_output(3, "body { font-family: Arial, sans-serif; margin: 20px; }\n");
	_output(3, ".chart-container { margin: 20px 0; max-width: 800px; }\n");
	_output(2, "</style>\n");
	_output(1, "</head>\n");
	_output(1, "<body>\n");
}

static void _generateHTMLEpilogue(void) {
	_output(1, "</body>\n");
	_output(0, "</html>\n");
}

/** PUBLIC FUNCTIONS */

void executeGenerator(CompilerState * compilerState) {
	logDebugging(_logger, "Generating final output...");
	
	Program * program = compilerState->abstractSyntaxtTree;
	if (program == NULL) {
		logError(_logger, "Program is NULL");
		return;
	}
	
	// Si es un programa de gráficos (statements)
	if (program->statements != NULL) {
		_generateHTMLPrologue();
		
		// Primero, procesar todas las sources para tener los datos disponibles
		Statement * current = program->statements;
		CSVData ** csvDataMap = NULL;
		const char ** sourceIdentifiers = NULL;
		size_t sourceCount = 0;
		size_t sourceCapacity = 0;
		
		// Contar sources primero (todas las que tienen identifier)
		while (current != NULL) {
			if (current->type == STMT_SOURCE && current->source != NULL && current->source->identifier != NULL) {
				sourceCount++;
			}
			current = current->next;
		}
		
		if (sourceCount > 0) {
			csvDataMap = calloc(sourceCount, sizeof(CSVData*));
			sourceIdentifiers = calloc(sourceCount, sizeof(char*));
			if (csvDataMap == NULL || sourceIdentifiers == NULL) {
				logError(_logger, "Memory allocation failed");
				return;
			}
			
			// Inicializar todos los slots
			for (size_t i = 0; i < sourceCount; i++) {
				csvDataMap[i] = NULL;
				sourceIdentifiers[i] = NULL;
			}
			
			// Primera pasada: procesar sources que tienen csvFile (sources base)
			current = program->statements;
			while (current != NULL) {
				if (current->type == STMT_SOURCE && current->source != NULL) {
					Source * source = current->source;
					if (source->identifier != NULL && source->csvFile != NULL) {
						// Buscar el slot para este identifier
						size_t targetIndex = sourceCount; // Invalid index
						for (size_t i = 0; i < sourceCount; i++) {
							if (sourceIdentifiers[i] == NULL) {
								targetIndex = i;
								sourceIdentifiers[i] = source->identifier; // Reservar el slot
								break;
							}
						}
						
						if (targetIndex >= sourceCount) {
							logError(_logger, "No free slot found for source '%s'", source->identifier);
							current = current->next;
							continue;
						}
						
						// Buscar CSV en diferentes paths (misma lógica que ChartProcessor)
						char * csvPath = NULL;
						
						// Primero intentar el path directo
						FILE * testFile = fopen(source->csvFile, "r");
						if (testFile != NULL) {
							fclose(testFile);
							csvPath = (char*)source->csvFile;
						} else {
							// Intentar en src/test/c/data/
							char * dataPath = calloc(strlen(source->csvFile) + 50, sizeof(char));
							if (dataPath != NULL) {
								sprintf(dataPath, "src/test/c/data/%s", source->csvFile);
								testFile = fopen(dataPath, "r");
								if (testFile != NULL) {
									fclose(testFile);
									csvPath = dataPath;
								} else {
									free(dataPath);
									dataPath = NULL;
								}
							}
							
							// Si aún no funciona, intentar path absoluto desde el directorio actual
							if (csvPath == NULL) {
								dataPath = calloc(strlen(source->csvFile) + 20, sizeof(char));
								if (dataPath != NULL) {
									sprintf(dataPath, "./%s", source->csvFile);
									testFile = fopen(dataPath, "r");
									if (testFile != NULL) {
										fclose(testFile);
										csvPath = dataPath;
									} else {
										free(dataPath);
										dataPath = NULL;
									}
								}
							}
						}
						
						if (csvPath == NULL) {
							logError(_logger, "Cannot find CSV file for source '%s': %s (tried: %s, src/test/c/data/%s, ./%s)", 
								source->identifier, source->csvFile, source->csvFile, source->csvFile, source->csvFile);
							// Liberar el slot reservado
							sourceIdentifiers[targetIndex] = NULL;
							// Continuar con la siguiente source en lugar de fallar completamente
						} else if (csvPath != NULL) {
							CSVData * csvData = readCSVFile(csvPath);
							if (csvData != NULL) {
								// Procesar la source (aplicar filtros y proyecciones)
								ProcessedData * processed = processSourceData(csvData, source);
								if (processed != NULL) {
									// Convertir ProcessedData a CSVData para compatibilidad
									// Los datos procesados tienen la misma estructura que CSVData
									CSVData * processedCSV = calloc(1, sizeof(CSVData));
									if (processedCSV != NULL) {
										processedCSV->headers = processed->columnNames;
										processedCSV->headerCount = processed->columnCount;
										processedCSV->rows = processed->rows;
										processedCSV->rowCount = processed->rowCount;
										// No liberar processed, sus datos ahora pertenecen a processedCSV
										// Solo liberar la estructura ProcessedData, no los datos
										free(processed);
										
										csvDataMap[targetIndex] = processedCSV;
										logDebugging(_logger, "Stored source '%s' in map at index %zu", source->identifier, targetIndex);
									} else {
										destroyProcessedData(processed);
										// Liberar el slot reservado
										sourceIdentifiers[targetIndex] = NULL;
									}
								} else {
									// El procesamiento falló, liberar el slot reservado
									sourceIdentifiers[targetIndex] = NULL;
								}
								// Liberar CSV original después de procesar
								destroyCSVData(csvData);
							} else {
								// No se pudo leer el CSV, liberar el slot reservado
								sourceIdentifiers[targetIndex] = NULL;
							}
							
							if (csvPath != source->csvFile && csvPath != NULL) {
								free(csvPath);
							}
						}
					}
				}
				current = current->next;
			}
			
			// Segunda pasada: procesar sources que tienen sourceIdentifier (composición)
			// Estas sources dependen de otras sources ya procesadas
			current = program->statements;
			while (current != NULL) {
				if (current->type == STMT_SOURCE && current->source != NULL) {
					Source * source = current->source;
					// Procesar sources que tienen sourceIdentifier pero no csvFile (composición)
					if (source->identifier != NULL && source->sourceIdentifier != NULL && source->csvFile == NULL) {
						// Buscar la source base en el mapa
						CSVData * baseData = NULL;
						for (size_t i = 0; i < sourceCount; i++) {
							if (sourceIdentifiers[i] != NULL && strcmp(sourceIdentifiers[i], source->sourceIdentifier) == 0) {
								baseData = csvDataMap[i];
								if (baseData == NULL) {
									logError(_logger, "Found identifier '%s' at index %zu but csvDataMap[%zu] is NULL", 
										source->sourceIdentifier, i, i);
								}
								break;
							}
						}
						
						if (baseData == NULL) {
							logError(_logger, "Base source '%s' not found for composed source '%s' (searched in %zu sources)", 
								source->sourceIdentifier, source->identifier, sourceCount);
							// Debug: mostrar qué sources están en el mapa
							for (size_t i = 0; i < sourceCount; i++) {
								if (sourceIdentifiers[i] != NULL) {
									logDebugging(_logger, "  Map[%zu]: identifier='%s', data=%p", i, sourceIdentifiers[i], (void*)csvDataMap[i]);
								} else {
									logDebugging(_logger, "  Map[%zu]: NULL", i);
								}
							}
							// Continuar con la siguiente source
						} else {
							// Procesar la source compuesta (aplicar filtros y proyecciones sobre los datos base)
							ProcessedData * processed = processSourceData(baseData, source);
							if (processed != NULL) {
								// Convertir ProcessedData a CSVData
								CSVData * processedCSV = calloc(1, sizeof(CSVData));
								if (processedCSV != NULL) {
									processedCSV->headers = processed->columnNames;
									processedCSV->headerCount = processed->columnCount;
									processedCSV->rows = processed->rows;
									processedCSV->rowCount = processed->rowCount;
									free(processed);
									
									// Buscar el slot correspondiente a este identifier
									// Primero buscar si ya existe (no debería)
									bool stored = false;
									for (size_t i = 0; i < sourceCount; i++) {
										if (sourceIdentifiers[i] != NULL && strcmp(sourceIdentifiers[i], source->identifier) == 0) {
											// Ya existe, reemplazar (no debería pasar)
											logError(_logger, "Source identifier '%s' already exists in map, replacing", source->identifier);
											if (csvDataMap[i] != NULL) {
												destroyCSVData(csvDataMap[i]);
											}
											csvDataMap[i] = processedCSV;
											stored = true;
											break;
										}
									}
									
									// Si no existe, buscar un slot libre
									if (!stored) {
										for (size_t i = 0; i < sourceCount; i++) {
											if (sourceIdentifiers[i] == NULL) {
												// Slot libre, guardar aquí
												csvDataMap[i] = processedCSV;
												sourceIdentifiers[i] = source->identifier;
												logDebugging(_logger, "Stored composed source '%s' (from '%s') in map at index %zu", 
													source->identifier, source->sourceIdentifier, i);
												stored = true;
												break;
											}
										}
									}
									
									if (!stored) {
										logError(_logger, "No free slot found for composed source '%s' (map full)", source->identifier);
										destroyCSVData(processedCSV);
									}
								} else {
									destroyProcessedData(processed);
								}
							}
						}
					}
				}
				current = current->next;
			}
		}
		
		// Procesar charts
		current = program->statements;
		while (current != NULL) {
			if (current->type == STMT_CHART && current->chart != NULL) {
				Chart * chart = current->chart;
				ChartData * chartData = processChart(chart, csvDataMap, sourceIdentifiers, sourceCount);
				
				if (chartData != NULL) {
					const char * canvasId = chart->id != NULL ? chart->id : "chart";
					_output(1, "<div class=\"chart-container\">\n");
					_output(2, "<canvas id=\"%s\"></canvas>\n", canvasId);
					_output(1, "</div>\n");
					_output(1, "<script>\n");
					_generateChartJS(chart, chartData, canvasId);
					_output(1, "</script>\n");
					
					destroyChartData(chartData);
				}
			}
			current = current->next;
		}
		
		// Liberar datos CSV
		if (csvDataMap != NULL) {
			for (size_t i = 0; i < sourceCount; i++) {
				if (csvDataMap[i] != NULL) {
					destroyCSVData(csvDataMap[i]);
				}
			}
			free(csvDataMap);
			free(sourceIdentifiers);
		}
		
		_generateHTMLEpilogue();
	}
	// Si es un programa de calculadora (expression), mantener el comportamiento original
	else if (program->expression != NULL) {
		_generatePrologue();
		_generateProgram(program);
		_generateEpilogue(compilerState->value);
	}
	
	logDebugging(_logger, "Generation is done.");
}
