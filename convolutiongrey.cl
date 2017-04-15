#pragma OPENCL EXTENSION cl_amd_printf : enable

__kernel void convolution (__global float *inputImage,
                           __global float *outputImage,
                           __constant float *filter,
                           __local float *cache)
{
    int modx, mody, centre;

    int ix = get_global_id(0);
    int iy = get_global_id(1);
    int iid = ix*WIDTH + iy;

    int lx = get_local_id(0);
    int ly = get_local_id(1);
    int lh = get_local_size(0) + DOUBLE_BUFFER_SIZE;
    int lw = get_local_size(1) + DOUBLE_BUFFER_SIZE;
    int lid = (lx+BUFFER_SIZE) * lw + ly + BUFFER_SIZE;

    //cache this thread's pixel
    cache[lid] = inputImage[iid];

    //if in buffer space
    if (ix < BUFFER_SIZE || iy < BUFFER_SIZE ||
        ix > HEIGHT-BUFFER_SIZE || iy >= WIDTH-BUFFER_SIZE)
    {
        //the only purpose was to cache, so just synchronise and return
        barrier(CLK_LOCAL_MEM_FENCE);
        return;
    }

    int xOffset = 0;
    int yOffset = 0;
    //top line
    if (lx < BUFFER_SIZE)
    {
        xOffset = -BUFFER_SIZE;
        cache[lx*lw + ly + BUFFER_SIZE] = inputImage[(ix-BUFFER_SIZE)*WIDTH + iy];
    }
    //bottom line
    else if (lx >= get_local_size(0) - BUFFER_SIZE)
    {
        xOffset = BUFFER_SIZE;
        cache[(lx+DOUBLE_BUFFER_SIZE)*lw +ly+BUFFER_SIZE] =
            inputImage[(ix+BUFFER_SIZE)*WIDTH+iy];
    }

    //far left
    if (ly < BUFFER_SIZE)
    {
        yOffset = -BUFFER_SIZE;
        cache[lid-BUFFER_SIZE] = inputImage[iid-BUFFER_SIZE];
    }
    //far right
    else if (ly >= get_local_size(1) - BUFFER_SIZE)
    {
        yOffset = BUFFER_SIZE;
        cache[lid+BUFFER_SIZE] = inputImage[iid+BUFFER_SIZE];
    }

    //corner
    if (xOffset != 0 && yOffset != 0)
    {
        cache[(lx+BUFFER_SIZE+xOffset) * lw + ly+BUFFER_SIZE+yOffset] = inputImage[(ix+xOffset)*WIDTH + iy + yOffset];
    }

    //wait until all threads have pulled their data
    barrier(CLK_LOCAL_MEM_FENCE);

    float sum = 0;
    int rowPlusLid;
    int fIndex = 0;

    for (int fx = -BUFFER_SIZE; fx <= BUFFER_SIZE; fx++)
    {
        rowPlusLid = fx * lw + lid;
        for (int fy = -BUFFER_SIZE; fy <= BUFFER_SIZE; fy++, fIndex++)
        {
            //printf("main %d,%d - %d\n", get_group_id(0), get_group_id(1), lid + row + fy);
            sum += cache[rowPlusLid + fy] * filter[fIndex];
        }
    }

    float val = sum * FACTOR + BIAS;

    outputImage[(ix-BUFFER_SIZE)*(WIDTH-DOUBLE_BUFFER_SIZE)+(iy-BUFFER_SIZE)] = val < 0 ? 0 : val > 255 ? 255 : val;
}
