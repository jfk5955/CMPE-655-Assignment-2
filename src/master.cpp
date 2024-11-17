//This file contains the code that the master process will execute.

#include <iostream>
#include <mpi.h>
#include <cstring>

#include "RayTrace.h"
#include "master.h"
#include "common.h"

void masterMain(ConfigData* data)
{
    //Depending on the partitioning scheme, different things will happen.
    //You should have a different function for each of the required 
    //schemes that returns some values that you need to handle.
    
    //Allocate space for the image on the master.
    float* pixels = new float[3 * data->width * data->height];
    
    //Execution time will be defined as how long it takes
    //for the given function to execute based on partitioning
    //type.
    double renderTime = 0.0, startTime, stopTime;

	//Add the required partitioning methods here in the case statement.
	//You do not need to handle all cases; the default will catch any
	//statements that are not specified. This switch/case statement is the
	//only place that you should be adding code in this function. Make sure
	//that you update the header files with the new functions that will be
	//called.
	//It is suggested that you use the same parameters to your functions as shown
	//in the sequential example below.
    switch (data->partitioningMode)
    {
        case PART_MODE_NONE:
            //Call the function that will handle this.
            startTime = MPI_Wtime();
            masterSequential(data, pixels);
            stopTime = MPI_Wtime();
            break;
        
        case PART_MODE_STATIC_STRIPS_VERTICAL:
            startTime = MPI_Wtime();
            masterStaticContinuousColumns(data, pixels);
            stopTime = MPI_Wtime();
            break;
        
        case PART_MODE_STATIC_CYCLES_HORIZONTAL:
            startTime = MPI_Wtime();
            masterStaticCyclicalRows(data, pixels);
            stopTime = MPI_Wtime();
            break;

        default:
            std::cout << "This mode (" << data->partitioningMode;
            std::cout << ") is not currently implemented." << std::endl;
            break;
    }

    renderTime = stopTime - startTime;
    std::cout << "Execution Time: " << renderTime << " seconds" << std::endl << std::endl;

    //After this gets done, save the image.
    std::cout << "Image will be save to: ";
    std::string file = "renders/" + generateFileName();
    std::cout << file << std::endl;
    savePixels(file, pixels, data);

    //Delete the pixel data.
    delete[] pixels; 
}

void masterSequential(ConfigData* data, float* pixels)
{
    //Start the computation time timer.
    double computationStart = MPI_Wtime();

    //Render the scene.
    for( int i = 0; i < data->height; ++i )
    {
        for( int j = 0; j < data->width; ++j )
        {
            int row = i;
            int column = j;

            //Calculate the index into the array.
            int baseIndex = 3 * ( row * data->width + column );

            //Call the function to shade the pixel.
            shadePixel(&(pixels[baseIndex]),row,j,data);
        }
    }

    //Stop the comp. timer
    double computationStop = MPI_Wtime();
    double computationTime = computationStop - computationStart;

    //After receiving from all processes, the communication time will
    //be obtained.
    double communicationTime = 0.0;

    //Print the times and the c-to-c ratio
	//This section of printing, IN THIS ORDER, needs to be included in all of the
	//functions that you write at the end of the function.
    std::cout << "Total Computation Time: " << computationTime << " seconds" << std::endl;
    std::cout << "Total Communication Time: " << communicationTime << " seconds" << std::endl;
    double c2cRatio = communicationTime / computationTime;
    std::cout << "C-to-C Ratio: " << c2cRatio << std::endl;
}

void masterStaticContinuousColumns(ConfigData* data, float* pixels) {
    MPI_Status status;

    //Start computation timer.
    double computationStart = MPI_Wtime();

    int subregionWidth = data->width / data->mpi_procs;
    int subregionRemainder = data->width % data->mpi_procs;

    // Compute our portion of the region
    // Describe our region
    RenderRegion region;
    region.xInImage = 0;
    region.yInImage = 0;
    region.xInPixels = 0;
    region.yInPixels = 0;
    region.width = subregionWidth;
    region.height = data->height;
    region.pixelsWidth = data->width;
    region.pixelsHeight = data->height;
    region.pixels = pixels;

    // Render our subregion
    renderRegion(data, &region);

    // Stop computation timer
    double computationStop = MPI_Wtime();
    double computationTime = computationStop - computationStart;

    // Start communication timer
    double communicationStart = MPI_Wtime();
    
    // Recieve subregions
    // Buffer with enough space for the largest subregion and the time report
    float* recieveBuffer = new float[(3 * (subregionWidth + subregionRemainder) * data->height) + 1];
    for(int i = 1; i < data->mpi_procs; i++) {
        // Recieve data from slave
        int recieveWidth = subregionWidth;

        if(i == data->mpi_procs - 1) {
            // Last slave has remainder as well
            recieveWidth += subregionRemainder;
        }

        int recieveSize = (3 * recieveWidth * data->height) + 1;

        MPI_Recv(recieveBuffer, recieveSize, MPI_FLOAT, i, 0, MPI_COMM_WORLD, &status);
        
        // Include slave computation time
        computationTime += (double) recieveBuffer[recieveSize - 1];

        // Determine location in output
        int slaveXInImage = subregionWidth * i;

        // Move into output buffer
        // Move row-by-row as recieved region is much thinner than the image
        for(int y = 0; y < data->height; y++) {
            int pixelsRowOffset = 3 * (slaveXInImage + (y * data->width));
            int recievedRowOffset = 3 * y * recieveWidth;
            memcpy(&(pixels[pixelsRowOffset]), &(recieveBuffer[recievedRowOffset]), 3 * recieveWidth * sizeof(float));
        }
    }
    
    delete[] recieveBuffer;

    // Stop communication timer
    double communicationStop = MPI_Wtime();
    double communicationTime = communicationStop - communicationStart;

    // Print times & c-to-c ratio
    // Copied from given sequential code
    std::cout << "Total Computation Time: " << computationTime << " seconds" << std::endl;
    std::cout << "Total Communication Time: " << communicationTime << " seconds" << std::endl;
    double c2cRatio = communicationTime / computationTime;
    std::cout << "C-to-C Ratio: " << c2cRatio << std::endl;
}

void masterStaticSquareBlocks(ConfigData* data, float* pixels) {
    MPI_Status status;

    //Start computation timer.
    double computationStart = MPI_Wtime();

    // Compute our portion of the region
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

    region.pixels = pixels;

    // Render our region
    renderRegion(data, &region);

    // Stop computation timer
    double computationStop = MPI_Wtime();
    double computationTime = computationStop - computationStart;

    // Start communication timer
    double communicationStart = MPI_Wtime();
    
    // Recieve subregions
    // TODO

    // Stop communication timer
    double communicationStop = MPI_Wtime();
    double communicationTime = communicationStop - communicationStart;

    // Print times & c-to-c ratio
    // Copied from given sequential code
    std::cout << "Total Computation Time: " << computationTime << " seconds" << std::endl;
    std::cout << "Total Communication Time: " << communicationTime << " seconds" << std::endl;
    double c2cRatio = communicationTime / computationTime;
    std::cout << "C-to-C Ratio: " << c2cRatio << std::endl;
}

void masterStaticCyclicalRows(ConfigData* data, float* pixels) {
    MPI_Status status;

    //Start computation timer.
    double computationStart = MPI_Wtime();

    // Compute our portion of the region
    // Describe our region
    RenderRegion region;
    region.xInImage = 0;
    region.yInImage = 0;
    region.xInPixels = 0;
    region.yInPixels = 0;
    region.width = data->width;
    region.height = data->cycleSize;
    region.pixelsWidth = data->width;
    region.pixelsHeight = data->height;
    region.pixels = pixels;

    // Render our region
    while(region.yInImage < data->height) {
        // Make sure we don't overrun
        if(region.yInImage + data->cycleSize >= data->height) {
            region.height = data->height - region.yInImage;
        }

        // Render subregion
        renderRegion(data, &region);

        region.yInImage += data->cycleSize * data->mpi_procs;
        region.yInPixels += data->cycleSize * data->mpi_procs;
    }

    // Stop computation timer
    double computationStop = MPI_Wtime();
    double computationTime = computationStop - computationStart;

    // Start communication timer
    double communicationStart = MPI_Wtime();
    
    // Recieve subregions
    int totalSubregions = (data->height / data->cycleSize) + ((data->height % data->cycleSize != 0) ? 1 : 0);
    int maxSubregions = (totalSubregions / data->mpi_procs) + 1;
    int slaveRegionHeight = maxSubregions * data->cycleSize;
    int slaveRegionSize = (3 * data->width * slaveRegionHeight) + 1;

    float** slaveRegions = new float*[data->mpi_procs - 1];

    // Recieve reach set of regions
    for(int i = 1; i < data->mpi_procs; i++) {
        slaveRegions[i - 1] = new float[slaveRegionSize];
        MPI_Recv(slaveRegions[i - 1], slaveRegionSize, MPI_FLOAT, i, 0, MPI_COMM_WORLD, &status);

        // Include slave computation time
        computationTime += (double) slaveRegions[i - 1][slaveRegionSize - 1];
    }

    // Move regions into image
    int subregionNumber = 1;
    int imageStartY = data->cycleSize;
    int slaveStartY = 0;

    while(imageStartY < data->height) {
        // Where are we getting data from
        int rank = subregionNumber % data->mpi_procs;

        if(rank == 0) {
            // Us. No copy
            slaveStartY += data->cycleSize;
            imageStartY += data->cycleSize;
            subregionNumber++;
            continue;
        }

        // Slave. Copy
        int imageIndex = 3 * data->width * imageStartY;
        int slaveIndex = 3 * data->width * slaveStartY;
        int height = data->cycleSize;

        // Don't overrun
        if(imageStartY + height >= data->height) {
            height = data->height - imageStartY;
        }

        int copySize = 3 * data->width * height * sizeof(float);

        float* slaveRegion = slaveRegions[rank - 1];
        memcpy(&(pixels[imageIndex]), &(slaveRegion[slaveIndex]), copySize);

        imageStartY += data->cycleSize;
        subregionNumber++;
    }

    // Clean up
    for(int i = 0; i < data->mpi_procs - 1; i++) {
        delete[] slaveRegions[i];
    }

    delete[] slaveRegions;

    // Stop communication timer
    double communicationStop = MPI_Wtime();
    double communicationTime = communicationStop - communicationStart;

    // Print times & c-to-c ratio
    // Copied from given sequential code
    std::cout << "Total Computation Time: " << computationTime << " seconds" << std::endl;
    std::cout << "Total Communication Time: " << communicationTime << " seconds" << std::endl;
    double c2cRatio = communicationTime / computationTime;
    std::cout << "C-to-C Ratio: " << c2cRatio << std::endl;
}

void masterDynamicCentralizedQueue(ConfigData* data, float* pixels) {
    // TODO
}
