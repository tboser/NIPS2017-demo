// tests.cpp
// Code inspired by Altera example:
// https://www.altera.com/support/support-resources/design-examples/design-software/opencl/hello_world.html

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <cstring>
#include "CL/opencl.h"
#include "AOCLUtils/aocl_utils.h"

#include "test_inputs.h"

using namespace aocl_utils;

#define STRING_BUFFER_LEN 1024

// Runtime constants
// Used to define the work set over which this kernel will execute.
static const size_t work_group_size = 8;  // 8 threads in the demo workgroup
// Defines kernel argument value, which is the workitem ID that will
// execute a printf call
static const int thread_id_to_output = 2;

// OpenCL runtime configuration
static cl_platform_id platform = NULL;
static cl_device_id device = NULL;
static cl_context context = NULL;
static cl_command_queue queue = NULL;
static cl_program program = NULL;

static cl_kernel convolute_2d_relu = NULL;
//static cl_kernel convolute_2d = NULL;
//static cl_kernel max_pool_2d = NULL;
//static cl_kernel dense = NULL;

// Function prototypes
bool init();
void cleanup();
void conv2d_relu();

static void device_info_ulong( cl_device_id device, cl_device_info param, const char* name);
static void device_info_uint( cl_device_id device, cl_device_info param, const char* name);
static void device_info_bool( cl_device_id device, cl_device_info param, const char* name);
static void device_info_string( cl_device_id device, cl_device_info param, const char* name);
static void display_device_info( cl_device_id device );

// Entry point.
int main() {
  cl_int status;

  if(!init()) {
    return -1;
  }

  conv2d_relu();

  // Free the resources allocated
  cleanup();

  return 0;
}

/////// HELPER FUNCTIONS ///////

void conv2d_relu() {
  cl_int status;

  cl_mem cri;
  cl_mem crw;
  cl_mem crb;
  cl_mem cro;

  scoped_array<scoped_aligned_ptr<float> > output;

  // create data transfer buffers
  cri = clCreateBuffer(context, CL_MEM_READ_ONLY, 784 * sizeof(float), NULL, &status);
  checkError(status, "Failed to create buffer for cri");
  crw = clCreateBuffer(context, CL_MEM_READ_ONLY, 150 * sizeof(float), NULL, &status);
  checkError(status, "Failed to create buffer for crw");
  crb = clCreateBuffer(context, CL_MEM_READ_ONLY, 6 * sizeof(float), NULL, &status);
  checkError(status, "Failed to create buffer for crb");
  cro = clCreateBuffer(context, CL_MEM_WRITE_ONLY, 3456 * sizeof(float), NULL, &status);
  checkError(status, "Failed to create buffer for cro");

  cl_event kernel_event;
  cl_event finish_event;

  // -- START CONV RELU TEST -- //
  const double start_time = getCurrentTimestamp();

  cl_event write_event[3];
  status = clEnqueueWriteBuffer(queue, cri, CL_FALSE, 0, 784 * sizeof(float), conv_relu_in, 0, NULL, &write_event[0]);
  checkError(status, "Failed to transfer input cri");
  status = clEnqueueWriteBuffer(queue, crw, CL_FALSE, 0, 150 * sizeof(float), conv_relu_weights, 0, NULL, &write_event[1]);
  checkError(status, "Failed to transfer input crw");
  status = clEnqueueWriteBuffer(queue, crb, CL_FALSE, 0, 6 * sizeof(float), conv_relu_bias, 0, NULL, &write_event[2]);
  checkError(status, "Failed to transfer input crb");

  status = clSetKernelArg(convolute_2d_relu, 1, sizeof(cl_mem), &cri);
  checkError(status, "Failed to set argument");
  status = clSetKernelArg(convolute_2d_relu, 2, sizeof(cl_mem), &crw);
  checkError(status, "Failed to set argument");
  status = clSetKernelArg(convolute_2d_relu, 3, sizeof(cl_mem), &crb);
  checkError(status, "Failed to set argument");
  status = clSetKernelArg(convolute_2d_relu, 0, sizeof(cl_mem), &cro);
  checkError(status, "Failed to set argument");

  // Other arguments
  int filter_size = 5;
  status = clSetKernelArg(convolute_2d_relu, 4, sizeof(int), &filter_size);
  checkError(status, "Failed to set argument");
  int output_width_height = 24;
  status = clSetKernelArg(convolute_2d_relu, 5, sizeof(int), &output_width_height);
  checkError(status, "Failed to set argument");
  int num_filters = 6;
  status = clSetKernelArg(convolute_2d_relu, 6, sizeof(int), &num_filters);
  checkError(status, "Failed to set argument");
  int num_input_channels = 1;
  status = clSetKernelArg(convolute_2d_relu, 7, sizeof(int), &num_input_channels);
  checkError(status, "Failed to set argument");
  int padded_matrix_dim = 28;
  status = clSetKernelArg(convolute_2d_relu, 8, sizeof(int), &padded_matrix_dim);
  checkError(status, "Failed to set argument");

  // Enqueue kernel.
  size_t global_work_size[3] = {28, 28, 6}; //height, width, num_filters
  status = clEnqueueNDRangeKernel(queue, convolute_2d_relu, 3, NULL, global_work_size, NULL, 3, write_event, &kernel_event);
  checkError(status, "Failed to launch kernel");

  status = clEnqueueReadBuffer(queue, cro, CL_FALSE, 0, 3456 * sizeof(float), output[0], 1, &kernel_event, &finish_event);

  clReleaseEvent(write_event[0]);
  clReleaseEvent(write_event[1]);
  clReleaseEvent(write_event[2]);

  clWaitForEvents(1, &finish_event);

  const double end_time = getCurrentTimestamp();

  printf("\nTime: %0.3f ms\n", (end_time - start_time) * 1e3);

  // Get kernel time using the OpenCL event profiling API.
  cl_ulong time_ns = getStartEndTime(kernel_event);
  printf("Kernel time (conv-relu): %0.3f ms\n", double(time_ns) * 1e-6);

  for (int i = 0; i < 100; i++) {
    printf("%0.3f ", output[0][i]);
  }
  // --- END RUNNING CONV RELU TEST --- //

  clReleaseEvent(kernel_event);
  clReleaseEvent(finish_event);
}

bool init() {
  cl_int status;

  if(!setCwdToExeDir()) {
    return false;
  }

  // Get the OpenCL platform.
  platform = findPlatform("Intel(R) FPGA");
  if(platform == NULL) {
    printf("ERROR: Unable to find Intel(R) FPGA OpenCL platform.\n");
    return false;
  }

  // Query the available OpenCL devices.
  scoped_array<cl_device_id> devices;
  cl_uint num_devices;

  devices.reset(getDevices(platform, CL_DEVICE_TYPE_ALL, &num_devices));

  // We'll just use the first device.
  device = devices[0];

  // Display some device information.
  display_device_info(device);

  // Create the context.
  context = clCreateContext(NULL, 1, &device, NULL, NULL, &status);
  checkError(status, "Failed to create context");

  // Create the command queue.
  queue = clCreateCommandQueue(context, device, CL_QUEUE_PROFILING_ENABLE, &status);
  checkError(status, "Failed to create command queue");

  // Create the program.
  std::string binary_file = getBoardBinaryFile("cnn_kernels", device);
  printf("Using AOCX: %s\n", binary_file.c_str());
  program = createProgramFromBinary(context, binary_file.c_str(), &device, 1);

  // Build the program that was just created.
  status = clBuildProgram(program, 0, NULL, "", NULL, NULL);
  checkError(status, "Failed to build program");

  // Create the kernel - name passed in here must match kernel name in the
  // original CL file, that was compiled into an AOCX file using the AOC tool
  convolute_2d_relu = clCreateKernel(program, "convolute_2d_relu", &status);
  //convolute_2d = clCreateKernel(program, "convolute_2d", &status);
  //max_pool_2d = clCreateKernel(program, "max_pool_2d", &status);
  //dense = clCreateKernel(program, "dense", &status);
  //POSSIBLE ERROR?
  //const char *kernel_name = "hello_world";  // Kernel name, as defined in the CL file
  //kernel = clCreateKernel(program, kernel_name, &status);
  //checkError(status, "Failed to create kernel");

  return true;
}

// Free the resources allocated during initialization
void cleanup() {
  //if(max_pool_2d) {
  //  clReleaseKernel(max_pool_2d);
  //}
  if(convolute_2d_relu) {
    clReleaseKernel(convolute_2d_relu);
  }
  //if(convolute_2d) {
  //  clReleaseKernel(convolute_2d);
  //}
  //if(dense) {
  //  clReleaseKernel(dense);
  //}
  if(program) {
    clReleaseProgram(program);
  }
  if(queue) {
    clReleaseCommandQueue(queue);
  }
  if(context) {
    clReleaseContext(context);
  }
}

// Additional helper functions
// Helper functions to display parameters returned by OpenCL queries
static void device_info_ulong( cl_device_id device, cl_device_info param, const char* name) {
   cl_ulong a;
   clGetDeviceInfo(device, param, sizeof(cl_ulong), &a, NULL);
   printf("%-40s = %lu\n", name, a);
}
static void device_info_uint( cl_device_id device, cl_device_info param, const char* name) {
   cl_uint a;
   clGetDeviceInfo(device, param, sizeof(cl_uint), &a, NULL);
   printf("%-40s = %u\n", name, a);
}
static void device_info_bool( cl_device_id device, cl_device_info param, const char* name) {
   cl_bool a;
   clGetDeviceInfo(device, param, sizeof(cl_bool), &a, NULL);
   printf("%-40s = %s\n", name, (a?"true":"false"));
}
static void device_info_string( cl_device_id device, cl_device_info param, const char* name) {
   char a[STRING_BUFFER_LEN]; 
   clGetDeviceInfo(device, param, STRING_BUFFER_LEN, &a, NULL);
   printf("%-40s = %s\n", name, a);
}

// Query and display OpenCL information on device and runtime environment
static void display_device_info( cl_device_id device ) {

   printf("Querying device for info:\n");
   printf("========================\n");
   device_info_string(device, CL_DEVICE_NAME, "CL_DEVICE_NAME");
   device_info_string(device, CL_DEVICE_VENDOR, "CL_DEVICE_VENDOR");
   device_info_uint(device, CL_DEVICE_VENDOR_ID, "CL_DEVICE_VENDOR_ID");
   device_info_string(device, CL_DEVICE_VERSION, "CL_DEVICE_VERSION");
   device_info_string(device, CL_DRIVER_VERSION, "CL_DRIVER_VERSION");
   device_info_uint(device, CL_DEVICE_ADDRESS_BITS, "CL_DEVICE_ADDRESS_BITS");
   device_info_bool(device, CL_DEVICE_AVAILABLE, "CL_DEVICE_AVAILABLE");
   device_info_bool(device, CL_DEVICE_ENDIAN_LITTLE, "CL_DEVICE_ENDIAN_LITTLE");
   device_info_ulong(device, CL_DEVICE_GLOBAL_MEM_CACHE_SIZE, "CL_DEVICE_GLOBAL_MEM_CACHE_SIZE");
   device_info_ulong(device, CL_DEVICE_GLOBAL_MEM_CACHELINE_SIZE, "CL_DEVICE_GLOBAL_MEM_CACHELINE_SIZE");
   device_info_ulong(device, CL_DEVICE_GLOBAL_MEM_SIZE, "CL_DEVICE_GLOBAL_MEM_SIZE");
   device_info_bool(device, CL_DEVICE_IMAGE_SUPPORT, "CL_DEVICE_IMAGE_SUPPORT");
   device_info_ulong(device, CL_DEVICE_LOCAL_MEM_SIZE, "CL_DEVICE_LOCAL_MEM_SIZE");
   device_info_ulong(device, CL_DEVICE_MAX_CLOCK_FREQUENCY, "CL_DEVICE_MAX_CLOCK_FREQUENCY");
   device_info_ulong(device, CL_DEVICE_MAX_COMPUTE_UNITS, "CL_DEVICE_MAX_COMPUTE_UNITS");
   device_info_ulong(device, CL_DEVICE_MAX_CONSTANT_ARGS, "CL_DEVICE_MAX_CONSTANT_ARGS");
   device_info_ulong(device, CL_DEVICE_MAX_CONSTANT_BUFFER_SIZE, "CL_DEVICE_MAX_CONSTANT_BUFFER_SIZE");
   device_info_uint(device, CL_DEVICE_MAX_WORK_ITEM_DIMENSIONS, "CL_DEVICE_MAX_WORK_ITEM_DIMENSIONS");
   device_info_uint(device, CL_DEVICE_MEM_BASE_ADDR_ALIGN, "CL_DEVICE_MAX_WORK_ITEM_DIMENSIONS");
   device_info_uint(device, CL_DEVICE_MIN_DATA_TYPE_ALIGN_SIZE, "CL_DEVICE_MIN_DATA_TYPE_ALIGN_SIZE");
   device_info_uint(device, CL_DEVICE_PREFERRED_VECTOR_WIDTH_CHAR, "CL_DEVICE_PREFERRED_VECTOR_WIDTH_CHAR");
   device_info_uint(device, CL_DEVICE_PREFERRED_VECTOR_WIDTH_SHORT, "CL_DEVICE_PREFERRED_VECTOR_WIDTH_SHORT");
   device_info_uint(device, CL_DEVICE_PREFERRED_VECTOR_WIDTH_INT, "CL_DEVICE_PREFERRED_VECTOR_WIDTH_INT");
   device_info_uint(device, CL_DEVICE_PREFERRED_VECTOR_WIDTH_LONG, "CL_DEVICE_PREFERRED_VECTOR_WIDTH_LONG");
   device_info_uint(device, CL_DEVICE_PREFERRED_VECTOR_WIDTH_FLOAT, "CL_DEVICE_PREFERRED_VECTOR_WIDTH_FLOAT");
   device_info_uint(device, CL_DEVICE_PREFERRED_VECTOR_WIDTH_DOUBLE, "CL_DEVICE_PREFERRED_VECTOR_WIDTH_DOUBLE");

   {
      cl_command_queue_properties ccp;
      clGetDeviceInfo(device, CL_DEVICE_QUEUE_PROPERTIES, sizeof(cl_command_queue_properties), &ccp, NULL);
      printf("%-40s = %s\n", "Command queue out of order? ", ((ccp & CL_QUEUE_OUT_OF_ORDER_EXEC_MODE_ENABLE)?"true":"false"));
      printf("%-40s = %s\n", "Command queue profiling enabled? ", ((ccp & CL_QUEUE_PROFILING_ENABLE)?"true":"false"));
   }
}
