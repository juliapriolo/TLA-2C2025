#include "Generator.h"
#include "../chart-processing/ChartProcessor.h"
#include "../data-processing/CSVProcessor.h"
#include "../data-processing/DataProcessor.h"
#include <string.h>
#include <stdio.h>

/* ESTADO INTERNO DEL MÓDULO */

const char _indentationCharacter = ' ';
const char _indentationSize = 4;
static Logger * _logger = NULL;

/** Cierra el estado interno del módulo. */
void _shutdownGeneratorModule() {
	if (_logger != NULL) {
		destroyLogger(_logger);
		_logger = NULL;
	}
}

ModuleDestructor initializeGeneratorModule() {
	_logger = createLogger("Generator");
	return _shutdownGeneratorModule;
}

/** FUNCIONES PRIVADAS */

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
 * Convierte un tipo de expresión al carácter de la operación correspondiente,
 * o retorna '\0' si no es posible.
 */
static const char _expressionTypeToCharacter(const ExpressionType type) {
	switch (type) {
		case ADDITION: return '+';
		case DIVISION: return '/';
		case MULTIPLICATION: return '*';
		case SUBTRACTION: return '-';
		default:
			return '\0';
	}
}

/**
 * Genera la salida de una constante.
 */
static void _generateConstant(const unsigned int indentationLevel, Constant * constant) {
	_output(indentationLevel, "%s", "[ $C$, circle, draw, black!20\n");
	_output(1 + indentationLevel, "%s%d%s", "[ $", constant->value, "$, circle, draw ]\n");
	_output(indentationLevel, "%s", "]\n");
}

/**
 * Crea el epílogo de la salida generada, es decir, las líneas finales que
 * completan un documento LaTeX válido.
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
 * Genera la salida de una expresión.
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
 * Genera la salida de un factor.
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
 * Genera la salida del programa.
 */
static void _generateProgram(Program * program) {
	_generateExpression(3, program->expression);
}

/**
 * Crea el prólogo de la salida generada, un documento LaTeX que renderiza
 * un árbol gracias al paquete Forest.
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
 * Genera una cadena de indentación para el nivel especificado.
 */
static char * _indentation(const unsigned int level) {
	return indentation(_indentationCharacter, level, _indentationSize);
}

/**
 * Escribe una cadena formateada a la salida estándar.
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

/** NUEVAS FUNCIONES PARA GENERACIÓN DE GRÁFICOS */

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
	
	_output(4, "labels: [");
	for (size_t i = 0; i < chartData->dataCount; i++) {
		if (i > 0) _output(0, ", ");
		_output(0, "'%s'", chartData->labels[i] != NULL ? chartData->labels[i] : "");
	}
	_output(0, "],\n");
	
	_output(4, "datasets: [{\n");
	_output(5, "label: '%s',\n", chartData->yLabel != NULL ? chartData->yLabel : "Data");
	_output(5, "data: [");
	for (size_t i = 0; i < chartData->dataCount; i++) {
		if (i > 0) _output(0, ", ");
		_output(0, "%.6f", chartData->values[i]);
	}
	_output(0, "],\n");
	
	if (chart->colors != NULL && chart->colorCount > 0) {
		_output(5, "backgroundColor: [");
		for (size_t i = 0; i < chart->colorCount && i < chartData->dataCount; i++) {
			if (i > 0) _output(0, ", ");
			_output(0, "'%s'", chart->colors[i] != NULL ? chart->colors[i] : "#3498db");
		}
		if (chartData->dataCount > chart->colorCount) {
			const char * lastColor = chart->colors[chart->colorCount - 1] != NULL ? chart->colors[chart->colorCount - 1] : "#3498db";
			for (size_t i = chart->colorCount; i < chartData->dataCount; i++) {
				_output(0, ", '%s'", lastColor);
			}
		}
		_output(0, "],\n");
	} else if (chart->singleColor != NULL) {
		_output(5, "backgroundColor: '%s',\n", chart->singleColor);
	} else {
		if (chart->type == CHART_PIE || chart->type == CHART_DONUT) {
			const char * defaultColors[] = {
				"#FF6384", "#36A2EB", "#FFCE56", "#4BC0C0", "#9966FF",
				"#FF9F40", "#FF6384", "#C9CBCF", "#4BC0C0", "#FF6384"
			};
			size_t defaultColorCount = sizeof(defaultColors) / sizeof(defaultColors[0]);
			_output(5, "backgroundColor: [");
			for (size_t i = 0; i < chartData->dataCount; i++) {
				if (i > 0) _output(0, ", ");
				_output(0, "'%s'", defaultColors[i % defaultColorCount]);
			}
			_output(0, "],\n");
		}
	}
	
	_output(4, "}]\n");
	_output(3, "},\n");
	
	if (chart->type == CHART_PIE || chart->type == CHART_DONUT) {
		_output(3, "plugins: [ChartDataLabels],\n");
	}
	
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
	
	if (chart->type == CHART_PIE || chart->type == CHART_DONUT) {
		_output(5, ",\n");
		_output(5, "datalabels: {\n");
		_output(6, "color: '#fff',\n");
		_output(6, "font: {\n");
		_output(7, "weight: 'bold',\n");
		_output(7, "size: 14\n");
		_output(6, "},\n");
		_output(6, "formatter: (value, ctx) => {\n");
		_output(7, "const dataArr = ctx.chart.data.datasets[0].data;\n");
		_output(7, "const total = dataArr.reduce((a, b) => a + b, 0);\n");
		_output(7, "const percentage = ((value / total) * 100).toFixed(1);\n");
		_output(7, "return percentage + \"%\";\n");
		_output(6, "}\n");
		_output(5, "}\n");
	}
	
	bool hasAdditionalOptions = (chart->type == CHART_BAR && chart->orientation != NULL) ||
	                            (chart->type == CHART_DONUT && chart->hole > 0.0) ||
	                            (chart->type == CHART_SCATTER && chart->xRange != NULL);
	
	if (hasAdditionalOptions) {
		_output(4, "},\n");
	} else {
		_output(4, "}\n");
	}
	
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
 * Convierte ChartType a cadena para Chart.js.
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
 * Genera el HTML completo con Chart.js.
 */
static void _generateHTMLPrologue(void) {
	_output(0, "<!DOCTYPE html>\n");
	_output(0, "<html lang=\"es\">\n");
	_output(1, "<head>\n");
	_output(2, "<meta charset=\"UTF-8\">\n");
	_output(2, "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">\n");
	_output(2, "<title>Generated Chart</title>\n");
	_output(2, "<script src=\"https://cdn.jsdelivr.net/npm/chart.js\"></script>\n");
	_output(2, "<script src=\"https://cdn.jsdelivr.net/npm/chartjs-plugin-datalabels@2\"></script>\n");
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

/** FUNCIONES PÚBLICAS */

void executeGenerator(CompilerState * compilerState) {
	Program * program = compilerState->abstractSyntaxtTree;
	if (program == NULL) {
		logError(_logger, "Program is NULL");
		return;
	}
	
	if (program->statements != NULL) {
		_generateHTMLPrologue();
		
		Statement * current = program->statements;
		CSVData ** csvDataMap = NULL;
		const char ** sourceIdentifiers = NULL;
		size_t sourceCount = 0;
		size_t sourceCapacity = 0;
		
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
			
			for (size_t i = 0; i < sourceCount; i++) {
				csvDataMap[i] = NULL;
				sourceIdentifiers[i] = NULL;
			}
			
			current = program->statements;
			while (current != NULL) {
				if (current->type == STMT_SOURCE && current->source != NULL) {
					Source * source = current->source;
					if (source->identifier != NULL && source->csvFile != NULL) {
						size_t targetIndex = sourceCount;
						for (size_t i = 0; i < sourceCount; i++) {
							if (sourceIdentifiers[i] == NULL) {
								targetIndex = i;
								sourceIdentifiers[i] = source->identifier;
								break;
							}
						}
						
						if (targetIndex >= sourceCount) {
							logError(_logger, "No free slot found for source '%s'", source->identifier);
							current = current->next;
							continue;
						}
						
						char * csvPath = NULL;
						
						FILE * testFile = fopen(source->csvFile, "r");
						if (testFile != NULL) {
							fclose(testFile);
							csvPath = (char*)source->csvFile;
						} else {
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
							sourceIdentifiers[targetIndex] = NULL;
						} else if (csvPath != NULL) {
							CSVData * csvData = readCSVFile(csvPath);
							if (csvData != NULL) {
								ProcessedData * processed = processSourceData(csvData, source);
								if (processed != NULL) {
									CSVData * processedCSV = calloc(1, sizeof(CSVData));
									if (processedCSV != NULL) {
										processedCSV->headers = processed->columnNames;
										processedCSV->headerCount = processed->columnCount;
										processedCSV->rows = processed->rows;
										processedCSV->rowCount = processed->rowCount;
										free(processed);
										
										csvDataMap[targetIndex] = processedCSV;
									} else {
										destroyProcessedData(processed);
										sourceIdentifiers[targetIndex] = NULL;
									}
								} else {
									sourceIdentifiers[targetIndex] = NULL;
								}
								destroyCSVData(csvData);
							} else {
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
			
			current = program->statements;
			while (current != NULL) {
				if (current->type == STMT_SOURCE && current->source != NULL) {
					Source * source = current->source;
					if (source->identifier != NULL && source->sourceIdentifier != NULL && source->csvFile == NULL) {
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
						} else {
							ProcessedData * processed = processSourceData(baseData, source);
							if (processed != NULL) {
								CSVData * processedCSV = calloc(1, sizeof(CSVData));
								if (processedCSV != NULL) {
									processedCSV->headers = processed->columnNames;
									processedCSV->headerCount = processed->columnCount;
									processedCSV->rows = processed->rows;
									processedCSV->rowCount = processed->rowCount;
									free(processed);
									
									bool stored = false;
									for (size_t i = 0; i < sourceCount; i++) {
										if (sourceIdentifiers[i] != NULL && strcmp(sourceIdentifiers[i], source->identifier) == 0) {
											logError(_logger, "Source identifier '%s' already exists in map, replacing", source->identifier);
											if (csvDataMap[i] != NULL) {
												destroyCSVData(csvDataMap[i]);
											}
											csvDataMap[i] = processedCSV;
											stored = true;
											break;
										}
									}
									
									if (!stored) {
										for (size_t i = 0; i < sourceCount; i++) {
											if (sourceIdentifiers[i] == NULL) {
												csvDataMap[i] = processedCSV;
												sourceIdentifiers[i] = source->identifier;
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
	else if (program->expression != NULL) {
		_generatePrologue();
		_generateProgram(program);
		_generateEpilogue(compilerState->value);
	}
}
