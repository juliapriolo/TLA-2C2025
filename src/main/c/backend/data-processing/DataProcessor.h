#ifndef DATA_PROCESSOR_HEADER
#define DATA_PROCESSOR_HEADER

#include "../../frontend/syntactic-analysis/AbstractSyntaxTree.h"
#include "CSVProcessor.h"
#include "../../support/logging/Logger.h"
#include "../../support/type/ModuleDestructor.h"
#include <stdbool.h>
#include <stdlib.h>

/** Initialize module's internal state. */
ModuleDestructor initializeDataProcessorModule();

/**
 * Estructura para datos procesados (después de filtros, proyecciones, etc.)
 */
typedef struct ProcessedData {
	char ** columnNames;  // Nombres de columnas resultantes
	size_t columnCount;
	CSVRow * rows;        // Filas procesadas
	size_t rowCount;
} ProcessedData;

/**
 * Procesa datos CSV aplicando filtros, proyecciones y agregaciones
 * @param csvData Datos CSV originales
 * @param source Source del AST con filtros y proyecciones
 * @return ProcessedData* Datos procesados o NULL si hay error
 */
ProcessedData * processSourceData(CSVData * csvData, Source * source);

/**
 * Aplica filtros a los datos
 * @param csvData Datos CSV
 * @param filters Lista de filtros a aplicar
 * @return CSVData* Nuevos datos filtrados (o referencia a original si no hay filtros)
 */
CSVData * applyFilters(CSVData * csvData, FilterCondition * filters);

/**
 * Aplica proyección (selecciona columnas específicas)
 * @param csvData Datos CSV
 * @param projection Proyección con columnas a seleccionar
 * @return ProcessedData* Datos con solo las columnas proyectadas
 */
ProcessedData * applyProjection(CSVData * csvData, Projection * projection);

/**
 * Evalúa una expresión sobre los datos
 * @param expression Expresión del AST
 * @param row Fila de datos actual
 * @param csvData Datos CSV (para obtener índices de columnas)
 * @return double Valor numérico resultante
 */
double evaluateExpression(Expression * expression, CSVRow * row, CSVData * csvData);

/**
 * Aplica función de agregación
 * @param csvData Datos CSV
 * @param columnName Nombre de la columna
 * @param function Función de agregación
 * @return double Valor agregado
 */
double applyAggregation(CSVData * csvData, const char * columnName, AggregateFunction function);

/**
 * Libera datos procesados
 */
void destroyProcessedData(ProcessedData * data);

#endif

