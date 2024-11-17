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
        
        case PART_MODE_DYNAMIC:
            slaveDynamicCentralizedQueue(data);
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
    double comp_start, comp_stop, comp_time;
    MPI_Status status;

    // Describe the region of a tile
    RenderRegion region;
    region.xInPixels = 0;
    region.yInPixels = 0;
    region.pixelsWidth = data->dynamicBlockWidth;
    region.pixelsHeight = data->dynamicBlockHeight;

    // Pixels includes 3 extra entries for x, y, computation time
    int pixelsSize = (3 * region.pixelsWidth * region.pixelsHeight) + 3;
    region.pixels = new float[pixelsSize];

    int* workPacket = new int[2];

    /*
     * 1.   Recieve initial work
     * 2.   Render tile
     * 3.   Send rendered data to master as (array, x, y, time) floats
     * 4.   Recieve more work as (x y) ints
     * 5.   If -1 -1 not recieved, continue from 2.
     */

    while(true) {
        // Recieve work
        MPI_Recv(workPacket, 2, MPI_INT, 0, 0, MPI_COMM_WORLD, &status);

        // Are we done
        if(workPacket[0] == -1) {
            break;
        }

        comp_start = MPI_Wtime();

        // Not done. Render a tile
        region.xInImage = workPacket[0];
        region.yInImage = workPacket[1];

        // Don't render out of bounds
        if(region.xInImage + data->dynamicBlockWidth >= data->width) {
            region.width = data->width - region.xInImage;
        } else {
            region.width = data->dynamicBlockWidth;
        }

        if(region.yInImage + data->dynamicBlockHeight >= data->height) {
            region.height = data->height - region.yInImage;
        } else {
            region.height = data->dynamicBlockHeight;
        }

        // Render
        renderRegion(data, &region);

        // Report results
        comp_stop = MPI_Wtime();
        comp_time = comp_stop - comp_start;

        region.pixels[pixelsSize - 3] = (float) region.xInImage;
        region.pixels[pixelsSize - 2] = (float) region.yInImage;
        region.pixels[pixelsSize - 1] = (float) comp_time;
        MPI_Send(region.pixels, pixelsSize, MPI_FLOAT, 0, 0, MPI_COMM_WORLD);
    }

    // clean up
    delete[] region.pixels;
    delete[] workPacket;
}
