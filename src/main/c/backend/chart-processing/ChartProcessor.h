#ifndef CHART_PROCESSOR_HEADER
#define CHART_PROCESSOR_HEADER

#include "../../frontend/syntactic-analysis/AbstractSyntaxTree.h"
#include "../data-processing/CSVProcessor.h"
#include "../data-processing/DataProcessor.h"
#include "../../support/logging/Logger.h"
#include "../../support/type/ModuleDestructor.h"
#include <stdbool.h>
#include <stdlib.h>

/** Initialize module's internal state. */
ModuleDestructor initializeChartProcessorModule();

/**
 * Estructura para datos preparados para un chart
 */
typedef struct ChartData {
	char ** labels;        // Etiquetas para eje X
	double * values;       // Valores para eje Y
	size_t dataCount;      // Número de puntos de datos
	char * yLabel;         // Etiqueta para eje Y (alias o nombre de columna)
} ChartData;

/**
 * Procesa un Chart del AST y prepara los datos para visualización
 * @param chart Chart del AST
 * @param csvDataMap Mapa de identificadores de sources a datos CSV (implementado como lista)
 * @return ChartData* Datos preparados para el chart o NULL si hay error
 */
ChartData * processChart(Chart * chart, CSVData ** csvDataMap, const char ** sourceIdentifiers, size_t sourceCount);

/**
 * Libera datos de chart
 */
void destroyChartData(ChartData * data);

#endif

