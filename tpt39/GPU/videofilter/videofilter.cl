// OpenCL kernel for the vector addition
__kernel void videofilter(__global const char *x, 
                        __global char * z)
{
    //For now we'll' do everything there, with no working groups, and recreate local variables here, maybe using the global memory would be faster and there is no padding 
    size_t j = get_global_id(0);
    size_t i = get_global_id(1);
    size_t WIDTH = get_global_size(0);
    size_t HEIGHT = get_global_size(1);
    float tmp=0.0f;
    float Xtmp=0.0f;
    float Ytmp=0.0f;
    float Blurrtmp1=0.0f;
    float Blurrtmp2=0.0f;
    float Blurrtmp3=0.0f;

    float gaussianKernel[9] = {0.0625,
  0.125,
  0.0625,
  0.125,
  0.25,
  0.125,
  0.0625,
  0.125,
  0.0625 };

    float XscharrKernel[9] = { 3./16. ,  10./16.,   3./16.,
 0.,   0. ,  0.,
-3./16.,  -10./16.,  -3./16. } ;
  
   float YscharrKernel[9] = { 3./16. ,  0.,   -3./16.,
10./16. ,  0,  -10./16.,
 3./16. ,  0 ,  -3./16.} ;

//Filter?
    WIDTH+=2;

    Blurrtmp1 = gaussianKernel[0]*(float)x[(i+0)*WIDTH+(j+0)]+gaussianKernel[1]*(float)x[(i+0)*WIDTH+(j+1)]+gaussianKernel[2]*(float)x[(i+0)*WIDTH+(j+2)]
       +gaussianKernel[3]*(float)x[(i+1)*WIDTH+(j+0)]+gaussianKernel[4]*(float)x[(i+1)*WIDTH+(j+1)]+gaussianKernel[5]*(float)x[(i+1)*WIDTH+(j+2)]
       +gaussianKernel[6]*(float)x[(i+2)*WIDTH+(j+0)]+gaussianKernel[7]*(float)x[(i+2)*WIDTH+(j+1)]+gaussianKernel[8]*(float)x[(i+2)*WIDTH+(j+2)];

//      Blurrtmp2 = gaussianKernel[0]*(float)x[(i+0)*WIDTH+(j+0)]+gaussianKernel[1]*(float)x[(i+0)*WIDTH+(j+1)]+gaussianKernel[2]*(float)x[(i+0)*WIDTH+(j+2)]
//        +gaussianKernel[3]*(float)x[(i+1)*WIDTH+(j+0)]+gaussianKernel[4]*(float)x[(i+1)*WIDTH+(j+1)]+gaussianKernel[5]*(float)x[(i+1)*WIDTH+(j+2)]
//        +gaussianKernel[6]*(float)x[(i+2)*WIDTH+(j+0)]+gaussianKernel[7]*(float)x[(i+2)*WIDTH+(j+1)]+gaussianKernel[8]*(float)x[(i+2)*WIDTH+(j+2)];

//  Blurrtmp3 = gaussianKernel[0]*(float)x[(i+0)*WIDTH+(j+0)]+gaussianKernel[1]*(float)x[(i+0)*WIDTH+(j+1)]+gaussianKernel[2]*(float)x[(i+0)*WIDTH+(j+2)]
//        +gaussianKernel[3]*(float)x[(i+1)*WIDTH+(j+0)]+gaussianKernel[4]*(float)x[(i+1)*WIDTH+(j+1)]+gaussianKernel[5]*(float)x[(i+1)*WIDTH+(j+2)]
//        +gaussianKernel[6]*(float)x[(i+2)*WIDTH+(j+0)]+gaussianKernel[7]*(float)x[(i+2)*WIDTH+(j+1)]+gaussianKernel[8]*(float)x[(i+2)*WIDTH+(j+2)];

    Xtmp = XscharrKernel[0]*(float)x[(i+0)*WIDTH+(j+0)]+XscharrKernel[1]*(float)x[(i+0)*WIDTH+(j+1)]+XscharrKernel[2]*(float)x[(i+0)*WIDTH+(j+2)]
       +XscharrKernel[3]*(float)x[(i+1)*WIDTH+(j+0)]+XscharrKernel[4]*(float)x[(i+1)*WIDTH+(j+1)]+XscharrKernel[5]*(float)x[(i+1)*WIDTH+(j+2)]
       +XscharrKernel[6]*(float)x[(i+2)*WIDTH+(j+0)]+XscharrKernel[7]*(float)x[(i+2)*WIDTH+(j+1)]+XscharrKernel[8]*(float)x[(i+2)*WIDTH+(j+2)];

    Ytmp = YscharrKernel[0]*(float)x[(i+0)*WIDTH+(j+0)]+YscharrKernel[1]*(float)x[(i+0)*WIDTH+(j+1)]+YscharrKernel[2]*(float)x[(i+0)*WIDTH+(j+2)]
       +YscharrKernel[3]*(float)x[(i+1)*WIDTH+(j+0)]+YscharrKernel[4]*(float)x[(i+1)*WIDTH+(j+1)]+YscharrKernel[5]*(float)x[(i+1)*WIDTH+(j+2)]
       +YscharrKernel[6]*(float)x[(i+2)*WIDTH+(j+0)]+YscharrKernel[7]*(float)x[(i+2)*WIDTH+(j+1)]+YscharrKernel[8]*(float)x[(i+2)*WIDTH+(j+2)];
    WIDTH-=2;

    tmp=0.25*Blurrtmp1+0.75*(0.5*Xtmp+0.5*Ytmp); //You can chage coefficients here to change the influence of each part
z[i*WIDTH+j]=(char)tmp;


//  z[i*WIDTH+j]=x[(i+1)*(WIDTH+2)+(j+1)]; //THIS WORKS



}
