#include "DataProcessor.h"
#include <string.h>
#include <math.h>

/* ESTADO INTERNO DEL MÓDULO */

static Logger * _logger = NULL;

/** Cierra el estado interno del módulo. */
void _shutdownDataProcessorModule() {
	if (_logger != NULL) {
		destroyLogger(_logger);
		_logger = NULL;
	}
}

ModuleDestructor initializeDataProcessorModule() {
	_logger = createLogger("DataProcessor");
	return _shutdownDataProcessorModule;
}

/** FUNCIONES PRIVADAS */

/**
 * Evalúa una condición de filtro sobre una fila
 */
static bool _evaluateFilterCondition(CSVRow * row, CSVData * csvData, FilterCondition * filter) {
	if (row == NULL || filter == NULL || csvData == NULL) {
		return false;
	}
	
	int columnIndex = getColumnIndex(csvData, filter->columnName);
	if (columnIndex < 0) {
		return false;
	}
	
	const char * cellValue = getCellValue(row, (size_t)columnIndex);
	if (cellValue == NULL) {
		return false;
	}
	
	switch (filter->valueType) {
		case FILTER_VALUE_STRING: {
			const char * filterValue = filter->value.stringValue;
			int cmp = strcmp(cellValue, filterValue);
			switch (filter->operator) {
				case FILTER_EQ: return cmp == 0;
				case FILTER_GT: return cmp > 0;
				case FILTER_LT: return cmp < 0;
				case FILTER_GE: return cmp >= 0;
				case FILTER_LE: return cmp <= 0;
				default: return false;
			}
		}
		case FILTER_VALUE_INT: {
			int cellInt = atoi(cellValue);
			int filterInt = filter->value.intValue;
			switch (filter->operator) {
				case FILTER_EQ: return cellInt == filterInt;
				case FILTER_GT: return cellInt > filterInt;
				case FILTER_LT: return cellInt < filterInt;
				case FILTER_GE: return cellInt >= filterInt;
				case FILTER_LE: return cellInt <= filterInt;
				default: return false;
			}
		}
		default:
			return false;
	}
}

/**
 * Evalúa todos los filtros (AND lógico)
 */
static bool _evaluateFilters(CSVRow * row, CSVData * csvData, FilterCondition * filters) {
	if (filters == NULL) {
		return true;
	}
	
	FilterCondition * current = filters;
	while (current != NULL) {
		if (!_evaluateFilterCondition(row, csvData, current)) {
			return false;
		}
		current = current->next;
	}
	return true;
}

/**
 * Copia una fila CSV
 */
static CSVRow * _copyCSVRow(CSVRow * source, size_t * columnIndices, size_t columnCount) {
	if (source == NULL) {
		return NULL;
	}
	
	CSVRow * row = calloc(1, sizeof(CSVRow));
	if (row == NULL) {
		return NULL;
	}
	
	row->columnCount = columnCount;
	row->values = calloc(columnCount + 1, sizeof(char*));
	if (row->values == NULL) {
		free(row);
		return NULL;
	}
	
	for (size_t i = 0; i < columnCount; i++) {
		if (columnIndices != NULL) {
			const char * value = getCellValue(source, columnIndices[i]);
			if (value != NULL) {
				row->values[i] = strdup(value);
				if (row->values[i] == NULL) {
					// Liberar lo que ya se copió
					for (size_t j = 0; j < i; j++) {
						free(row->values[j]);
					}
					free(row->values);
					free(row);
					return NULL;
				}
			}
		} else {
			// Copiar todas las columnas
			if (i < source->columnCount && source->values[i] != NULL) {
				row->values[i] = strdup(source->values[i]);
				if (row->values[i] == NULL) {
					for (size_t j = 0; j < i; j++) {
						free(row->values[j]);
					}
					free(row->values);
					free(row);
					return NULL;
				}
			}
		}
	}
	row->values[columnCount] = NULL;
	row->next = NULL;
	return row;
}

/**
 * Libera una fila procesada
 */
static void _destroyProcessedRow(CSVRow * row) {
	if (row != NULL) {
		if (row->values != NULL) {
			for (size_t i = 0; i < row->columnCount; i++) {
				if (row->values[i] != NULL) {
					free(row->values[i]);
				}
			}
			free(row->values);
		}
		_destroyProcessedRow(row->next);
		free(row);
	}
}

/** FUNCIONES PÚBLICAS */

CSVData * applyFilters(CSVData * csvData, FilterCondition * filters) {
	if (csvData == NULL || filters == NULL) {
		return csvData;
	}
	
	CSVData * filtered = calloc(1, sizeof(CSVData));
	if (filtered == NULL) {
		return NULL;
	}
	
	filtered->headerCount = csvData->headerCount;
	filtered->headers = calloc(csvData->headerCount + 1, sizeof(char*));
	if (filtered->headers == NULL) {
		free(filtered);
		return NULL;
	}
	
	for (size_t i = 0; i < csvData->headerCount; i++) {
		if (csvData->headers[i] != NULL) {
			filtered->headers[i] = strdup(csvData->headers[i]);
			if (filtered->headers[i] == NULL) {
				for (size_t j = 0; j < i; j++) {
					free(filtered->headers[j]);
				}
				free(filtered->headers);
				free(filtered);
				return NULL;
			}
		}
	}
	filtered->headers[csvData->headerCount] = NULL;
	
	CSVRow * lastRow = NULL;
	CSVRow * current = csvData->rows;
	
	while (current != NULL) {
		if (_evaluateFilters(current, csvData, filters)) {
			CSVRow * newRow = _copyCSVRow(current, NULL, current->columnCount);
			if (newRow != NULL) {
				if (lastRow == NULL) {
					filtered->rows = newRow;
				} else {
					lastRow->next = newRow;
				}
				lastRow = newRow;
				filtered->rowCount++;
			}
		}
		current = current->next;
	}
	
	return filtered;
}

ProcessedData * applyProjection(CSVData * csvData, Projection * projection) {
	if (csvData == NULL) {
		return NULL;
	}
	
	ProcessedData * processed = calloc(1, sizeof(ProcessedData));
	if (processed == NULL) {
		return NULL;
	}
	
	if (projection == NULL || projection->columns == NULL || projection->columnCount == 0) {
		processed->columnCount = csvData->headerCount;
		processed->columnNames = calloc(csvData->headerCount + 1, sizeof(char*));
		if (processed->columnNames == NULL) {
			free(processed);
			return NULL;
		}
		for (size_t i = 0; i < csvData->headerCount; i++) {
			if (csvData->headers[i] != NULL) {
				processed->columnNames[i] = strdup(csvData->headers[i]);
			}
		}
		processed->columnNames[csvData->headerCount] = NULL;
		
		CSVRow * lastRow = NULL;
		CSVRow * current = csvData->rows;
		while (current != NULL) {
			CSVRow * newRow = _copyCSVRow(current, NULL, current->columnCount);
			if (newRow != NULL) {
				if (lastRow == NULL) {
					processed->rows = newRow;
				} else {
					lastRow->next = newRow;
				}
				lastRow = newRow;
				processed->rowCount++;
			}
			current = current->next;
		}
		return processed;
	}
	
	processed->columnCount = projection->columnCount;
	processed->columnNames = calloc(projection->columnCount + 1, sizeof(char*));
	if (processed->columnNames == NULL) {
		free(processed);
		return NULL;
	}
	
	size_t * columnIndices = calloc(projection->columnCount, sizeof(size_t));
	if (columnIndices == NULL) {
		free(processed->columnNames);
		free(processed);
		return NULL;
	}
	
	for (size_t i = 0; i < projection->columnCount; i++) {
		if (projection->columns[i] != NULL) {
			processed->columnNames[i] = strdup(projection->columns[i]);
			columnIndices[i] = (size_t)getColumnIndex(csvData, projection->columns[i]);
			if (columnIndices[i] == (size_t)-1) {
				logError(_logger, "Column not found in projection: %s", projection->columns[i]);
				for (size_t j = 0; j < i; j++) {
					free(processed->columnNames[j]);
				}
				free(processed->columnNames);
				free(columnIndices);
				free(processed);
				return NULL;
			}
		}
	}
	processed->columnNames[projection->columnCount] = NULL;
	
	CSVRow * lastRow = NULL;
	CSVRow * current = csvData->rows;
	while (current != NULL) {
		CSVRow * newRow = _copyCSVRow(current, columnIndices, projection->columnCount);
		if (newRow != NULL) {
			if (lastRow == NULL) {
				processed->rows = newRow;
			} else {
				lastRow->next = newRow;
			}
			lastRow = newRow;
			processed->rowCount++;
		}
		current = current->next;
	}
	
	free(columnIndices);
	return processed;
}

double evaluateExpression(Expression * expression, CSVRow * row, CSVData * csvData) {
	if (expression == NULL || row == NULL || csvData == NULL) {
		return 0.0;
	}
	
	switch (expression->type) {
		case ADDITION:
		case SUBTRACTION:
		case MULTIPLICATION:
		case DIVISION: {
			double left = evaluateExpression(expression->leftExpression, row, csvData);
			double right = evaluateExpression(expression->rightExpression, row, csvData);
			double result = 0.0;
			switch (expression->type) {
				case ADDITION: result = left + right; break;
				case SUBTRACTION: result = left - right; break;
				case MULTIPLICATION: result = left * right; break;
				case DIVISION: result = right != 0.0 ? left / right : 0.0; break;
				default: result = 0.0; break;
			}
			return result;
		}
		case FACTOR: {
			if (expression->factor == NULL) {
				return 0.0;
			}
			switch (expression->factor->type) {
				case CONSTANT: {
					Constant * c = expression->factor->constant;
					if (c != NULL) {
						if (c->string != NULL) {
							int colIndex = getColumnIndex(csvData, c->string);
							if (colIndex < 0) {
								logError(_logger, "Column '%s' not found in CSV. Available columns:", c->string);
								if (csvData != NULL && csvData->headers != NULL) {
									for (size_t i = 0; i < csvData->headerCount; i++) {
										if (csvData->headers[i] != NULL) {
											logError(_logger, "  [%zu] '%s'", i, csvData->headers[i]);
										}
									}
								} else {
									logError(_logger, "  (CSV data or headers are NULL)");
								}
								return 0.0;
							}
							const char * value = getCellValue(row, (size_t)colIndex);
							if (value == NULL) {
								logError(_logger, "Cell value is NULL for column '%s' at index %d", c->string, colIndex);
								return 0.0;
							}
							double result = atof(value);
							return result;
						} else if (c->number != 0.0) {
							return c->number;
						} else {
							return (double)c->value;
						}
					}
					return 0.0;
				}
				case AGGREGATE_FACTOR: {
					return 0.0;
				}
				case EXPRESSION: {
					return evaluateExpression(expression->factor->expression, row, csvData);
				}
				default:
					return 0.0;
			}
		}
		default:
			return 0.0;
	}
}

double applyAggregation(CSVData * csvData, const char * columnName, AggregateFunction function) {
	if (csvData == NULL || columnName == NULL) {
		return 0.0;
	}
	
	int colIndex = getColumnIndex(csvData, columnName);
	if (colIndex < 0) {
		return 0.0;
	}
	
	CSVRow * current = csvData->rows;
	if (current == NULL) {
		return 0.0;
	}
	
	double sum = 0.0;
	double min = 0.0;
	double max = 0.0;
	size_t count = 0;
	bool first = true;
	
	while (current != NULL) {
		const char * value = getCellValue(current, (size_t)colIndex);
		if (value != NULL) {
			double numValue = atof(value);
			if (first) {
				min = max = numValue;
				first = false;
			}
			sum += numValue;
			if (numValue < min) min = numValue;
			if (numValue > max) max = numValue;
			count++;
		}
		current = current->next;
	}
	
	switch (function) {
		case AGG_AVERAGE:
			return count > 0 ? sum / count : 0.0;
		case AGG_SUM:
			return sum;
		case AGG_MIN:
			return first ? 0.0 : min;
		case AGG_MAX:
			return first ? 0.0 : max;
		case AGG_COUNT:
			return (double)count;
		default:
			return 0.0;
	}
}

ProcessedData * processSourceData(CSVData * csvData, Source * source) {
	if (csvData == NULL || source == NULL) {
		return NULL;
	}
	
	CSVData * filtered = applyFilters(csvData, source->filters);
	if (filtered == NULL && source->filters != NULL) {
		return NULL;
	}
	
	CSVData * dataToProject = (filtered != NULL) ? filtered : csvData;
	
	ProcessedData * processed = applyProjection(dataToProject, source->projection);
	
	if (filtered != NULL && filtered != csvData) {
		destroyCSVData(filtered);
	}
	
	return processed;
}

void destroyProcessedData(ProcessedData * data) {
	if (data != NULL) {
		if (data->columnNames != NULL) {
			for (size_t i = 0; i < data->columnCount; i++) {
				if (data->columnNames[i] != NULL) {
					free(data->columnNames[i]);
				}
			}
			free(data->columnNames);
		}
		_destroyProcessedRow(data->rows);
		free(data);
	}
}

