// OpenCL kernel for the vector addition
__kernel void vector_add(__global const float *x, 
                        __global const float *y, 
                        __global float *restrict z)
{
    size_t i = get_global_id(0);
    z[i]=x[i]+y[i];
}

