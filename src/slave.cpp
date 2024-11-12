//This file contains the code that the master process will execute.

#include <iostream>
#include <mpi.h>
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
    RenderRegion region;
    //region.xInImage = TODO
    //region.yInImage = TODO
    region.xInPixels = 0;
    region.yInPixels = 0;
    //region.width = TODO
    //region.height = TODO
    region.pixelsWidth = region.width;
    region.pixelsHeight = region.height;

    // Pixels includes 1 extra entry for computation time
    int pixelsSize = (3 * region.pixelsWidth * region.pixelsHeight) + 1
    region.pixels = new float[pixelsSize];

    // Render our region
    renderRegion(data, &region);

    // Send our results
    comp_stop = MPI_Wtime();
    comp_time = comp_stop - comp_start;

    region.pixels[pixelsSize - 1] = (float) comp_time;
    MPI_Send(region.pixels, pixelsSize, MPI_FLOAT, 0, 0, MPI_WORLD);
}

void slaveStaticSquareBlocks(ConfigData* data) {
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
    region.pixelsWidth = region.width;
    region.pixelsHeight = region.height;

    // Pixels includes 1 extra entry for computation time
    int pixelsSize = (3 * region.pixelsWidth * region.pixelsHeight) + 1
    region.pixels = new float[pixelsSize];

    // Render our region
    renderRegion(data, &region);

    // Send our results
    comp_stop = MPI_Wtime();
    comp_time = comp_stop - comp_start;

    region.pixels[pixelsSize - 1] = (float) comp_time;
    MPI_Send(region.pixels, pixelsSize, MPI_FLOAT, 0, 0, MPI_WORLD);
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
    int pixelsSize = (3 * region.pixelsWidth * region.pixelsHeight) + 1
    region.pixels = new float[pixelsSize];

    // Render our region
    // TODO - some kind of loop which does each sub-region

    // Send our results
    comp_stop = MPI_Wtime();
    comp_time = comp_stop - comp_start;

    region.pixels[pixelsSize - 1] = (float) comp_time;
    MPI_Send(region.pixels, pixelsSize, MPI_FLOAT, 0, 0, MPI_WORLD);
}

void slaveDynamicCentralizedQueue(ConfigData* data) {
    // TODO
}
