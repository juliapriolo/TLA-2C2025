#include "ChartProcessor.h"
#include <string.h>
#include <math.h>
#include <stdio.h>

/* ESTADO INTERNO DEL MÓDULO */

static Logger * _logger = NULL;

/** Cierra el estado interno del módulo. */
void _shutdownChartProcessorModule() {
	if (_logger != NULL) {
		destroyLogger(_logger);
		_logger = NULL;
	}
}

ModuleDestructor initializeChartProcessorModule() {
	_logger = createLogger("ChartProcessor");
	return _shutdownChartProcessorModule;
}

/** FUNCIONES PRIVADAS */

/**
 * Busca datos CSV por identificador de source
 */
static CSVData * _findCSVData(const char * identifier, CSVData ** csvDataMap, const char ** sourceIdentifiers, size_t sourceCount) {
	if (identifier == NULL || csvDataMap == NULL || sourceIdentifiers == NULL) {
		return NULL;
	}
	
	for (size_t i = 0; i < sourceCount; i++) {
		if (sourceIdentifiers[i] != NULL) {
			if (strcmp(sourceIdentifiers[i], identifier) == 0) {
				return csvDataMap[i];
			}
		}
	}
	return NULL;
}

/**
 * Libera un array de sources, solo las que fueron leídas desde archivo
 */
static void _freeSourceDataArray(CSVData ** sourceDataArray, size_t count, Source * sources) {
	if (sourceDataArray == NULL) {
		return;
	}
	
	for (size_t i = 0; i < count; i++) {
		if (sourceDataArray[i] != NULL) {
			Source * source = sources;
			size_t j = 0;
			while (source != NULL && j < i) {
				source = source->next;
				j++;
			}
			if (source != NULL && source->csvFile != NULL && source->sourceIdentifier == NULL) {
				destroyCSVData(sourceDataArray[i]);
			}
		}
	}
	free(sourceDataArray);
}

/**
 * Determina si un CSVData debe liberarse o no
 * Retorna true si debe liberarse (fue leído desde archivo o es una copia combinada)
 * Retorna false si viene del mapa (sourceIdentifier)
 */
static bool _shouldFreeCSVData(CSVData * data, Chart * chart, size_t sourceListCount) {
	if (data == NULL || chart == NULL) {
		return false;
	}
	
	if (sourceListCount > 1) {
		return true;
	}
	
	if (sourceListCount == 1) {
		Source * firstSource = chart->sources;
		if (firstSource != NULL && firstSource->csvFile != NULL && firstSource->sourceIdentifier == NULL) {
			return true;
		}
		return false;
	}
	
	return false;
}

/**
 * Combina múltiples CSVData en uno solo
 * Todas las sources deben tener las mismas columnas
 */
static CSVData * _combineCSVData(CSVData ** dataArray, size_t count) {
	if (dataArray == NULL || count == 0) {
		return NULL;
	}
	
	if (count == 1) {
		return dataArray[0];
	}
	
	size_t headerCount = dataArray[0]->headerCount;
	for (size_t i = 1; i < count; i++) {
		if (dataArray[i] == NULL || dataArray[i]->headerCount != headerCount) {
			return NULL;
		}
		for (size_t j = 0; j < headerCount; j++) {
			if (dataArray[0]->headers[j] == NULL || dataArray[i]->headers[j] == NULL ||
			    strcmp(dataArray[0]->headers[j], dataArray[i]->headers[j]) != 0) {
				return NULL;
			}
		}
	}
	
	size_t totalRows = 0;
	for (size_t i = 0; i < count; i++) {
		CSVRow * current = dataArray[i]->rows;
		while (current != NULL) {
			totalRows++;
			current = current->next;
		}
	}
	
	CSVData * combined = calloc(1, sizeof(CSVData));
	if (combined == NULL) {
		return NULL;
	}
	
	combined->headerCount = headerCount;
	combined->headers = calloc(headerCount, sizeof(char*));
	if (combined->headers == NULL) {
		free(combined);
		return NULL;
	}
	
	for (size_t i = 0; i < headerCount; i++) {
		combined->headers[i] = strdup(dataArray[0]->headers[i]);
		if (combined->headers[i] == NULL) {
			for (size_t j = 0; j < i; j++) {
				free(combined->headers[j]);
			}
			free(combined->headers);
			free(combined);
			return NULL;
		}
	}
	
	CSVRow * lastRow = NULL;
	for (size_t i = 0; i < count; i++) {
		CSVRow * current = dataArray[i]->rows;
		while (current != NULL) {
			CSVRow * newRow = calloc(1, sizeof(CSVRow));
			if (newRow == NULL) {
				destroyCSVData(combined);
				return NULL;
			}
			
			newRow->columnCount = headerCount;
			newRow->values = calloc(headerCount, sizeof(char*));
			if (newRow->values == NULL) {
				free(newRow);
				destroyCSVData(combined);
				return NULL;
			}
			
			for (size_t j = 0; j < headerCount; j++) {
				if (current->values != NULL && current->values[j] != NULL) {
					newRow->values[j] = strdup(current->values[j]);
					if (newRow->values[j] == NULL) {
						for (size_t k = 0; k < j; k++) {
							free(newRow->values[k]);
						}
						free(newRow->values);
						free(newRow);
						destroyCSVData(combined);
						return NULL;
					}
				}
			}
			
			if (lastRow == NULL) {
				combined->rows = newRow;
				lastRow = newRow;
			} else {
				lastRow->next = newRow;
				lastRow = newRow;
			}
			
			current = current->next;
		}
	}
	
	combined->rowCount = totalRows;
	return combined;
}

/**
 * Verifica si una expresión contiene una función de agregación
 */
static bool _hasAggregation(Expression * expression) {
	if (expression == NULL) {
		return false;
	}
	
	if (expression->type == FACTOR && expression->factor != NULL) {
		if (expression->factor->type == AGGREGATE_FACTOR) {
			return true;
		}
		if (expression->factor->type == EXPRESSION) {
			return _hasAggregation(expression->factor->expression);
		}
	}
	
	if (expression->type == ADDITION || expression->type == SUBTRACTION ||
	    expression->type == MULTIPLICATION || expression->type == DIVISION) {
		return _hasAggregation(expression->leftExpression) || 
		       _hasAggregation(expression->rightExpression);
	}
	
	return false;
}

/**
 * Obtiene la función de agregación y columna de una expresión
 */
static bool _getAggregationInfo(Expression * expression, AggregateFunction * function, char ** columnName) {
	if (expression == NULL || function == NULL || columnName == NULL) {
		return false;
	}
	
	if (expression->type == FACTOR && expression->factor != NULL) {
		if (expression->factor->type == AGGREGATE_FACTOR) {
			*function = expression->factor->aggregate.function;
			*columnName = expression->factor->aggregate.columnName;
			return true;
		}
		if (expression->factor->type == EXPRESSION) {
			return _getAggregationInfo(expression->factor->expression, function, columnName);
		}
	}
	
	if (expression->type == ADDITION || expression->type == SUBTRACTION ||
	    expression->type == MULTIPLICATION || expression->type == DIVISION) {
		if (_getAggregationInfo(expression->leftExpression, function, columnName)) {
			return true;
		}
		return _getAggregationInfo(expression->rightExpression, function, columnName);
	}
	
	return false;
}

/**
 * Extrae el nombre de la columna de una expresión simple (identificador)
 */
static char * _extractColumnName(Expression * expression) {
	if (expression == NULL) {
		return NULL;
	}
	
	if (expression->type == FACTOR && expression->factor != NULL) {
		if (expression->factor->type == CONSTANT) {
			Constant * c = expression->factor->constant;
			if (c != NULL && c->string != NULL) {
				return strdup(c->string);
			}
		}
	}
	
	return NULL;
}

/** PUBLIC FUNCTIONS */

ChartData * processChart(Chart * chart, CSVData ** csvDataMap, const char ** sourceIdentifiers, size_t sourceCount) {
	if (chart == NULL || chart->sources == NULL) {
		logError(_logger, "Chart or sources are NULL");
		return NULL;
	}
	
	size_t sourceListCount = 0;
	Source * tempSource = chart->sources;
	while (tempSource != NULL) {
		sourceListCount++;
		tempSource = tempSource->next;
	}
	
	CSVData ** sourceDataArray = calloc(sourceListCount, sizeof(CSVData*));
	if (sourceDataArray == NULL) {
		logError(_logger, "Memory allocation failed");
		return NULL;
	}
	
	Source * source = chart->sources;
	size_t sourceIndex = 0;
	
	while (source != NULL && sourceIndex < sourceListCount) {
		CSVData * csvData = NULL;
		CSVData * dataToUse = NULL;
		
		if (source->csvFile != NULL) {
			char * csvPath = NULL;
			
			// Intentar el path directo
			FILE * testFile = fopen(source->csvFile, "r");
			if (testFile != NULL) {
				fclose(testFile);
				csvPath = source->csvFile;
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
				logError(_logger, "Cannot find CSV file: %s (tried: %s, src/test/c/data/%s, ./%s)", 
					source->csvFile, source->csvFile, source->csvFile, source->csvFile);
				_freeSourceDataArray(sourceDataArray, sourceIndex, chart->sources);
				return NULL;
			}
			
			csvData = readCSVFile(csvPath);
			if (csvData == NULL) {
				logError(_logger, "Cannot read CSV file: %s", csvPath);
				if (csvPath != source->csvFile && csvPath != NULL) {
					free(csvPath);
				}
				_freeSourceDataArray(sourceDataArray, sourceIndex, chart->sources);
				return NULL;
			}
			
			if (csvPath != source->csvFile && csvPath != NULL) {
				free(csvPath);
			}
			
			if (source->filters != NULL || source->projection != NULL) {
				ProcessedData * processed = processSourceData(csvData, source);
				if (processed == NULL) {
					destroyCSVData(csvData);
					for (size_t i = 0; i < sourceIndex; i++) {
						if (sourceDataArray[i] != NULL) {
							destroyCSVData(sourceDataArray[i]);
						}
					}
					free(sourceDataArray);
					return NULL;
				}
				
				CSVData * processedCSV = calloc(1, sizeof(CSVData));
				if (processedCSV == NULL) {
					destroyProcessedData(processed);
					destroyCSVData(csvData);
					for (size_t i = 0; i < sourceIndex; i++) {
						if (sourceDataArray[i] != NULL) {
							destroyCSVData(sourceDataArray[i]);
						}
					}
					free(sourceDataArray);
					return NULL;
				}
				
				processedCSV->headers = processed->columnNames;
				processedCSV->headerCount = processed->columnCount;
				processedCSV->rows = processed->rows;
				processedCSV->rowCount = processed->rowCount;
				free(processed);
				
				destroyCSVData(csvData);
				dataToUse = processedCSV;
			} else {
				dataToUse = csvData;
			}
		} else if (source->sourceIdentifier != NULL) {
			csvData = _findCSVData(source->sourceIdentifier, csvDataMap, sourceIdentifiers, sourceCount);
			if (csvData == NULL) {
				logError(_logger, "Source identifier not found: %s (searched in %zu sources)", source->sourceIdentifier, sourceCount);
				_freeSourceDataArray(sourceDataArray, sourceIndex, chart->sources);
				return NULL;
			}
			dataToUse = csvData;
		} else {
			logError(_logger, "Source has neither CSV file nor source identifier");
			for (size_t i = 0; i < sourceIndex; i++) {
				if (sourceDataArray[i] != NULL) {
					destroyCSVData(sourceDataArray[i]);
				}
			}
			free(sourceDataArray);
			return NULL;
		}
		
		sourceDataArray[sourceIndex] = dataToUse;
		sourceIndex++;
		source = source->next;
	}
	
	CSVData * finalData = _combineCSVData(sourceDataArray, sourceListCount);
	if (finalData == NULL) {
		logError(_logger, "Failed to combine sources");
		for (size_t i = 0; i < sourceListCount; i++) {
			if (sourceDataArray[i] != NULL) {
				Source * source = chart->sources;
				size_t j = 0;
				while (source != NULL && j < i) {
					source = source->next;
					j++;
				}
				if (source != NULL && source->csvFile != NULL && source->sourceIdentifier == NULL) {
					destroyCSVData(sourceDataArray[i]);
				}
			}
		}
		free(sourceDataArray);
		return NULL;
	}
	
	if (sourceListCount > 1) {
		for (size_t i = 0; i < sourceListCount; i++) {
			if (sourceDataArray[i] != NULL) {
				Source * source = chart->sources;
				size_t j = 0;
				while (source != NULL && j < i) {
					source = source->next;
					j++;
				}
				if (source != NULL && source->csvFile != NULL && source->sourceIdentifier == NULL) {
					destroyCSVData(sourceDataArray[i]);
				}
			}
		}
	}
	free(sourceDataArray);
	
	bool hasAggregation = _hasAggregation(chart->yExpression);
	
	ChartData * chartData = calloc(1, sizeof(ChartData));
	if (chartData == NULL) {
		if (_shouldFreeCSVData(finalData, chart, sourceListCount)) {
			destroyCSVData(finalData);
		}
		return NULL;
	}
	
	int xColumnIndex = -1;
	if (chart->xColumn != NULL) {
		for (size_t i = 0; i < finalData->headerCount; i++) {
			if (finalData->headers[i] != NULL && strcmp(finalData->headers[i], chart->xColumn) == 0) {
				xColumnIndex = (int)i;
				break;
			}
		}
	}
	if (hasAggregation) {
		AggregateFunction aggFunction;
		char * aggColumn = NULL;
		if (!_getAggregationInfo(chart->yExpression, &aggFunction, &aggColumn)) {
			logError(_logger, "Cannot extract aggregation info from expression");
			if (_shouldFreeCSVData(finalData, chart, sourceListCount)) {
				destroyCSVData(finalData);
			}
			free(chartData);
			return NULL;
		}

		int xIdx = -1;
		if (chart->xColumn != NULL) {
			for (size_t i = 0; i < finalData->headerCount; i++) {
				if (finalData->headers[i] != NULL && strcmp(finalData->headers[i], chart->xColumn) == 0) {
					xIdx = (int)i;
					break;
				}
			}
		}
		int yIdx = -1;
		if (aggColumn != NULL) {
			yIdx = getColumnIndex(finalData, aggColumn);
		}

		size_t capacity = 8;
		size_t groupCount = 0;
		char ** labels = (char **) malloc(sizeof(char*) * (capacity + 1));
		double * values = (double *) malloc(sizeof(double) * capacity);
		size_t * counts = (size_t *) malloc(sizeof(size_t) * capacity);
		if (labels == NULL || values == NULL || counts == NULL) {
			logError(_logger, "Out of memory while allocating grouping structures");
			free(labels); free(values); free(counts);
			if (_shouldFreeCSVData(finalData, chart, sourceListCount)) destroyCSVData(finalData);
			free(chartData);
			return NULL;
		}
		for (size_t i = 0; i < capacity; ++i) {
			labels[i] = NULL;
			values[i] = 0.0;
			counts[i] = 0;
		}
		labels[capacity] = NULL;

		CSVRow * cur = finalData->rows;
		while (cur != NULL) {
			const char * xVal = (xIdx >= 0) ? getCellValue(cur, (size_t)xIdx) : "";
			if (xVal == NULL) xVal = "";

			size_t foundPos = (size_t)-1;
			for (size_t p = 0; p < groupCount; ++p) {
				if (labels[p] != NULL && strcmp(labels[p], xVal) == 0) {
					foundPos = p;
					break;
				}
			}

			if (foundPos == (size_t)-1) {
				if (groupCount == capacity) {
				size_t newCap = capacity * 2;
				char ** l2 = (char **) realloc(labels, sizeof(char*) * (newCap + 1));
				double * v2 = (double *) realloc(values, sizeof(double) * newCap);
				size_t * c2 = (size_t *) realloc(counts, sizeof(size_t) * newCap);
				if (l2 == NULL || v2 == NULL || c2 == NULL) {
					logError(_logger, "Out of memory while expanding grouping structures");
					free(l2); free(v2); free(c2);
					for (size_t i = 0; i < groupCount; ++i) free(labels[i]);
					free(labels); free(values); free(counts);
					if (_shouldFreeCSVData(finalData, chart, sourceListCount)) destroyCSVData(finalData);
					free(chartData);
					return NULL;
				}
				labels = l2; values = v2; counts = c2;
				for (size_t i = capacity; i < newCap; ++i) {
					labels[i] = NULL;
					values[i] = 0.0;
					counts[i] = 0;
				}
				capacity = newCap;
				labels[capacity] = NULL;
			}

			labels[groupCount] = strdup(xVal ? xVal : "");
			if (labels[groupCount] == NULL) {
				logError(_logger, "Out of memory while duplicating label");
				for (size_t i = 0; i < groupCount; ++i) free(labels[i]);
				free(labels); free(values); free(counts);
				if (_shouldFreeCSVData(finalData, chart, sourceListCount)) destroyCSVData(finalData);
				free(chartData);
				return NULL;
			}

			if (aggFunction == AGG_COUNT) {
				values[groupCount] = 1.0;
				counts[groupCount] = 1;
			} else if (aggFunction == AGG_SUM) {
				double yv = 0.0;
				if (yIdx >= 0) {
					const char * cell = getCellValue(cur, (size_t)yIdx);
					if (cell) yv = atof(cell);
				}
				values[groupCount] = yv;
				counts[groupCount] = 1;
			} else if (aggFunction == AGG_MIN || aggFunction == AGG_MAX) {
				double yv = 0.0;
				if (yIdx >= 0) {
					const char * cell = getCellValue(cur, (size_t)yIdx);
					if (cell) yv = atof(cell);
				}
				values[groupCount] = yv;
				counts[groupCount] = 1;
			} else if (aggFunction == AGG_AVERAGE) {
				double yv = 0.0;
				if (yIdx >= 0) {
					const char * cell = getCellValue(cur, (size_t)yIdx);
					if (cell) yv = atof(cell);
				}
				values[groupCount] = yv;
				counts[groupCount] = 1;
			} else {
				logError(_logger, "Unsupported aggregation function");
				for (size_t i = 0; i < groupCount; ++i) free(labels[i]);
				free(labels); free(values); free(counts);
				if (_shouldFreeCSVData(finalData, chart, sourceListCount)) destroyCSVData(finalData);
				free(chartData);
				return NULL;
			}

			foundPos = groupCount;
			groupCount++;
		} else {
			if (aggFunction == AGG_COUNT) {
				values[foundPos] += 1.0;
				counts[foundPos] += 1;
			} else if (aggFunction == AGG_SUM) {
				double yv = 0.0;
				if (yIdx >= 0) {
					const char * cell = getCellValue(cur, (size_t)yIdx);
					if (cell) yv = atof(cell);
				}
				values[foundPos] += yv;
				counts[foundPos] += 1;
			} else if (aggFunction == AGG_MIN) {
				double yv = 0.0;
				if (yIdx >= 0) {
					const char * cell = getCellValue(cur, (size_t)yIdx);
					if (cell) yv = atof(cell);
				}
				if (counts[foundPos] == 0 || yv < values[foundPos]) values[foundPos] = yv;
				counts[foundPos] += 1;
			} else if (aggFunction == AGG_MAX) {
				double yv = 0.0;
				if (yIdx >= 0) {
					const char * cell = getCellValue(cur, (size_t)yIdx);
					if (cell) yv = atof(cell);
				}
				if (counts[foundPos] == 0 || yv > values[foundPos]) values[foundPos] = yv;
				counts[foundPos] += 1;
			} else if (aggFunction == AGG_AVERAGE) {
				double yv = 0.0;
				if (yIdx >= 0) {
					const char * cell = getCellValue(cur, (size_t)yIdx);
					if (cell) yv = atof(cell);
				}
				values[foundPos] += yv;
				counts[foundPos] += 1;
			}
		}

		cur = cur->next;
	}

	if (aggFunction == AGG_AVERAGE) {
		for (size_t i = 0; i < groupCount; ++i) {
			if (counts[i] > 0) {
				values[i] = values[i] / (double)counts[i];
			}
		}
	}

	chartData->dataCount = groupCount;
	chartData->labels = calloc(groupCount + 1, sizeof(char*));
	chartData->values = calloc(groupCount, sizeof(double));
	if (chartData->labels == NULL || chartData->values == NULL) {
		logError(_logger, "Out of memory while finalizing chart data");
		for (size_t i = 0; i < groupCount; ++i) free(labels[i]);
		free(labels); free(values); free(counts);
		if (_shouldFreeCSVData(finalData, chart, sourceListCount)) destroyCSVData(finalData);
		free(chartData);
		return NULL;
	}
	for (size_t i = 0; i < groupCount; ++i) {
		chartData->labels[i] = labels[i];
		chartData->values[i] = values[i];
	}
	chartData->labels[groupCount] = NULL;

	if (chart->yAlias != NULL) {
		chartData->yLabel = strdup(chart->yAlias);
	} else {
		chartData->yLabel = aggColumn != NULL ? strdup(aggColumn) : strdup("Value");
	}

	free(labels); free(values); free(counts);
	}else {
		CSVRow * current = finalData->rows;
		size_t count = 0;
		
		while (current != NULL) {
			count++;
			current = current->next;
		}
		
		if (count == 0) {
			if (_shouldFreeCSVData(finalData, chart, sourceListCount)) {
				destroyCSVData(finalData);
			}
			free(chartData);
			return NULL;
		}
		
		chartData->dataCount = count;
		chartData->labels = calloc(count + 1, sizeof(char*));
		chartData->values = calloc(count, sizeof(double));
		
		if (chartData->labels == NULL || chartData->values == NULL) {
			if (_shouldFreeCSVData(finalData, chart, sourceListCount)) {
				destroyCSVData(finalData);
			}
			free(chartData);
			return NULL;
		}
		
		current = finalData->rows;
		size_t index = 0;
		while (current != NULL && index < count) {
			if (xColumnIndex >= 0) {
				const char * xValue = getCellValue(current, (size_t)xColumnIndex);
				chartData->labels[index] = xValue != NULL ? strdup(xValue) : strdup("");
			} else {
				chartData->labels[index] = strdup("");
			}
			
			chartData->values[index] = evaluateExpression(chart->yExpression, current, finalData);
			
			current = current->next;
			index++;
		}
		chartData->labels[count] = NULL;
		
		if (chart->yAlias != NULL) {
			chartData->yLabel = strdup(chart->yAlias);
		} else {
			char * columnName = _extractColumnName(chart->yExpression);
			if (columnName != NULL) {
				chartData->yLabel = columnName;
			} else {
				chartData->yLabel = strdup("Y");
			}
		}
	}
	
	if (_shouldFreeCSVData(finalData, chart, sourceListCount)) {
		destroyCSVData(finalData);
	}
	
	return chartData;
}

void destroyChartData(ChartData * data) {
	if (data != NULL) {
		if (data->labels != NULL) {
			for (size_t i = 0; i < data->dataCount; i++) {
				if (data->labels[i] != NULL) {
					free(data->labels[i]);
				}
			}
			free(data->labels);
		}
		if (data->values != NULL) {
			free(data->values);
		}
		if (data->yLabel != NULL) {
			free(data->yLabel);
		}
		free(data);
	}
}

