#ifndef FPGAIOREGS_HPP
#define FPGAIOREGS_HPP 1
class Layer;
class FPGAIORegs {
public:
  FPGAIORegs(const std::string& mmapFilePath="/dev/mem");
  ~FPGAIORegs();
  int openMMapFile();
  bool writeParameters(uint16_t layerID, uint16_t moduleNum, 
		       uint16_t nParameters, const int16_t *data) const; 
  bool writeCnvLayer(const Layer& layer, uint16_t layerID) const;

private:
  uint32_t *p_h2p_lw_IO1_addr=0x0; 
  uint32_t *p_h2p_lw_IO2_addr=0x0; 
  uint32_t *p_virtual_base=0x0;
  int m_fd=-1;
  std::string m_mmapFilePath;
  friend void test_FPGAIORegs();
};
#endif
