#ifndef FPGAIOREGS_HPP
#define FPGAIOREGS_HPP 1
#include <array>
#include <vector>
typedef std::vector<uint16_t> Image_t;
typedef std::vector<Image_t> ImageBatch_t;
struct Result {
  std::array<int16_t,10> probs;
  int timeMus;
  Result(int time) : timeMus(time) {}
};
  
typedef std::array<int16_t,10> Result_t;
typedef std::vector<Result_t> Results_t;

class Layer;
class FPGAIORegs {
public:
  FPGAIORegs(const std::string& mmapFilePath="/dev/mem",
	     int debug=1, 
	     int16_t divideBy=4);
  ~FPGAIORegs();
  int openDevMem();
  int openMMapFile();
  const uint16_t* writeData(uint16_t nData, const uint16_t *data) const; 
  const int16_t* writeParameters(uint16_t layerID, uint16_t group, 
				 uint16_t moduleNum, 
				 uint16_t nParameters, const int16_t *data) const; 
  int writeParameter(int layer, int group, int mod_num,
		     int address, int data) const;

  bool writeCnvLayer(const Layer& layer, uint16_t layerID) const;
  bool writeFCLayer(const Layer& layer, uint16_t layerID, size_t nRowsPerMod=0) const;
  bool writeImgBatch(const ImageBatch_t& imgs) const;
  bool readResults(Results_t& res) const;
  void startImgProc() const;
  void resetImgProc() const;
  int waitOnImgProc() const;
  //when  step = 0 => result_data_stream <= image_data_stream;
  //when  1 => result_data_stream <= first_layer_convolution_streams_out(0);
  //when  2 => result_data_stream <= first_layer_bias_and_relu_streams_out(0);
  //when  3 => result_data_stream <= first_layer_max_pooling_streams_out(0);
  //when  4 => result_data_stream <= second_layer_convolution_streams_out(0);
  //when  5 => result_data_stream <= second_layer_add_matrices_out(0);
  //when  6 => result_data_stream <= second_layer_bias_and_relu_streams_out(0);
  //when  7 => result_data_stream <= second_layer_max_pooling_streams_out(0);
  //when  8 => result_data_stream <= dense1_layer_mult_data_stream_out;
  //when  9 => result_data_stream <= dense1_layer_bias_and_relu_streams_out;
  //when 10 => result_data_stream <= dense2_layer_mult_data_stream_out;
  //when 11 => result_data_stream <= dense2_layer_bias_and_relu_streams_out;
  //when 12 => result_data_stream <= dense3_layer_mult_data_stream_out;
  //when 13 => result_data_stream <= dense3_layer_bias_and_relu_streams_out;
  bool selectOutput(int step=13);

  
private:

  uint32_t *p_IParms_addr=0x0;  //write model parms to FPGA - 1st reg
  uint32_t *p_IAddr_addr=0x0;   //write model parms to FPGA - 2nd reg
  uint32_t *p_IImg_addr=0x0;    //reg to write image to FPGA
  uint32_t *p_ORes_addr=0x0;    //reg to read network results from FPGA
  uint32_t *p_IRes_addr=0x0;    //write FPGA address to read results from
  void *p_virtual_base=0x0;
  int m_fd=-1;
  int m_debug;
  std::string m_mmapFilePath; 
  int16_t m_divideBy;

  uint32_t* calcRegAddress(uint32_t base);
  
  friend void test_FPGAIORegs();
};
#endif
