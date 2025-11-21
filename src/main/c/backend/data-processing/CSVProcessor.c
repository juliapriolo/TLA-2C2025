#include "CSVProcessor.h"
#include <stdio.h>
#include <string.h>

/* MODULE INTERNAL STATE */

static Logger * _logger = NULL;

/** Shutdown module's internal state. */
void _shutdownCSVProcessorModule() {
	if (_logger != NULL) {
		// logDebugging(_logger, "Destroying module: CSVProcessor...");
		destroyLogger(_logger);
		_logger = NULL;
	}
}

ModuleDestructor initializeCSVProcessorModule() {
	_logger = createLogger("CSVProcessor");
	return _shutdownCSVProcessorModule;
}

/** PRIVATE FUNCTIONS */

/**
 * Parsea una línea CSV, manejando comillas y escapes
 */
static char ** _parseCSVLine(const char * line, size_t * columnCount) {
	if (line == NULL || strlen(line) == 0) {
		*columnCount = 0;
		return NULL;
	}
	
	// Contar columnas (separadas por comas, respetando comillas)
	size_t count = 1;
	bool inQuotes = false;
	for (const char * p = line; *p != '\0'; p++) {
		if (*p == '"') {
			inQuotes = !inQuotes;
		} else if (*p == ',' && !inQuotes) {
			count++;
		}
	}
	
	char ** columns = calloc(count + 1, sizeof(char*));
	if (columns == NULL) {
		*columnCount = 0;
		return NULL;
	}
	
	// Parsear columnas
	size_t colIndex = 0;
	const char * start = line;
	inQuotes = false;
	
	for (const char * p = line; *p != '\0'; p++) {
		if (*p == '"') {
			inQuotes = !inQuotes;
		} else if ((*p == ',' && !inQuotes) || *(p + 1) == '\0') {
			size_t len = (*(p + 1) == '\0' && *p != ',') ? (p - start + 1) : (p - start);
			if (len > 0) {
				columns[colIndex] = calloc(len + 1, sizeof(char));
				if (columns[colIndex] != NULL) {
					strncpy(columns[colIndex], start, len);
					columns[colIndex][len] = '\0';
					// Remover comillas si existen
					if (columns[colIndex][0] == '"' && columns[colIndex][len - 1] == '"') {
						memmove(columns[colIndex], columns[colIndex] + 1, len - 2);
						columns[colIndex][len - 2] = '\0';
					}
					colIndex++;
				}
			}
			start = p + 1;
		}
	}
	
	columns[count] = NULL;
	*columnCount = count;
	return columns;
}

/**
 * Libera una fila CSV
 */
static void _destroyCSVRow(CSVRow * row) {
	if (row != NULL) {
		if (row->values != NULL) {
			for (size_t i = 0; i < row->columnCount; i++) {
				if (row->values[i] != NULL) {
					free(row->values[i]);
				}
			}
			free(row->values);
		}
		_destroyCSVRow(row->next);
		free(row);
	}
}

/** PUBLIC FUNCTIONS */

CSVData * readCSVFile(const char * filePath) {
	if (filePath == NULL) {
		logError(_logger, "CSV file path is NULL");
		return NULL;
	}
	
	FILE * file = fopen(filePath, "r");
	if (file == NULL) {
		logError(_logger, "Cannot open CSV file: %s", filePath);
		return NULL;
	}
	
	CSVData * data = calloc(1, sizeof(CSVData));
	if (data == NULL) {
		fclose(file);
		return NULL;
	}
	
	char line[4096]; // Buffer para leer líneas
	
	// Leer headers (primera línea)
	if (fgets(line, sizeof(line), file) != NULL) {
		// Remover newline
		size_t len = strlen(line);
		if (len > 0 && line[len - 1] == '\n') {
			line[len - 1] = '\0';
			len--;
		}
		if (len > 0 && line[len - 1] == '\r') {
			line[len - 1] = '\0';
		}
		
		data->headers = _parseCSVLine(line, &data->headerCount);
		if (data->headers == NULL) {
			logError(_logger, "Failed to parse CSV headers");
			fclose(file);
			free(data);
			return NULL;
		}
	} else {
		logError(_logger, "CSV file is empty");
		fclose(file);
		free(data);
		return NULL;
	}
	
	// Leer filas
	CSVRow * lastRow = NULL;
	while (fgets(line, sizeof(line), file) != NULL) {
		// Remover newline
		size_t len = strlen(line);
		if (len > 0 && line[len - 1] == '\n') {
			line[len - 1] = '\0';
			len--;
		}
		if (len > 0 && line[len - 1] == '\r') {
			line[len - 1] = '\0';
		}
		
		// Saltar líneas vacías
		if (strlen(line) == 0) {
			continue;
		}
		
		CSVRow * row = calloc(1, sizeof(CSVRow));
		if (row == NULL) {
			break;
		}
		
		row->values = _parseCSVLine(line, &row->columnCount);
		if (row->values == NULL || row->columnCount != data->headerCount) {
			logError(_logger, "Row column count mismatch: expected %zu, got %zu", data->headerCount, row->columnCount);
			free(row);
			continue;
		}
		
		row->next = NULL;
		if (lastRow == NULL) {
			data->rows = row;
		} else {
			lastRow->next = row;
		}
		lastRow = row;
		data->rowCount++;
	}
	
	fclose(file);
	
	// logDebugging(_logger, "CSV file loaded: %zu headers, %zu rows", data->headerCount, data->rowCount);
	return data;
}

void destroyCSVData(CSVData * data) {
	if (data != NULL) {
		if (data->headers != NULL) {
			for (size_t i = 0; i < data->headerCount; i++) {
				if (data->headers[i] != NULL) {
					free(data->headers[i]);
				}
			}
			free(data->headers);
		}
		_destroyCSVRow(data->rows);
		free(data);
	}
}

int getColumnIndex(CSVData * data, const char * columnName) {
	if (data == NULL || columnName == NULL || data->headers == NULL) {
		return -1;
	}
	
	for (size_t i = 0; i < data->headerCount; i++) {
		if (data->headers[i] != NULL && strcmp(data->headers[i], columnName) == 0) {
			return (int)i;
		}
	}
	
	return -1;
}

const char * getCellValue(CSVRow * row, size_t columnIndex) {
	if (row == NULL || columnIndex >= row->columnCount || row->values == NULL) {
		return NULL;
	}
	return row->values[columnIndex];
}

