#ifndef __SLAVE_PROCESS_H__
#define __SLAVE_PROCESS_H__

#include "RayTrace.h"

void slaveMain( ConfigData *data );

/*
 * Static partitioning - contiguous strips of columns
 * Renders a strip of columns corresponding to our rank
 * 
 * @param data Scene information
 */
void slaveStaticContinuousColumns(ConfigData* data);

/*
 * Static partitioning - square blocks
 * Renders a square in the image corresponding to our rank
 * 
 * @param data Scene information
 */
void slaveStaticSquareBlocks(ConfigData* data);

/*
 * Static partitioning - cyclical rows
 * Renders strips of n contiguous rows, as many are needed
 * 
 * @param data Scene information
 */
void slaveStaticCyclicalRows(ConfigData* data);

/*
 * Dynamic partitioning - centralized queue
 * Recieves work units from a central queue.
 * 
 * @param data Scene information
 */
void slaveDynamicCentralizedQueue(ConfigData* data);

#endif
