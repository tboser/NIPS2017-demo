// lenet_main.cpp
//
//
// Host code for LeNet inference.

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "CL/opencl.h"
#include "AOCLUtils/aocl_utils.h"

using namespace aocl_utils;

// OpenCL runtime configuration
static cl_platform_id platform = NULL;
static cl_device_id device = NULL;
static cl_context context = NULL;
static cl_command_queue queue = NULL;
static cl_program program = NULL;
static cl_kernel kernel = NULL;

// TODO - decide whether or not to use SVM API here.
scoped_array<cl_mem> input_buf;
scoped_array<cl_mem> output_buf;

