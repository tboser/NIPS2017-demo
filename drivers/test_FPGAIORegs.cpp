#include <iostream>
#include <vector>
#include "FPGAIORegs.hpp"

void test_FPGAIORegs() {
#ifdef ONDE10
  FPGAIORegs fIO("/dev/mem", 3);
#else
  FPGAIORegs fIO("/tmp/test.bin", 3 /*debug*/);
#endif
  std::vector<uint16_t> udata= {0xA,0xB,0xC,0xD,0xE,0xF};
  fIO.writeData(udata.size(), udata.data());
  std::vector<int16_t> data= {-0xA,0xB,-0xC,0xD,-0xE,0xF};
  fIO.writeParameters(0xCAFE, 0xDE, 0XAD, data.size(), data.data());

}

int main() {
  std::cout << "test_FPGAIORegs starts" << std::endl;
  test_FPGAIORegs();
  std::cout << "test_FPGAIORegs ends" << std::endl;
  return 0;
}
