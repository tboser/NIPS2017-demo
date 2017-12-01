#ifndef FPGAIOREGS_HPP
#define FPGAIOREGS_HPP 1
#include <vector>
typedef std::vector<uint16_t> Image_t;
typedef std::vector<Image_t> ImageBatch_t;
typedef std::vector<uint16_t> Results_t;

class Layer;
class FPGAIORegs {
public:
  FPGAIORegs(const std::string& mmapFilePath="/dev/mem",
	     int16_t divideBy=2);
  ~FPGAIORegs();
  int openMMapFile();
  const int16_t* writeParameters(uint16_t layerID, uint16_t moduleNum, 
				 uint16_t nParameters, const int16_t *data) const; 
  bool writeCnvLayer(const Layer& layer, uint16_t layerID) const;
  bool writeFCLayer(const Layer& layer, uint16_t layerID, size_t nRowsPerMod=0) const;
  bool writeImgBatch(const ImageBatch_t& imgs) const;
  bool readResults(Results_t& res) const;
  
private:
  uint32_t *p_h2p_lw_IO1_addr=0x0; 
  uint32_t *p_h2p_lw_IO2_addr=0x0; 
  uint32_t *p_virtual_base=0x0;

  int m_fd=-1;
  std::string m_mmapFilePath;
  int16_t m_divideBy;
  
  friend void test_FPGAIORegs();
};
#endif
