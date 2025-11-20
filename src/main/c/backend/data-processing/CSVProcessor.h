#ifndef CSV_PROCESSOR_HEADER
#define CSV_PROCESSOR_HEADER

#include "../../support/logging/Logger.h"
#include "../../support/type/ModuleDestructor.h"
#include <stdbool.h>
#include <stdlib.h>

/** Initialize module's internal state. */
ModuleDestructor initializeCSVProcessorModule();

/**
 * Estructura para representar una fila de datos CSV
 */
typedef struct CSVRow {
	char ** values;      // Array de strings, uno por columna
	size_t columnCount;  // Número de columnas
	struct CSVRow * next; // Para lista enlazada
} CSVRow;

/**
 * Estructura para representar un archivo CSV completo
 */
typedef struct CSVData {
	char ** headers;     // Nombres de las columnas
	size_t headerCount;  // Número de columnas
	CSVRow * rows;       // Lista enlazada de filas
	size_t rowCount;     // Número de filas
} CSVData;

/**
 * Lee y parsea un archivo CSV
 * @param filePath Ruta al archivo CSV
 * @return CSVData* Estructura con los datos parseados, o NULL si hay error
 */
CSVData * readCSVFile(const char * filePath);

/**
 * Libera la memoria de un CSVData
 */
void destroyCSVData(CSVData * data);

/**
 * Obtiene el índice de una columna por su nombre
 * @param data Datos CSV
 * @param columnName Nombre de la columna
 * @return Índice de la columna o -1 si no existe
 */
int getColumnIndex(CSVData * data, const char * columnName);

/**
 * Obtiene el valor de una celda específica
 * @param row Fila de datos
 * @param columnIndex Índice de la columna
 * @return String con el valor o NULL si no existe
 */
const char * getCellValue(CSVRow * row, size_t columnIndex);

#endif

