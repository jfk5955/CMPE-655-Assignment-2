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
        
        case PART_MODE_STATIC_STRIPS_VERTICAL:
            slaveStaticContinuousColumns(data);
            break;
        
        case PART_MODE_STATIC_CYCLES_HORIZONTAL:
            slaveStaticCyclicalRows(data);
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
    delete[] region.pixels;
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
    int pixelsSize = (3 * region.pixelsWidth * region.pixelsHeight) + 1;
    region.pixels = new float[pixelsSize];

    // Render our region
    renderRegion(data, &region);

    // Send our results
    comp_stop = MPI_Wtime();
    comp_time = comp_stop - comp_start;

    region.pixels[pixelsSize - 1] = (float) comp_time;
    MPI_Send(region.pixels, pixelsSize, MPI_FLOAT, 0, 0, MPI_COMM_WORLD);
    delete[] region.pixels;
}

void slaveStaticCyclicalRows(ConfigData* data) {
    double comp_start, comp_stop, comp_time;
    comp_start = MPI_Wtime();

    // Describe our region
    RenderRegion region;
    region.xInImage = 0;
    region.xInPixels = 0;
    region.width = data->width;
    region.height = data->cycleSize;

    // Number of strips, maximum number of strips we handle
    int totalSubregions = (data->height / data->cycleSize) + ((data->height % data->cycleSize != 0) ? 1 : 0);
    int maxSubregions = (totalSubregions / data->mpi_procs) + 1;

    region.pixelsWidth = data->width;
    region.pixelsHeight = maxSubregions * data->cycleSize;

    // Pixels includes 1 extra entry for computation time
    int pixelsSize = (3 * region.pixelsWidth * region.pixelsHeight) + 1;
    region.pixels = new float[pixelsSize];

    // Render our subregions
    region.yInImage = (data->mpi_rank * data->cycleSize);
    region.yInPixels = 0;

    while(region.yInImage < data->height) {
        // Make sure we don't overrun
        if(region.yInImage + data->cycleSize >= data->height) {
            region.height = data->height - region.yInImage;
        }

        // Render subregion
        renderRegion(data, &region);

        region.yInImage += data->cycleSize * data->mpi_procs;
        region.yInPixels += data->cycleSize;
    }

    // Send our results
    comp_stop = MPI_Wtime();
    comp_time = comp_stop - comp_start;

    region.pixels[pixelsSize - 1] = (float) comp_time;
    MPI_Send(region.pixels, pixelsSize, MPI_FLOAT, 0, 0, MPI_COMM_WORLD);
    delete[] region.pixels;
}

void slaveDynamicCentralizedQueue(ConfigData* data) {
    // TODO
}
