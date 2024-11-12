#ifndef __MASTER_PROCESS_H__
#define __MASTER_PROCESS_H__

#include "RayTrace.h"

//This function is the main that only the master process
//will run.
//
//Inputs:
//    data - the ConfigData that holds the scene information.
//
//Outputs: None
void masterMain( ConfigData *data );

//This function will perform ray tracing when no MPI use was
//given.
//
//Inputs:
//    data - the ConfigData that holds the scene information.
//
//Outputs: None
void masterSequential(ConfigData *data, float* pixels);

/*
 * Static partitioning - contiguous strips of columns
 * Renders a strip of columns corresponding to our rank
 * 
 * @param data Scene information
 * @param pixels Buffer for rendered image
 */
void masterStaticContinuousColumns(ConfigData* data, float* pixels);

/*
 * Static partitioning - square blocks
 * Renders a square in the image corresponding to our rank
 * 
 * @param data Scene information
 * @param pixels Buffer for rendered image
 */
void masterStaticSquareBlocks(ConfigData* data, float* pixels);

/*
 * Static partitioning - cyclical rows
 * Renders strips of n contiguous rows, as many are needed
 * 
 * @param data Scene information
 * @param pixels Buffer for rendered image
 */
void masterStaticCyclicalRows(ConfigData* data, float* pixels);

/*
 * Dynamic partitioning - centralized queue
 * Recieves work units from a central queue.
 * 
 * @param data Scene information
 * @param pixels Buffer for rendered image
 */
void masterDynamicCentralizedQueue(ConfigData* data, float* pixels);

#endif
