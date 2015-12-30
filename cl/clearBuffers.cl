/**
 * Input: Buffers which can be effectively cleaned!
 * Side effect: Clears buffers
 */
kernel void clearBuffers
(
    //global float* fov; //does not need to be cleaned, it will be overwritten on next call
    //global char* objects, //does not have to be cleaned either, it's sufficient to clean the offsets
    global int* frameBuf, // TODO ^
    global float* depthBuf
)
{
    const int i = get_global_id(0);
    //frameBuf[i] = 0 << 24 | 0 << 16 | 0 << 8 | 255;
    depthBuf[i] = 1.0f/0.0f;
}
