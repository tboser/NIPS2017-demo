// tests.cpp
// Code inspired by Altera example:
// https://www.altera.com/support/support-resources/design-examples/design-software/opencl/vector-addition.html

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "CL/opencl.h"
#include "AOCLUtils/aocl_utils.h"
#include "test_inputs.h"

using namespace aocl_utils;

// OpenCL runtime configuration
cl_platform_id platform = NULL;
cl_device_id device = NULL;
cl_context context = NULL;
cl_command_queue queue = NULL;
cl_program program = NULL;
// Kernels
cl_kernel convolute_2d_relu = NULL;
cl_kernel convolute_2d = NULL;
cl_kernel max_pool_2d = NULL;
cl_kernel dense = NULL;

// Data transfer buffers:
cl_mem cri;
cl_mem crw;
cl_mem crb;
cl_mem cro;

cl_mem mpi;
cl_mem mpo;

cl_mem cni;
cl_mem cnw;
cl_mem cnb;
cl_mem cno;

cl_mem di;
cl_mem dw;
cl_mem db;
cl_mem dlo;

scoped_array<scoped_aligned_ptr<float> > output;

// Function prototypes
bool init_opencl();
void run_tests();
void cleanup();

// Entry point.
int main(int argc, char **argv) {
  // In case we want to add some command line args
  //Options options(argc, argv);

  // Initialize OpenCL.
  if(!init_opencl()) {
    return -1;
  }

  // Run the kernel.
  run_tests();

  // Free the resources allocated
  cleanup();

  return 0;
}

/////// HELPER FUNCTIONS ///////

// Initializes the OpenCL objects.
bool init_opencl() {
  cl_int status;
  cl_int num_devices;

  printf("Initializing OpenCL\n");

  // Get the OpenCL platform.
  platform = findPlatform("Intel");
  if(platform == NULL) {
    printf("ERROR: Unable to find Intel FPGA OpenCL platform.\n");
    return false;
  }

  // Query the available OpenCL device.
  printf("Platform: %s\n", getPlatformName(platform).c_str());
  clGetDeviceIDs(platform, CL_DEVICE_TYPE_ALL, 1, &device, &num_devices);

  // Create the context.
  context = clCreateContext(0, 1, &device, NULL, NULL, &status);
  checkError(status, "Failed to create context");

  // Create the program for all device. Use the first device as the
  // representative device (assuming all device are of the same type).
  printf("Using AOCX: %s\n", "cnn_kernels.aocx");
  program = createProgramFromBinary(context, "../../device/bin/cnn_kernels.aocx", device, num_devices);

  // Build the program that was just created.
  status = clBuildProgram(program, 0, NULL, "", NULL, NULL);
  checkError(status, "Failed to build program");

  queue = clCreateCommandQueue(context, device, CL_QUEUE_PROFILING_ENABLE, &status);
  checkError(status, "Failed to create command queue");

  convolute_2d_relu = clCreateKernel(program, "convolute_2d_relu", &status);
  convolute_2d = clCreateKernel(program, "convolute_2d", &status);
  max_pool_2d = clCreateKernel(program, "max_pool_2d", &status);
  dense = clCreateKernel(program, "dense", &status);

  // create data transfer buffers
  cri = clCreateBuffer(context, CL_MEM_READ_ONLY, 784 * sizeof(float), NULL, &status);
  checkError(status, "Failed to create buffer for cri");
  crw = clCreateBuffer(context, CL_MEM_READ_ONLY, 150 * sizeof(float), NULL, &status);
  checkError(status, "Failed to create buffer for crw");
  crb = clCreateBuffer(context, CL_MEM_READ_ONLY, 6 * sizeof(float), NULL, &status);
  checkError(status, "Failed to create buffer for crb");
  cro = clCreateBuffer(context, CL_MEM_WRITE_ONLY, 3456 * sizeof(float), NULL, &status);
  checkError(status, "Failed to create buffer for cro");

  mpi = clCreateBuffer(context, CL_MEM_READ_ONLY, 3456 * sizeof(float), NULL, &status);
  checkError(status, "Failed to create buffer for mpi");
  mpo = clCreateBuffer(context, CL_MEM_WRITE_ONLY, 864 * sizeof(float), NULL, &status);
  checkError(status, "Failed to create buffer for mpo");

  cni = clCreateBuffer(context, CL_MEM_READ_ONLY, 864 * sizeof(float), NULL, &status);
  checkError(status, "Failed to create buffer for cni");
  cnw = clCreateBuffer(context, CL_MEM_READ_ONLY, 2400 * sizeof(float), NULL, &status);
  checkError(status, "Failed to create buffer for cnw");
  cnb = clCreateBuffer(context, CL_MEM_READ_ONLY, 16 * sizeof(float), NULL, &status);
  checkError(status, "Failed to create buffer for cnb");
  cno = clCreateBuffer(context, CL_MEM_WRITE_ONLY, 1024 * sizeof(float), NULL, &status);
  checkError(status, "Failed to create buffer for cno");

  di = clCreateBuffer(context, CL_MEM_READ_ONLY, 256 * sizeof(float), NULL, &status);
  checkError(status, "Failed to create buffer for di");
  dw = clCreateBuffer(context, CL_MEM_READ_ONLY, 30720 * sizeof(float), NULL, &status);
  checkError(status, "Failed to create buffer for dw");
  db = clCreateBuffer(context, CL_MEM_READ_ONLY, 120 * sizeof(float), NULL, &status);
  checkError(status, "Failed to create buffer for db");
  dlo = clCreateBuffer(context, CL_MEM_WRITE_ONLY, 120 * sizeof(float), NULL, &status);
  checkError(status, "Failed to create buffer for dlo");
  // TODO -- look into storing weights on FPGA.

  return true;
}


void run_tests() {
  cl_int status;
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
  const size_t[] global_work_size = {28, 28, 6}; //height, width, num_filters
  status = clEnqueueNDRangeKernel(queue, convolute_2d_relu, 3, NULL, &global_work_size, NULL, 3, write_event, &kernel_event);
  checkError(status, "Failed to launch kernel");

  status = clEnqueueReadBuffer(queue, cro, CL_FALSE, 0, 3456 * sizeof(float), output[0], 1, &kernel_event, &finish_event);

  clReleaseEvent(write_event[0]);
  clReleaseEvent(write_event[1]);
  clReleaseEvent(write_event[2]);

  clWaitForEvents(1, finish_event);

  const double end_time = getCurrentTimestamp();

  printf("\nTime: %0.3f ms\n", (end_time - start_time) * 1e3);

  // Get kernel time using the OpenCL event profiling API.
  cl_ulong time_ns = getStartEndTime(kernel_event);
  printf("Kernel time (conv-relu): %0.3f ms\n", double(time_ns) * 1e-6);

  // --- END RUNNING CONV RELU TEST --- //

  // TODO - implement other layers here! TODO - verify results
  clReleaseEvent(kernel_event);
  clReleaseEvent(finish_event);
}

// Free the resources allocated during initialization
void cleanup() {
  if(max_pool_2d) {
    clReleaseKernel(max_pool_2d);
  }
  if(convolute_2d_relu) {
    clReleaseKernel(convolute_2d_relu);
  }
  if(convolute_2d) {
    clReleaseKernel(convolute_2d);
  }
  if(dense) {
    clReleaseKernel(dense);
  }
  if(queue) {
    clReleaseCommandQueue(queue);
  }
  if(program) {
    clReleaseProgram(program);
  }
  if(context) {
    clReleaseContext(context);
  }
}

