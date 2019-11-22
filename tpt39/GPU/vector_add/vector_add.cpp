#include <stdio.h>
#include <stdlib.h>
#include <iostream> // for standard I/O
#include <math.h>
#include <time.h>
#include <CL/cl.h>
#include <CL/cl_ext.h>
#define STRING_BUFFER_LEN 1024
using namespace std;

void print_clbuild_errors(cl_program program, cl_device_id device)
{
  cout << "Program Build failed\n";
  size_t length;
  char buffer[2048];
  clGetProgramBuildInfo(program, device, CL_PROGRAM_BUILD_LOG, sizeof(buffer), buffer, &length);
  cout << "--- Build log ---\n " << buffer << endl;
  exit(1);
}

unsigned char **read_file(const char *name)
{
  size_t size;
  unsigned char **output = (unsigned char **)malloc(sizeof(unsigned char *));
  FILE *fp = fopen(name, "rb");
  if (!fp)
  {
    printf("no such file:%s", name);
    exit(-1);
  }

  fseek(fp, 0, SEEK_END);
  size = ftell(fp);
  fseek(fp, 0, SEEK_SET);

  *output = (unsigned char *)malloc(size);
  unsigned char **outputstr = (unsigned char **)malloc(sizeof(unsigned char *));
  *outputstr = (unsigned char *)malloc(size);
  if (!*output)
  {
    fclose(fp);
    printf("mem allocate failure:%s", name);
    exit(-1);
  }

  if (!fread(*output, size, 1, fp))
    printf("failed to read file\n");
  fclose(fp);
  printf("file size %d\n", size);
  printf("-------------------------------------------\n");
  snprintf((char *)*outputstr, size, "%s\n", *output);
  printf("%s\n", *outputstr);
  printf("-------------------------------------------\n");
  return outputstr;
}
void callback(const char *buffer, size_t length, size_t final, void *user_data)
{
  fwrite(buffer, 1, length, stdout);
}

//This function will check for any over/underflow in time and performance measurement 
enum { NS_PER_SECOND = 1000000000 };
void sub_timespec(struct timespec t1, struct timespec t2, struct timespec *td)
{
    td->tv_nsec = t2.tv_nsec - t1.tv_nsec;
    td->tv_sec  = t2.tv_sec - t1.tv_sec;
    if (td->tv_sec > 0 && td->tv_nsec < 0)
    {
        td->tv_nsec += NS_PER_SECOND;
        td->tv_sec--;
    }
    else if (td->tv_sec < 0 && td->tv_nsec > 0)
    {
        td->tv_nsec -= NS_PER_SECOND;
        td->tv_sec++;
    }
}

void checkError(int status, const char *msg)
{
  if (status != CL_SUCCESS)
    printf("%s\n", msg);
}

// Randomly generate a floating-point number between -10 and 10.
float rand_float()
{
  return float(rand()) / float(RAND_MAX) * 20.0f - 10.0f;
}

int main()
{
// Setup the openCL variables
  char char_buffer[STRING_BUFFER_LEN];
  cl_platform_id platform;
  cl_device_id device;
  cl_context context;
  cl_context_properties context_properties[] =
      {
          CL_CONTEXT_PLATFORM, 0,
          CL_PRINTF_CALLBACK_ARM, (cl_context_properties)callback,
          CL_PRINTF_BUFFERSIZE_ARM, 0x1000,
          0};
  cl_command_queue queue;
  cl_program program;
  cl_kernel kernel;

//--------------------------------------------------------------------
// Setup Parameters 
  const unsigned N = 50000000; // Change N to see performances
  float *input_a;
  float *input_b;
  float *output;
  float *ref_output = (float *)malloc(sizeof(float) * N);
  cl_mem input_a_buf; // num_devices elements
  cl_mem input_b_buf; // num_devices elements
  cl_mem output_buf;  // num_devices elements
  int status;

// Setup OpenCL program and kernel
  clGetPlatformIDs(1, &platform, NULL);
  clGetPlatformInfo(platform, CL_PLATFORM_NAME, STRING_BUFFER_LEN, char_buffer, NULL);
  printf("%-40s = %s\n", "CL_PLATFORM_NAME", char_buffer);
  clGetPlatformInfo(platform, CL_PLATFORM_VENDOR, STRING_BUFFER_LEN, char_buffer, NULL);
  printf("%-40s = %s\n", "CL_PLATFORM_VENDOR ", char_buffer);
  clGetPlatformInfo(platform, CL_PLATFORM_VERSION, STRING_BUFFER_LEN, char_buffer, NULL);
  printf("%-40s = %s\n\n", "CL_PLATFORM_VERSION ", char_buffer);

  context_properties[1] = (cl_context_properties)platform;
  clGetDeviceIDs(platform, CL_DEVICE_TYPE_GPU, 1, &device, NULL);
  context = clCreateContext(context_properties, 1, &device, NULL, NULL, NULL);
  queue = clCreateCommandQueue(context, device, 0, NULL);

  unsigned char **opencl_program = read_file("vector_add.cl");
  program = clCreateProgramWithSource(context, 1, (const char **)opencl_program, NULL, NULL);
  if (program == NULL)
  {
    printf("Program creation failed\n");
    return 1;
  }

  int success = clBuildProgram(program, 0, NULL, NULL, NULL, NULL);
  if (success != CL_SUCCESS)
    print_clbuild_errors(program, device);
  kernel = clCreateKernel(program, "vector_add", NULL);

// GPU Part
  // Input buffers. (GPU Space)
  input_a_buf = clCreateBuffer(context, CL_MEM_READ_ONLY,
                               N * sizeof(float), NULL, &status);
  checkError(status, "Failed to create buffer for input A");

  input_b_buf = clCreateBuffer(context, CL_MEM_READ_ONLY,
                               N * sizeof(float), NULL, &status);
  checkError(status, "Failed to create buffer for input B");

  // Output buffer.
  output_buf = clCreateBuffer(context, CL_MEM_WRITE_ONLY,
                              N * sizeof(float), NULL, &status);
  checkError(status, "Failed to create buffer for output");

  //Event Initialization
  cl_event write_event[2];
  cl_event kernel_event;
  cl_int errcode = 0;

  // Map to host memory before writing 
  input_a = (float *)clEnqueueMapBuffer(queue, input_a_buf, CL_TRUE,
                                        CL_MAP_WRITE, 0, N * sizeof(float), 0, NULL, &write_event[0], &errcode);
  checkError(errcode, "Failed to map input A");

  input_b = (float *)clEnqueueMapBuffer(queue, input_b_buf, CL_TRUE,
                                        CL_MAP_WRITE, 0, N * sizeof(float), 0, NULL, &write_event[1], &errcode);
  checkError(errcode, "Failed to map input B");


  

// CPU PART

  // Define performance measurement variables
  struct timespec start, end, diff;
  clockid_t clk_id = CLOCK_MONOTONIC;

  // Create random numbers
  clock_gettime(clk_id, &start);
  for (unsigned j = 0; j < N; ++j)
  {
    input_a[j] = rand_float();
    input_b[j] = rand_float();
    //printf("ref %f\n",ref_output[j]);
  }
  clock_gettime(clk_id, &end);
  sub_timespec(start,end,&diff);

  printf("CPU took %d.%.9ld seconds to create random numbers.\n", (int) diff.tv_sec,diff.tv_nsec);


  // Unmap when filled 
  clEnqueueUnmapMemObject(queue,input_a_buf,input_a,0,NULL,NULL);
  clEnqueueUnmapMemObject(queue,input_b_buf,input_b,0,NULL,NULL);

  // Add vectors /!\ if you do that after the GPU activity without having unmapped, data is corrupted and you don't get the good result 
  clock_gettime(clk_id, &start);
  for (unsigned j = 0; j < N; ++j)
  {
    ref_output[j] = input_a[j] + input_b[j];
  }
  clock_gettime(clk_id, &end);
  sub_timespec(start,end,&diff);
  printf("CPU took %d.%.9ld seconds to compute.\n", (int) diff.tv_sec,diff.tv_nsec);

// KERNEL PART
  // Set kernel arguments.
  clock_gettime(clk_id, &start);
  unsigned argi = 0;

  status = clSetKernelArg(kernel, argi++, sizeof(cl_mem), &input_a_buf);
  checkError(status, "Failed to set argument 1");

  status = clSetKernelArg(kernel, argi++, sizeof(cl_mem), &input_b_buf);
  checkError(status, "Failed to set argument 2");

  status = clSetKernelArg(kernel, argi++, sizeof(cl_mem), &output_buf);
  checkError(status, "Failed to set argument 3"); 

  clock_gettime(clk_id, &end);
  sub_timespec(start,end,&diff);

  printf("GPU took %d.%.9ld seconds to set args.\n", (int) diff.tv_sec,diff.tv_nsec);

  // Launch kernels
  const size_t global_work_size = N;

  clock_gettime(clk_id, &start);
  status = clEnqueueNDRangeKernel(queue, kernel, 1, NULL,
                                  &global_work_size, NULL, 2, write_event, &kernel_event); ///THERE you launch the kernels
  checkError(status, "Failed to launch kernel");
  clWaitForEvents(1, &kernel_event); //Wait for the GPU events to end 
  clock_gettime(clk_id, &end);
  sub_timespec(start,end,&diff);
  printf("GPU took %d.%.9ld seconds to compute.\n", (int) diff.tv_sec,diff.tv_nsec);

// Read and Compare results
// Map to host memory
output = (float *)clEnqueueMapBuffer(queue, output_buf, CL_TRUE,
                                     CL_MAP_READ, 0, N * sizeof(float), 0, NULL, NULL, &errcode);
checkError(errcode, "Failed to map output");

bool pass = true;
  for (unsigned j = 0; j < N && pass; ++j)
  {
    if (fabsf(output[j] - ref_output[j]) > 1.0e-5f)
    {
      printf("Failed verification @ index %d\nOutput: %f\nReference: %f\n",
             j, output[j], ref_output[j]);
      pass = false;
    }
  }
  clEnqueueUnmapMemObject(queue, output_buf, output, 0, NULL, NULL); //Don't forget to unmap 

  // Release local events.
  clReleaseEvent(write_event[0]);
  clReleaseEvent(write_event[1]);
  clReleaseKernel(kernel);
  clReleaseCommandQueue(queue);
  clReleaseMemObject(input_a_buf);
  clReleaseMemObject(input_b_buf);
  clReleaseMemObject(output_buf);
  clReleaseProgram(program);
  clReleaseContext(context);

  //--------------------------------------------------------------------

  clFinish(queue);

  return 0;
}

/* PERFORMANCES for N=50000000 :
CPU took 18.53349495 seconds to create random numbers.
GPU took 0.00001404 seconds to set args.

Computing :
Before mapping:
CPU took 0.523s to compute.
GPU took 0.220s to compute. 
After mapping:
CPU took 0.50105840 seconds to compute.
GPU took 0.09505503 seconds to compute.
*/