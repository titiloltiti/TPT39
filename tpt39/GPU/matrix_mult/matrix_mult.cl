__kernel void matrix_mult(__global const float *x, 
                        __global const float *y, 
                        __global float *restrict z,
                        const int M,
                        const int K)
{
    size_t i = get_global_id(0);
    size_t j = get_global_id(1);
    float acc = 0.0f;
    for (uint k = 0; k < K ; k++) {
        acc += x[i*M+k]*y[k*K+j] ;
    }
    z[i*M+j]=acc;
}

