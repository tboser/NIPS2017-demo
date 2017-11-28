#include <iostream>
#include <vector>
#include "FPGAIORegs.hpp"

void test_FPGAIORegs() {
  FPGAIORegs fIO("/tmp/mmapOut.bin");
  std::vector<int16_t> data= {-0xA,0xB,-0xC,0xD,-0xE,0xF};
  fIO.writeParameters(0xCAFE, 0xDEAD, data.size(), data.data());

}

int main() {
  std::cout << "test_FPGAIORegs starts" << std::endl;
  test_FPGAIORegs();
  std::cout << "test_FPGAIORegs ends" << std::endl;
  return 0;
}
