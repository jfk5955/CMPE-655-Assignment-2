//This file contains the code that the master process will execute.

#include <iostream>
#include <mpi.h>
#include <cstring>
#include <math.h>

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
        
        case PART_MODE_DYNAMIC:
            startTime = MPI_Wtime();
            masterDynamicCentralizedQueue(data, pixels);
            stopTime = MPI_Wtime();
            break;
            
        case PART_MODE_STATIC_BLOCKS:
            startTime = MPI_Wtime();
            masterStaticSquareBlocks(data, pixels);
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

    int subregionWidth = data->width / ((int)sqrt(data->mpi_procs));
    int subregionWidthRemainder = data->width % ((int)sqrt(data->mpi_procs));
    
    int subregionHeight = data->height / ((int)sqrt(data->mpi_procs));
    int subregionHeightRemainder = data->height % ((int)sqrt(data->mpi_procs));

    // Compute our portion of the region
    // Describe our region
    RenderRegion region;
    region.xInImage = 0;
    region.yInImage = 0;
    region.xInPixels = 0;
    region.yInPixels = 0;
    region.width = subregionWidth;
    region.height = subregionHeight;
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
    // Buffer with enough space for the largest subregion and the time report
    float* recieveBuffer = new float[(3 * (subregionWidth + subregionWidthRemainder) * (subregionHeight + subregionHeightRemainder)) + 1];
    for(int i = 1; i < data->mpi_procs; i++) {
        // Recieve data from slave
        int recieveWidth = subregionWidth;
        int recieveHeight = subregionHeight;

        if(i == data->mpi_procs - 1) {
            // Last slave has remainder as well
            recieveWidth += subregionWidthRemainder;
            recieveHeight += subregionHeightRemainder;
        }

        int recieveSize = (3 * recieveWidth * recieveHeight) + 1;

        MPI_Recv(recieveBuffer, recieveSize, MPI_FLOAT, i, 0, MPI_COMM_WORLD, &status);
        
        // Include slave computation time
        computationTime += (double) recieveBuffer[recieveSize - 1];

        // Determine location in output
        int slaveXInImage = subregionWidth * (i % (int)sqrt(data->mpi_procs));
        int slaveYInImage = subregionHeight * (i / (int)sqrt(data->mpi_procs));

        // Move into output buffer
        for(int y = 0; y < recieveHeight; y++) {
            int pixelsRowOffset = 3 * (slaveXInImage + ((y + slaveYInImage) * data->width));
            int recievedRowOffset = 3 * y * recieveWidth;
            memcpy(&(pixels[pixelsRowOffset]), &(recieveBuffer[recievedRowOffset]), 3 * recieveWidth * sizeof(float));
        }
    }

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

    // Recieve each set of regions
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
    MPI_Status status;
    double computationTime = 0.0;
    double communicationStart = MPI_Wtime();

    /*
     * 1.   Distribute initial work
     * 2.   Recv from MPI_ANY_SOURCE a results packet, consisting (width * height) + 3 floats:
     *          data_array, x, y, time
     * 3.   Send to the rank we got the packet from a work packet, consisting of 2 ints:
     *          x, y
     *      If no work is remaining, -1 -1 is sent and the rank is marked as done
     * 4.   Copy recieved packet data into image
     * 5.   If any ranks are not done, continue from 2.
     */

    int resultsSize = (3 * data->dynamicBlockWidth * data->dynamicBlockHeight) + 3;
    float* resultsPacket = new float[resultsSize];

    int* workPacket = new int[2];
    workPacket[0] = 0;  // x
    workPacket[1] = 0;  // y

    // Distribute initial work
    for(int i = 1; i < data->mpi_procs; i++) {
        MPI_Send(workPacket, 2, MPI_INT, i, 0, MPI_COMM_WORLD);
        incrementWorkPacket(data, workPacket);
    }

    // Work-sending loop
    while(workPacket[0] != -1) {
        // Recieve results packet
        MPI_Recv(resultsPacket, resultsSize, MPI_FLOAT, MPI_ANY_SOURCE, 0, MPI_COMM_WORLD, &status);

        // Send new work
        MPI_Send(workPacket, 2, MPI_INT, status.MPI_SOURCE, 0, MPI_COMM_WORLD);

        // Copy into image
        int imageX = (int) resultsPacket[resultsSize - 3];
        int imageY = (int) resultsPacket[resultsSize - 2];
        computationTime += (double) resultsPacket[resultsSize - 1];

        int copyWidth = data->dynamicBlockWidth;
        if(imageX + copyWidth >= data->width) {
            copyWidth = data->width - imageX;
        }

        // Copy each row
        for(int resultsY = 0; resultsY < data->dynamicBlockHeight && imageY < data->height; resultsY++, imageY++) {
            int resultsOffset = 3 * resultsY * data->dynamicBlockWidth;
            int pixelsOffset = 3 * ((imageY * data->width) + imageX);

            memcpy(&(pixels[pixelsOffset]), &(resultsPacket[resultsOffset]), 3 * copyWidth * sizeof(float));
        }

        incrementWorkPacket(data, workPacket);
    }

    // Results-waiting loop
    for(int i = 1; i < data->mpi_procs; i++) {
        // Recieve results packet
        MPI_Recv(resultsPacket, resultsSize, MPI_FLOAT, MPI_ANY_SOURCE, 0, MPI_COMM_WORLD, &status);

        // Send termination packet
        MPI_Send(workPacket, 2, MPI_INT, status.MPI_SOURCE, 0, MPI_COMM_WORLD);

        // Copy into image
        int imageX = (int) resultsPacket[resultsSize - 3];
        int imageY = (int) resultsPacket[resultsSize - 2];
        computationTime += (double) resultsPacket[resultsSize - 1];

        int copyWidth = data->dynamicBlockWidth;
        if(imageX + copyWidth >= data->width) {
            copyWidth = data->width - imageX;
        }

        // Copy each row
        for(int resultsY = 0; resultsY < data->dynamicBlockHeight && imageY < data->height; resultsY++, imageY++) {
            int resultsOffset = 3 * resultsY * data->dynamicBlockWidth;
            int pixelsOffset = 3 * ((imageY * data->width) + imageX);

            memcpy(&(pixels[pixelsOffset]), &(resultsPacket[resultsOffset]), 3 * copyWidth * sizeof(float));
        }

        incrementWorkPacket(data, workPacket);
    }

    // Clean up
    delete[] workPacket;
    delete[] resultsPacket;

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

void incrementWorkPacket(ConfigData* data, int* workPacket) {
    if(workPacket[0] == -1) {
        return;
    }

    // Increment in x
    workPacket[0] += data->dynamicBlockWidth;
    if(workPacket[0] >= data->width) {
        // Packet ends up starting out of bounds. Increment in y
        workPacket[0] = 0;
        workPacket[1] += data->dynamicBlockHeight;

        // Packet ends up starting out of bounds. We're done.
        if(workPacket[1] >= data->height) {
            workPacket[0] = -1;
            workPacket[1] = -1;
        }
    }
}
