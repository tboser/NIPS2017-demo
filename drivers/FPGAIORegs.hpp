#ifndef FPGAIOREGS_H
#define FPGAIOREGS_H 1
class FPGAIORegs {
public:
  FPGAIORegs(const std::string& mmapFilePath="/dev/mem");
  ~FPGAIORegs();
  int openMMapFile();
  bool writeParameters(uint16_t layerID, uint16_t filterNum, 
		       uint16_t nParameters, uint16_t *data) const; 

private:
  uint32_t *p_h2p_lw_IO1_addr=0x0; 
  uint32_t *p_h2p_lw_IO2_addr=0x0; 
  uint32_t *p_virtual_base=0x0;
  int m_fd=-1;
  std::string m_mmapFilePath;
  friend void test_FPGAIORegs();
};
#endif
