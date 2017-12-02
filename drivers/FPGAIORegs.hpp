#ifndef FPGAIOREGS_HPP
#define FPGAIOREGS_HPP 1
#include <array>
#include <vector>
typedef std::vector<uint16_t> Image_t;
typedef std::vector<Image_t> ImageBatch_t;
typedef std::array<uint16_t,10> Result_t;
typedef std::vector<Result_t > Results_t;

class Layer;
class FPGAIORegs {
public:
  FPGAIORegs(const std::string& mmapFilePath="/dev/mem",
	     int16_t divideBy=2);
  ~FPGAIORegs();
  int openDevMem();
  int openMMapFile();
  const uint16_t* writeData(uint16_t nData, const uint16_t *data) const; 
  const int16_t* writeParameters(uint16_t layerID, uint16_t group, 
				 uint16_t moduleNum, 
				 uint16_t nParameters, const int16_t *data) const; 
  bool writeCnvLayer(const Layer& layer, uint16_t layerID) const;
  bool writeFCLayer(const Layer& layer, uint16_t layerID, size_t nRowsPerMod=0) const;
  bool writeImgBatch(const ImageBatch_t& imgs) const;
  bool readResults(Results_t& res) const;
  void startImgProc() const;
  void resetImgProc() const;
  int waitOnImgProc() const;
  
private:

  uint32_t *p_IParms_addr=0x0;  //write model parms to FPGA - 1st reg
  uint32_t *p_IAddr_addr=0x0;   //write model parms to FPGA - 2nd reg
  uint32_t *p_IImg_addr=0x0;    //reg to write image to FPGA
  uint32_t *p_ORes_addr=0x0;    //reg to read network results from FPGA
  uint32_t *p_IRes_addr=0x0;    //write FPGA address to read results from
  void *p_virtual_base=0x0;
  int m_fd=-1;
  std::string m_mmapFilePath; 
  int16_t m_divideBy;

  uint32_t* calcRegAddress(uint32_t base);
  
  friend void test_FPGAIORegs();
};
#endif
