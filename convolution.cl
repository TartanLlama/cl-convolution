#pragma OPENCL EXTENSION cl_amd_printf : enable

__kernel void convolution (__global float *inputImage,
                           __global float *outputImage,
                           float factor, float bias,
                           __constant float *filter, int filterSize,
                           __local float *cache)
{
    int modx, mody, centre;

    int height = get_global_size(0);
    int width = get_global_size(1);

    int ix = get_global_id(0);
    int iy = get_global_id(1);
    int iid = ix*width + iy;

    int bufferSize = filterSize/2;

    int lx = get_local_id(0);
    int ly = get_local_id(1);
    int lh = get_local_size(0) + bufferSize*2;
    int lw = get_local_size(1) + bufferSize*2;
    int lid = (lx+bufferSize) * lw + ly + bufferSize;

    //cache this thread's pixel
    cache[lid] = inputImage[iid];

    //if in buffer space
    if (ix < bufferSize || iy < bufferSize ||
        ix > height-bufferSize || iy >= width-bufferSize)
    {
        //the only purpose was to cache, so just synchronise and return
        barrier(CLK_LOCAL_MEM_FENCE);
        return;
    }

    int xOffset = 0;
    int yOffset = 0;
    //top line
    if (lx < bufferSize)
    {
        xOffset = -bufferSize;
        cache[lx*lw + ly + bufferSize] = inputImage[(ix-bufferSize)*width + iy];
    }
    //bottom line
    else if (lx >= get_local_size(0) - bufferSize)
    {
        xOffset = bufferSize;
        cache[(lx+bufferSize*2)*lw +ly+bufferSize] = inputImage[(ix+bufferSize)*width+iy];
    }

    //far left
    if (ly < bufferSize)
    {
        yOffset = -bufferSize;
        cache[lid-bufferSize] = inputImage[iid-bufferSize];
    }
    //far right
    else if (ly >= get_local_size(1) - bufferSize)
    {
        yOffset = bufferSize;
        cache[lid+bufferSize] = inputImage[iid+bufferSize];
    }

    //corner
    if (xOffset != 0 && yOffset != 0)
    {
        cache[(lx+bufferSize+xOffset) * lw + ly+bufferSize+yOffset] = inputImage[(ix+xOffset)*width + iy + yOffset];
    }

    //wait until all threads have pulled their data
    barrier(CLK_LOCAL_MEM_FENCE);

    float sum = 0;
    int row;
    int fIndex = 0;

    for (int fx = -bufferSize; fx <= bufferSize; fx++)
    {
        row = fx * lw;
        for (int fy = -bufferSize; fy <= bufferSize; fy++, fIndex++)
        {
            //printf("main %d,%d - %d\n", get_group_id(0), get_group_id(1), lid + row + fy);
            sum += cache[lid + row + fy] * filter[fIndex];
        }
    }

    float val = sum*factor + bias;

    outputImage[(ix-bufferSize)*(width-bufferSize*2)+(iy-bufferSize)] = val < 0 ? 0 : val > 255 ? 255 : val;
}
