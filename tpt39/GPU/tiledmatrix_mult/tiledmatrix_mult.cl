__kernel void tiledmatrix_mult(__global const float *X,
                               __global const float *Y,
                               __global float *restrict Z,
                               const int M,
                               const int N,
                               const int K)
                        
{
    //try get global size
    size_t i = get_global_id(0);
    size_t j = get_global_id(1);
    float acc = 0.0f;
    for (uint k = 0; k < K ; k++) {
        acc += X[i*M+k]*Y[k*K+j] ;
    }
    Z[i*M+j]=acc;
}
