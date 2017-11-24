__kernel void convolute_2d_relu(__global float *c, __global const float *a, 
                                __global float *b, __constant float *d, int filter_size,
                                int output_width_height, int num_filters, int num_input_channels, 
                                int padded_matrix_dim)
{
    size_t j = get_global_id(0); //height
    size_t i = get_global_id(1); //width
    size_t k = get_global_id(2); //num filters (output depth)
    size_t h;
    size_t l;
    size_t s;
    float conv_result = 0;
    for(h = 0; h < num_input_channels; h++) {
        for(l = 0; l < filter_size; l++) {
            if(num_input_channels == 1) {
                for(s = 0; s < filter_size; s++) {
                    conv_result += b[k + s*num_filters + l*num_filters*filter_size]*a[j+s + (i+l)*padded_matrix_dim];
                }
            } else {
                for(s = 0; s < filter_size; s++) {
                    conv_result += b[k + h*num_filters + s*num_filters*num_input_channels + l*num_filters*num_input_channels*filter_size]*a[h + (j+s)*num_input_channels + (i+l)*num_input_channels*padded_matrix_dim];
                }
            }
        }
    }
    conv_result += d[k];
    if (conv_result < 0) {
        conv_result = 0;
    }
    c[k + j*num_filters + i*num_filters*output_width_height] = conv_result;
}

__kernel void convolute_2d(__global float *c, __global const float *a, 
                           __global float *b, __constant float *d, int filter_size, 
                           int output_width_height, int num_filters, int num_input_channels, 
                           int padded_matrix_dim)
{
    size_t j = get_global_id(0); //height
    size_t i = get_global_id(1); //width
    size_t k = get_global_id(2); //num filters (output depth)
    size_t h;
    size_t l;
    size_t s;
    float conv_result = 0;
    for(h = 0; h < num_input_channels; h++) {
        for(l = 0; l < filter_size; l++) {
            if(num_input_channels == 1) {
                for(s = 0; s < filter_size; s++) {
                    conv_result += b[k + s*num_filters + l*num_filters*filter_size]*a[j+s + (i+l)*padded_matrix_dim];
                }
            } else {
                for(s = 0; s < filter_size; s++) {
                    conv_result += b[k + h*num_filters + s*num_filters*num_input_channels + l*num_filters*num_input_channels*filter_size]*a[h + (j+s)*num_input_channels + (i+l)*num_input_channels*padded_matrix_dim];
                }
            }
        }
    }
    conv_result += d[k];
    c[k + j*num_filters + i*num_filters*output_width_height] = conv_result;
}

__kernel void max_pool_2d(__global float *c, __global const float *a, 
                          int kernel_size, int stride, int output_width_height, 
                          int num_filters, int input_w_h)
{
    size_t j = get_global_id(0); //height
    size_t i = get_global_id(1); //width
    size_t k = get_global_id(2); //num filters (output depth)
    int l;
    int s;
    int i_i = i*2;
    int j_j = j*2;
    float curr_max = 0;
    for (l = 0; l < kernel_size; l++) {
        for (s = 0; s < kernel_size; s++) {
            if (curr_max < a[k + (j_j+s)*num_filters + (i_i+l)*num_filters*input_w_h]) {
                curr_max = a[k + (j_j+s)*num_filters + (i_i+l)*num_filters*input_w_h];
            }
        }
    } 
    c[k + j*num_filters + i*num_filters*output_width_height] = curr_max;
}

__kernel void dense(__global float* c, __global float* a, 
          __global float* b, __global float* d, int wa)
{
    // a - weight matrix, b - input vector, c - output vector, d - bias vector, wa - width of a
    // We can simplify these operations because 
    // our weights are conveniently
    // created for a flattened array
    int j = get_global_id(0); // height
    int i = get_global_id(1); // width

    // Write the matrix to device memory each 
    // thread writes one element
    c[j] += a[j*wA + i]*b[i] + d[i];
}
