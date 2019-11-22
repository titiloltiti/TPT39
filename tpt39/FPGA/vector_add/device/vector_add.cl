 // ACL kernel for adding two input vectors
__attribute__((num_compute_units(32)))
__attribute__((num_simd_work_items(4)))
__attribute__((reqd_work_group_size(16,16,1)))
__kernel void vector_add(__global const float *restrict x, 
                        __global const float *restrict y, 
                        __global float *restrict z)
{
    // get index of the work item
    size_t i = get_global_id(0);
    size_t j = get_global_id(1);

    float acc = 0.0f;
    for (uint k = 0; k < M ; k++) {
        acc += x[i*64+k]*y[k*64+j] ;
    }
    z[i*64+j]=acc; 
}

//64*64 = 4069