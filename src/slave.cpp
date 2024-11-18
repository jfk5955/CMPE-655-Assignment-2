//This file contains the code that the master process will execute.

#include <iostream>
#include <mpi.h>
#include <math.h>

#include "RayTrace.h"
#include "slave.h"
#include "common.h"

void slaveMain(ConfigData* data)
{
    //Depending on the partitioning scheme, different things will happen.
    //You should have a different function for each of the required 
    //schemes that returns some values that you need to handle.
    switch (data->partitioningMode)
    {
        case PART_MODE_NONE:
            //The slave will do nothing since this means sequential operation.
            break;
        
        case PART_MODE_STATIC_STRIPS_VERTICAL:
            slaveStaticContinuousColumns(data);
            break;

        case PART_MODE_STATIC_BLOCKS:
            slaveStaticSquareBlocks(data);
            break;

        default:
            std::cout << "This mode (" << data->partitioningMode;
            std::cout << ") is not currently implemented." << std::endl;
            break;
    }
}

void slaveStaticContinuousColumns(ConfigData* data) {
    double comp_start, comp_stop, comp_time;
    comp_start = MPI_Wtime();

    // Describe our region
    int subregionWidth, ourWidth;
    subregionWidth = data->width / data->mpi_procs;

    // Last slave handles remainder, if work is evenly distributed this
    // overlaps with other slaves' communication
    if(data->mpi_rank == data->mpi_procs - 1) {
        // we're last
        ourWidth = subregionWidth + (data->width % data->mpi_procs);
    } else {
        // we're not last
        ourWidth = subregionWidth;
    }

    RenderRegion region;
    region.xInImage = subregionWidth * data->mpi_rank;
    region.yInImage = 0;
    region.xInPixels = 0;
    region.yInPixels = 0;
    region.width = ourWidth;
    region.height = data->height;
    region.pixelsWidth = region.width;
    region.pixelsHeight = region.height;

    // Pixels includes 1 extra entry for computation time
    int pixelsSize = (3 * region.pixelsWidth * region.pixelsHeight) + 1;
    region.pixels = new float[pixelsSize];

    // Render our region
    renderRegion(data, &region);

    // Send our results
    comp_stop = MPI_Wtime();
    comp_time = comp_stop - comp_start;

    region.pixels[pixelsSize - 1] = (float) comp_time;
    MPI_Send(region.pixels, pixelsSize, MPI_FLOAT, 0, 0, MPI_COMM_WORLD);
}

void slaveStaticSquareBlocks(ConfigData* data) {
    double comp_start, comp_stop, comp_time;
    comp_start = MPI_Wtime();

    // Describe our region
    int subregionWidth, subregionHeight, ourWidth, ourHeight;
    subregionWidth = data->width / ((int)sqrt(data->mpi_procs));
    subregionHeight = data->height / ((int)sqrt(data->mpi_procs));

    // Last slave handles remainder, if work is evenly distributed this
    // overlaps with other slaves' communication
    if(data->mpi_rank == data->mpi_procs - 1) {
        // we're last
        ourWidth = subregionWidth + (data->width % ((int)sqrt(data->mpi_procs)));
        ourHeight = subregionHeight + (data->height % ((int)sqrt(data->mpi_procs)));
    } else {
        // we're not last
        ourWidth = subregionWidth;
        ourHeight = subregionHeight;
    }

    RenderRegion region;
    region.xInImage = subregionWidth * (data->mpi_rank % (int)sqrt(data->mpi_procs));
    region.yInImage = subregionHeight * (data->mpi_rank / (int)sqrt(data->mpi_procs));
    region.xInPixels = 0;
    region.yInPixels = 0;
    region.width = ourWidth;
    region.height = ourHeight;
    region.pixelsWidth = region.width;
    region.pixelsHeight = region.height;

    // Pixels includes 1 extra entry for computation time
    int pixelsSize = (3 * region.pixelsWidth * region.pixelsHeight) + 1;
    region.pixels = new float[pixelsSize];

    // Render our region
    renderRegion(data, &region);

    // Send our results
    comp_stop = MPI_Wtime();
    comp_time = comp_stop - comp_start;

    region.pixels[pixelsSize - 1] = (float) comp_time;
    MPI_Send(region.pixels, pixelsSize, MPI_FLOAT, 0, 0, MPI_COMM_WORLD);
}

void slaveStaticCyclicalRows(ConfigData* data) {
    double comp_start, comp_stop, comp_time;
    comp_start = MPI_Wtime();

    // Describe our region
    RenderRegion region;
    //region.xInImage = TODO
    //region.yInImage = TODO
    region.xInPixels = 0;
    region.yInPixels = 0;
    //region.width = TODO
    //region.height = TODO
    //region.pixelsWidth = TODO
    //region.pixelsHeight = TODO

    // Pixels includes 1 extra entry for computation time
    int pixelsSize = (3 * region.pixelsWidth * region.pixelsHeight) + 1;
    region.pixels = new float[pixelsSize];

    // Render our region
    // TODO - some kind of loop which does each of our sub-regions

    // Send our results
    comp_stop = MPI_Wtime();
    comp_time = comp_stop - comp_start;

    region.pixels[pixelsSize - 1] = (float) comp_time;
    MPI_Send(region.pixels, pixelsSize, MPI_FLOAT, 0, 0, MPI_COMM_WORLD);
}

void slaveDynamicCentralizedQueue(ConfigData* data) {
    // TODO
}
