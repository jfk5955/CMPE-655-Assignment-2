// Code common to both master and slave processes

#include "RayTrace.h"
#include "common.h"

void renderRegion(ConfigData* data, RenderRegion* region) {
    // Render the given part of the scene
    // Loop over local coordinates
    for(int ry = 0; ry < region->height; ry++) {
        for(int rx = 0; rx < region->width; rx++) {
            // Get image and pixel coordinates
            int ix = region->xInImage + rx;
            int iy = region->yInImage + ry;
            int px = region->xInPixels + rx;
            int py = region->YInPixels + ry;

            // Get index in pixels
            int baseIndex = 3 * ((py * region->pixelsWidth) + px);

            // Shade
            shadePixel(&(region->pixels[baseIndex]), iy, ix, data);
        }
    }
}
