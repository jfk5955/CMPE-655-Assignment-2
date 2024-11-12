//This file contains the code that the master process will execute.

#include <iostream>
#include <mpi.h>

#include "RayTrace.h"
#include "master.h"

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

void masterStaticSquareBlocks(ConfigData* data, float* pixels) {
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
    //region.pixelsWidth = TODO
    //region.pixelsHeight = TODO

    region.pixels = pixels;

    // Render our region
    // TODO - some kind of loop which does each of our sub-regions

    // Stop computation timer
    double computationStop = MPI_Wtime();
    double computationTime = computationStop - computationStart;

    // Start communication timer
    double communicationStart = MPI_Wtime();
    
    // Recieve subregions
    // TODO - will be a bit more complicated as slaves pack their subregions
    // Might change to have slaves send each of their subregions separately,
    // but that makes timing things more annoying

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
