#ifndef __PROCESS_COMMON_H__
#define __PROCESS_COMMON_H__

#include "RayTrace.h"

// Describes a region of the image
typedef struct {
    // x, y of lower left corner in image
    int xInImage;
    int yInImage;

    // x, y of lower left corner in pixels
    int xInPixels;
    int yInPixels;

    // Size of the pixels array
    int pixelsWidth;
    int pixelsHeight;

    // Size of the region to render
    int width;
    int height;

    // Array to place data in
    float* pixels;
} RenderRegion;

/*
 * Generic function which renders a region of the image
 * @param data Supplies scene information
 * @param region Supplies region information
 */
void renderRegion(ConfigData* data, RenderRegion* region);

#endif
