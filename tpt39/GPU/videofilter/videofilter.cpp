#include <stdio.h>
#include <stdlib.h>
#include <iostream> // for standard I/O
#include <fstream>
#include <math.h>
#include <time.h>
#include <CL/cl.h>
#include <CL/cl_ext.h>
#include "opencv2/opencv.hpp"
#define STRING_BUFFER_LEN 1024

using namespace cv;
using namespace std;

const char *getErrorString(cl_int error)
{
switch(error){
    // run-time and JIT compiler errors
    case 0: return "CL_SUCCESS";
    case -1: return "CL_DEVICE_NOT_FOUND";
    case -2: return "CL_DEVICE_NOT_AVAILABLE";
    case -3: return "CL_COMPILER_NOT_AVAILABLE";
    case -4: return "CL_MEM_OBJECT_ALLOCATION_FAILURE";
    case -5: return "CL_OUT_OF_RESOURCES";
    case -6: return "CL_OUT_OF_HOST_MEMORY";
    case -7: return "CL_PROFILING_INFO_NOT_AVAILABLE";
    case -8: return "CL_MEM_COPY_OVERLAP";
    case -9: return "CL_IMAGE_FORMAT_MISMATCH";
    case -10: return "CL_IMAGE_FORMAT_NOT_SUPPORTED";
    case -11: return "CL_BUILD_PROGRAM_FAILURE";
    case -12: return "CL_MAP_FAILURE";
    case -13: return "CL_MISALIGNED_SUB_BUFFER_OFFSET";
    case -14: return "CL_EXEC_STATUS_ERROR_FOR_EVENTS_IN_WAIT_LIST";
    case -15: return "CL_COMPILE_PROGRAM_FAILURE";
    case -16: return "CL_LINKER_NOT_AVAILABLE";
    case -17: return "CL_LINK_PROGRAM_FAILURE";
    case -18: return "CL_DEVICE_PARTITION_FAILED";
    case -19: return "CL_KERNEL_ARG_INFO_NOT_AVAILABLE";

    // compile-time errors
    case -30: return "CL_INVALID_VALUE";
    case -31: return "CL_INVALID_DEVICE_TYPE";
    case -32: return "CL_INVALID_PLATFORM";
    case -33: return "CL_INVALID_DEVICE";
    case -34: return "CL_INVALID_CONTEXT";
    case -35: return "CL_INVALID_QUEUE_PROPERTIES";
    case -36: return "CL_INVALID_COMMAND_QUEUE";
    case -37: return "CL_INVALID_HOST_PTR";
    case -38: return "CL_INVALID_MEM_OBJECT";
    case -39: return "CL_INVALID_IMAGE_FORMAT_DESCRIPTOR";
    case -40: return "CL_INVALID_IMAGE_SIZE";
    case -41: return "CL_INVALID_SAMPLER";
    case -42: return "CL_INVALID_BINARY";
    case -43: return "CL_INVALID_BUILD_OPTIONS";
    case -44: return "CL_INVALID_PROGRAM";
    case -45: return "CL_INVALID_PROGRAM_EXECUTABLE";
    case -46: return "CL_INVALID_KERNEL_NAME";
    case -47: return "CL_INVALID_KERNEL_DEFINITION";
    case -48: return "CL_INVALID_KERNEL";
    case -49: return "CL_INVALID_ARG_INDEX";
    case -50: return "CL_INVALID_ARG_VALUE";
    case -51: return "CL_INVALID_ARG_SIZE";
    case -52: return "CL_INVALID_KERNEL_ARGS";
    case -53: return "CL_INVALID_WORK_DIMENSION";
    case -54: return "CL_INVALID_WORK_GROUP_SIZE";
    case -55: return "CL_INVALID_WORK_ITEM_SIZE";
    case -56: return "CL_INVALID_GLOBAL_OFFSET";
    case -57: return "CL_INVALID_EVENT_WAIT_LIST";
    case -58: return "CL_INVALID_EVENT";
    case -59: return "CL_INVALID_OPERATION";
    case -60: return "CL_INVALID_GL_OBJECT";
    case -61: return "CL_INVALID_BUFFER_SIZE";
    case -62: return "CL_INVALID_MIP_LEVEL";
    case -63: return "CL_INVALID_GLOBAL_WORK_SIZE";
    case -64: return "CL_INVALID_PROPERTY";
    case -65: return "CL_INVALID_IMAGE_DESCRIPTOR";
    case -66: return "CL_INVALID_COMPILER_OPTIONS";
    case -67: return "CL_INVALID_LINKER_OPTIONS";
    case -68: return "CL_INVALID_DEVICE_PARTITION_COUNT";

    // extension errors
    case -1000: return "CL_INVALID_GL_SHAREGROUP_REFERENCE_KHR";
    case -1001: return "CL_PLATFORM_NOT_FOUND_KHR";
    case -1002: return "CL_INVALID_D3D10_DEVICE_KHR";
    case -1003: return "CL_INVALID_D3D10_RESOURCE_KHR";
    case -1004: return "CL_D3D10_RESOURCE_ALREADY_ACQUIRED_KHR";
    case -1005: return "CL_D3D10_RESOURCE_NOT_ACQUIRED_KHR";
    default: return "Unknown OpenCL error";
    }
}

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

void checkError(int status, const char *msg)
{
  if (status != CL_SUCCESS)
    printf("%s\n", msg);
}


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


#define SHOW
int main(int, char**)
{	
///Video input and video output setup
    VideoCapture camera("./bourne.mp4");
    if(!camera.isOpened())  // check if we succeeded
        return -1;

    const string NAME = "./output1.avi";   // Form the new name with container
    int ex = static_cast<int>(CV_FOURCC('M','J','P','G'));
	uint HEIGHT = camera.get(CV_CAP_PROP_FRAME_HEIGHT);
	uint WIDTH = camera.get(CV_CAP_PROP_FRAME_WIDTH);

    Size S = Size((int) WIDTH,    // Acquire input size
                  (int) HEIGHT);
	//Size S =Size(1280,720);
	cout << "SIZE:" << S << endl;
	
    VideoWriter outputVideo;                                        // Open the output
        outputVideo.open(NAME, ex, 25, S, true);

    if (!outputVideo.isOpened())
    {
        cout  << "Could not open the output video for write: " << NAME << endl;
        return -1;
    }
	struct timespec start,end;
	struct timespec diff;
	clockid_t clk_id = CLOCK_MONOTONIC;
	int count=0;
	const char *windowName = "filter";   // Name shown in the GUI window.

	/// Setup CL
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
/// Setup Parameters 
//   const cl_int frame_HEIGHT=HEIGHT, frame_WIDTH = WIDTH;
  char *output;
  char *padded_input_frame;
  cl_mem input_frame_buf;                                         // num_devices elements       // num_devices elements
  cl_mem output_buf;
  cl_int errcode = 0;                                         
  int status;

///Setup CL program and kernel
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

  unsigned char **opencl_program = read_file("videofilter.cl");
  program = clCreateProgramWithSource(context, 1, (const char **)opencl_program, NULL, NULL);
  if (program == NULL)
  {
    printf("Program creation failed\n");
    return 1;
  }

  int success = clBuildProgram(program, 0, NULL, NULL, NULL, NULL);
  if (success != CL_SUCCESS)
    print_clbuild_errors(program, device);
  
  kernel = clCreateKernel(program, "videofilter", &errcode);
  printf("Kernel Creation: %s\n",getErrorString(errcode));
  checkError(errcode, "Failed to create kernel");

  // Input buffers. (GPU Space)
  input_frame_buf = clCreateBuffer(context, CL_MEM_READ_ONLY,
                               (HEIGHT+2) * (WIDTH+2) * sizeof(char), NULL, &status); // WARNING ON THE SIZE
  checkError(status, "Failed to create buffer for input frame");

// Output buffer.
  output_buf = clCreateBuffer(context, CL_MEM_WRITE_ONLY,
                              HEIGHT * WIDTH * sizeof(char), NULL, &status);
  printf("Output Buffer Creation: %s\n",getErrorString(status));
  checkError(status, "Failed to create buffer for output");

//Event Initialization
  cl_event write_event;
  cl_event kernel_event;

/// Filtering
    #ifdef SHOW
    namedWindow(windowName); // Resizable window, might not work on Windows.
    #endif
    while (true) {
      Mat cameraFrame,displayframe;
		  count=count+1;
		  if(count > 299) break;
      camera >> cameraFrame; // Here is the Frame we are going to filter 
      Mat filterframe = Mat(cameraFrame.size(), CV_8UC3 ); 	
		  Mat grayframe,edge;
    	cvtColor(cameraFrame, grayframe, CV_BGR2GRAY); 
    	// Clear the output image to black, so that the cartoon line drawings will be black (ie: not drawn).
      memset((char*)displayframe.data, 0, displayframe.step * displayframe.rows);
		  grayframe.copyTo(displayframe,edge);
      //cvtColor(displayframe, displayframe, CV_GRAY2BGR);
		  // outputVideo << displayframe;
	
	///GPU part, not optimized yet, some things might be taken out of the while loop

/// Map to host memory with padding 
///////FIRST KERNEL
Mat cpy_input_frame(HEIGHT,WIDTH,CV_8UC1);

    // memcpy(input_frame,grayframe.data,HEIGHT*WIDTH*sizeof(char));
    copyMakeBorder( grayframe, cpy_input_frame, 1, 1, 1, 1, BORDER_REPLICATE );
    
    padded_input_frame = (char *)clEnqueueMapBuffer(queue, input_frame_buf, CL_TRUE,
                                      CL_MAP_WRITE, 0, (HEIGHT+2) * (WIDTH+2) * sizeof(char), 0, NULL, &write_event, &errcode);
    checkError(errcode, "Failed to map input frame");
    memcpy(padded_input_frame,cpy_input_frame.data,(HEIGHT+2)*(WIDTH+2)*sizeof(char));
  // Unmap buffers when filled
    clEnqueueUnmapMemObject(queue, input_frame_buf, padded_input_frame, 0, NULL, NULL);
  // KERNEL PART
    // Set kernel arguments.
      int argi = 0;
      status = clSetKernelArg(kernel, argi++, sizeof(cl_mem), &input_frame_buf);
      printf("Argument Setting : %s\n",getErrorString(status));
      checkError(status, "Failed to set argument 1");

      status = clSetKernelArg(kernel, argi++, sizeof(cl_mem), &output_buf);
      checkError(status, "Failed to set argument 2");

    // Launch kernels
      const size_t global_work_size[2] = {WIDTH,HEIGHT};
      clock_gettime(clk_id, &start);
  
      status = clEnqueueNDRangeKernel(queue, kernel, 2, NULL,
                                      global_work_size, NULL , 1, &write_event, &kernel_event); ///THERE you launch the kernels
      printf("Launching Kernel : %s\n",getErrorString(status));
      checkError(status, "Failed to launch kernel");
      clWaitForEvents(1, &kernel_event); //Wait for the GPU events to end
  
      clock_gettime(clk_id, &end);
      sub_timespec(start,end,&diff);
      printf("GPU took %d.%.9ld seconds to filter frame %d.\n", (int) diff.tv_sec,diff.tv_nsec,count);


    // Read and compare results
      // Map to host memory before reading
      output = (char *)clEnqueueMapBuffer(queue, output_buf, CL_TRUE,
                                          CL_MAP_READ, 0, HEIGHT * WIDTH * sizeof(char), 0, NULL, NULL, &errcode);
      printf("Output Mapping : %s\n",getErrorString(errcode));
      checkError(errcode, "Failed to map output");

    clEnqueueUnmapMemObject(queue, output_buf, output, 0, NULL, NULL);//Don't forget to unmap 
    
    //////////KERNEL 2 (Had no time to do this)
    // Mat cpy_output1(HEIGHT,WIDTH,CV_8UC1);
    // memcpy(cpy_output1,output,HEIGHT*WIDTH*sizeof(char));
    // // memcpy(input_frame,grayframe.data,HEIGHT*WIDTH*sizeof(char));
    // copyMakeBorder( cpy_output1, cpy_output1, 1, 1, 1, 1, BORDER_REPLICATE );
    
    // padded_input_frame = (char *)clEnqueueMapBuffer(queue, input_frame_buf, CL_TRUE,
    //                                   CL_MAP_WRITE, 0, (HEIGHT+2) * (WIDTH+2) * sizeof(char), 0, NULL, &write_event, &errcode);
    // checkError(errcode, "Failed to map input frame");
    // memcpy(padded_input_frame,cpy_output1.data,(HEIGHT+2)*(WIDTH+2)*sizeof(char));


// Convert the output in an openCV matrix and then in the video
    Mat output_grayframe(HEIGHT,WIDTH,CV_8UC1) ;
    memcpy(output_grayframe.data, output,HEIGHT*WIDTH*sizeof(char));
    Mat output_displayframe(cameraFrame.size(),CV_8UC3);
    cvtColor(output_grayframe,output_displayframe,COLOR_GRAY2RGB) ;
    outputVideo << output_displayframe;

	#ifdef SHOW
        imshow(windowName, displayframe);
	#endif
	}

	// Release local events.
  clReleaseEvent(write_event);
  clReleaseKernel(kernel);
  clReleaseCommandQueue(queue);
  clReleaseMemObject(input_frame_buf);
  clReleaseMemObject(output_buf);
  clReleaseProgram(program);
  clReleaseContext(context);

//--------------------------------------------------------------------
  clFinish(queue);
	outputVideo.release();
	camera.release();
  	printf ("CPU terminated : FPS %.2lf .\n", 299.0);///((float)diff.tv_sec+(float)diff.tv_nsec/(float)1000000000));// worng value for now

    return EXIT_SUCCESS;

}


/* copy openCV mat to an array : memcpy(input,cameraFrame.data,3*HEIGHT*WIDTH*sizeof(char))
to pad : copyMakeBorder(src,dst,top,bottom,left,right,borderType,value)*/