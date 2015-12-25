/**
 * Input: Buffers which can be effectively cleaned!
 * Side effect: Clears buffers
 */
__kernel void clearBuffers
(
    //__global float* fov; //does not need to be cleaned, it will be overwritten on next call
    //__global char* objects, //does not have to be cleaned either, it's sufficient to clean the offsets
    __global int* offsets,
    __global char* frameBuf,
    __global float* depthBuf
)
{
    //this shader has to be called size_x*size_y times.
    //the buffer size of offsets has to be smaller than size_x*size_y
    //default (hardcoded) : 1000
    //if if >= 1000, ignore offsets
    const int i = get_global_id(0);

    if(i < 1000)
        offsets[i] = -1;
    
    frameBuf[i] = 100;
    depthBuf[i] = 1.0f/0.0f;
}

