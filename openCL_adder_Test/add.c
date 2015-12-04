#include <stdio.h>
#include <stdlib.h>
#include <CL/opencl.h>

#include "err_code.h"

#define DATA_SIZE 10
#define FPGA_DEVICE

//using namespace std;
//-----------------------------------------------------------------------------------
int
load_file_to_memory(const char *filename, char **result)
{ 
  size_t size = 0;
  FILE *f = fopen(filename, "rb");
  if (f == NULL) 
  { 
    *result = NULL;
    return -1; // -1 means file opening fail 
  } 
  fseek(f, 0, SEEK_END);
  size = ftell(f);
  fseek(f, 0, SEEK_SET);
  *result = (char *)malloc(size+1);
  if (size != fread(*result, sizeof(char), size, f)) 
  { 
    free(*result);
    return -2; // -2 means file reading fail 
  } 
  fclose(f);
  (*result)[size] = 0;
  return size;
}
//--------------------------------------------------------------------------------------
int main(int argc, char** argv)
{
	cl_context context;
	cl_context_properties properties[3];
	cl_kernel kernel;
	cl_command_queue command_queue;
	cl_program program;
	cl_int err;
	cl_uint num_of_platforms=0;
	cl_platform_id platform_id;
	cl_device_id device_id;
	cl_uint num_of_devices=0;
	cl_mem inputA, inputB, output;

	size_t global;

	float inputDataA[DATA_SIZE]={1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
	float inputDataB[DATA_SIZE]={1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
	float results[DATA_SIZE]={0};

	int i;
	// retreive a list of platforms avaible
	err = clGetPlatformIDs(1, &platform_id, &num_of_platforms);
	checkError(err, "Retreive list of platforms");
	// try to get a supported FPGA device-----------------------------
        int fpga = 0;
        #if defined (FPGA_DEVICE)
        fpga = 1;
        #endif
//-------------------------------------------------------------------------
	err = clGetDeviceIDs(platform_id, CL_DEVICE_TYPE_ALL, 1, &device_id, &num_of_devices);
	checkError(err, "Get devices");
		

	// context properties list - must be terminated with 0
	properties[0]= CL_CONTEXT_PLATFORM;
	properties[1]= (cl_context_properties) platform_id;
	properties[2]= 0;
///My additions///////////////////////////////
       char buffer[10240];
       cl_uint buf_uint;
       size_t wgsize;
       clGetDeviceInfo(device_id,CL_DEVICE_NAME,10240,buffer,NULL);
       printf("DEVICE NAME=%s\n",buffer);
       clGetDeviceInfo(device_id,CL_DEVICE_VENDOR,10240,buffer,NULL);
       printf("DEVICE VENDOR=%s\n",buffer);
       clGetDeviceInfo(device_id,CL_DEVICE_VERSION,10240,buffer,NULL);
       printf("DEVICE VERSION=%s\n",buffer);
       clGetDeviceInfo(device_id,CL_DEVICE_MAX_COMPUTE_UNITS,sizeof(buf_uint),&buf_uint,NULL);
       printf("DEVICE_MAX_COMPUTE_UNITS=%u\n",buf_uint);
       clGetDeviceInfo(device_id,CL_DEVICE_MAX_WORK_GROUP_SIZE,sizeof(size_t),&wgsize,NULL);
       printf("DEVICE_MAX_WORK_GROUP_SIZE=%i\n",(int)wgsize);
/////My additions///////////////////////////////

	// create a context with the GPU device
	context = clCreateContext(properties,1,&device_id,NULL,NULL,&err);

	// create command queue using the context and device
	command_queue = clCreateCommandQueue(context, device_id, 0, &err);

	// create a program from the kernel source code
	char *filename = "add.cl";
	char * ProgramSource;
	size_t stringSize = load_file_to_memory(filename,&ProgramSource);
	if (stringSize < 0)
	{
		printf("Error building program from file.\n");
		return 1;
	}
	program = clCreateProgramWithSource(context,1,(const char **) &ProgramSource, NULL, &err);
	checkError(err, "Create program");
 /*
// My addition---Load binary from disk-------------------------------------
  int status;
  unsigned char *kernelbinary;
  char *xclbin=argv[1];
  printf("loading %s\n", xclbin);
  int n_i = load_file_to_memory(xclbin, (char **) &kernelbinary);
  if (n_i < 0) {
    printf("failed to load kernel from xclbin: %s\n", xclbin);
    printf("Test failed\n");
    return EXIT_FAILURE;
  }
  size_t n = n_i;
  // Create the compute program from offline
  program = clCreateProgramWithBinary(context, 1, &device_id, &n,
                                      (const unsigned char **) &kernelbinary, &status, &err);
///My additions------------------------------------------------------------
*/
	// compile the program
	err = clBuildProgram(program, 0, NULL, NULL, NULL, NULL); 
	checkError(err, "Building program.");
	// specify which kernel from the program to execute
	kernel = clCreateKernel(program, "add", &err);

	// create buffers for the input and ouput

	inputA = clCreateBuffer(context, CL_MEM_READ_ONLY, sizeof(float) * DATA_SIZE, NULL, NULL);
	inputB = clCreateBuffer(context, CL_MEM_READ_ONLY, sizeof(float) * DATA_SIZE, NULL, NULL);
	output = clCreateBuffer(context, CL_MEM_WRITE_ONLY, sizeof(float) * DATA_SIZE, NULL, NULL);

	// load data into the input buffer
	clEnqueueWriteBuffer(command_queue, inputA, CL_TRUE, 0, sizeof(float) * DATA_SIZE, inputDataA, 0, NULL, NULL);
	clEnqueueWriteBuffer(command_queue, inputB, CL_TRUE, 0, sizeof(float) * DATA_SIZE, inputDataB, 0, NULL, NULL);

	// set the argument list for the kernel command
	clSetKernelArg(kernel, 0, sizeof(cl_mem), &inputA);
	clSetKernelArg(kernel, 1, sizeof(cl_mem), &inputB);
	clSetKernelArg(kernel, 2, sizeof(cl_mem), &output);

	global=DATA_SIZE;

// My additions/////////////////
// ////My additions
  size_t cl_kernel_work_group_size;
  cl_uint cl_kernel_preferred_work_group_size_multiple;
////My additions*/

clGetKernelWorkGroupInfo(kernel,device_id,CL_KERNEL_WORK_GROUP_SIZE,sizeof(size_t),&cl_kernel_work_group_size,NULL);
       printf("KERNEL_WORK_GROUP_SIZE=%i\n", (int)cl_kernel_work_group_size);
clGetKernelWorkGroupInfo(kernel,device_id,CL_KERNEL_PREFERRED_WORK_GROUP_SIZE_MULTIPLE,sizeof(size_t),&cl_kernel_preferred_work_group_size_multiple,NULL);
       printf("KERNEL_PREFERRED_WG_SIZE_MULTIPLE=%u\n", cl_kernel_preferred_work_group_size_multiple);
//////My additions*/

	// enqueue the kernel command for execution
	clEnqueueNDRangeKernel(command_queue, kernel, 1, NULL, &global, NULL, 0, NULL, NULL);
	clFinish(command_queue);

	// copy the results from out of the output buffer
	clEnqueueReadBuffer(command_queue, output, CL_TRUE, 0, sizeof(float) *DATA_SIZE, results, 0, NULL, NULL);

//-------------------------------------My additions-------------------------
	// print the results
	printf("input A: \n");
	for(i=0;i<DATA_SIZE; i++)
	{
		printf("%f ",inputDataA[i]);
	}
	
        printf("\n input B: \n");
	for(i=0;i<DATA_SIZE; i++)
	{
		printf("%f ",inputDataB[i]);
	}

	printf("\n output: \n");
	for(i=0;i<DATA_SIZE; i++)
	{
		printf("%f ",results[i]);
	}
        printf("\n");
//-----------------------------------My additions-----------------------------

	// cleanup - release OpenCL resources
	clReleaseMemObject(inputA);
	clReleaseMemObject(inputB);
	clReleaseMemObject(output);
	clReleaseProgram(program);
	clReleaseKernel(kernel);
	clReleaseCommandQueue(command_queue);
	clReleaseContext(context);

	return 0;

}	
