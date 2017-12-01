/*
This program demonstrate how to use hps communicate with FPGA through light AXI Bridge.
uses should program the FPGA by GHRD project before executing the program
refer to user manual chapter 7 for details about the demo
*/
#include <cassert>
#include <iostream>
#include <stdexcept>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#ifdef CROSS_COMPILE
#include "hwlib.h"
#include "socal/socal.h"
#include "socal/hps.h"
#include "socal/alt_gpio.h"
#else
#define ALT_STM_OFST 0x0
#define ALT_LWFPGASLVS_OFST 0x0
#endif
#include "hps_0.h"
#include "layers/TrainedLayers.hpp"
#include "FPGAIORegs.hpp"
using namespace std;

const uint32_t HW_REGS_BASE ( ALT_STM_OFST );
//const uint32_t HW_REGS_SPAN (1<<27); //0x04000000
const uint32_t HW_REGS_SPAN (1<<16);
const uint32_t HW_REGS_MASK ( HW_REGS_SPAN - 1 );

const int16_t*  
FPGAIORegs::writeParameters(uint16_t layerID, uint16_t moduleNum, 
			    uint16_t nParameters, const int16_t *data) const { 
  *p_h2p_lw_IO1_addr = ((1<<31) | (layerID<<16) | moduleNum);
  std::cout << std::hex 
	    << "FPGAIORegs::writeParameters IO1 0x" << layerID << " 0x" << moduleNum 
	    << " 0x" << *p_h2p_lw_IO1_addr << std::dec << std::endl;
  for (uint16_t i=0; i<nParameters; ++i){
    //notice we have to cast data[i] to unsigned to avoid messing up the whole work
    *p_h2p_lw_IO2_addr = (1<<31) | (i<<16) | (uint16_t)data[i]; 
    std::cout << std::hex 
	      << "FPGAIORegs::writeParameters IO2 0x" << i << " 0x" << data[i] 
	      << " 0x" << *p_h2p_lw_IO2_addr << std::dec << std::endl;
  }
  return data + (nParameters * sizeof(int16_t));
}

bool
FPGAIORegs::writeFCLayer(const Layer& layer, uint16_t layerID, size_t nRowsPerMod) const {
  size_t nRows(layer.weightShape[0]);
  assert(nRows-1<=0xFFFE); //leave 0xFFFF for the biases

  if (0==nRowsPerMod) nRowsPerMod=nRows;
  
  size_t nColumns(layer.weightShape[1]);
  size_t nWMod=nRowsPerMod*nColumns;

  const int16_t *pData(layer.weights.data());
  assert(pData);

  //module loop: one module every nRowsPerMod
  for (size_t iR=0; iR<nRows; iR += nRowsPerMod) {
    uint16_t modID = iR;
    std::cout << "FPGAIORegs::writeFCLayer: " << layer.name <<  " layerID "
	      << layerID << " modID " << modID << " first weight address " 
	      << pData << " first weight "  << *pData << std::endl;
    //write nwMod weights for this module
    pData = this->writeParameters(layerID, modID, nWMod, pData);
  }

  //write "divide by" for all layers
  std::cout << "FPGAIORegs::writeCnvLayer: " << layer.name <<  " layerID " << layerID << " divide by " << m_divideBy << std::endl;
  this->writeParameters(layerID, 0xFFFE, 1, &m_divideBy);

  //write biases for nFilters at modID 0xFFFF
  std::cout << "FPGAIORegs::writeFCLayer: " << layer.name <<  " layerID " << layerID << " biases " << *(layer.biases.data()) << std::endl;
  this->writeParameters(layerID, 0xFFFF, layer.nBiases, layer.biases.data());

  return true;
}

bool
FPGAIORegs::writeCnvLayer(const Layer& layer, uint16_t layerID) const {
  size_t nRows(layer.weightShape[0]);
  size_t nCols(layer.weightShape[1]);
  size_t nWMod(nRows*nCols);
  size_t nChannels(layer.weightShape[2]);
  assert(nChannels-1<=0xFE);  //leave 0xFFFF for the biases
  size_t nFilters(layer.weightShape[3]);
  assert(nFilters-1<=0xFF);
  const int16_t *pData(layer.weights.data());
  assert(pData);

  //module loop: one module per input and per output channel
  for (size_t f=0; f<nFilters; ++f) {
    for (size_t i=0; i<nChannels; ++i) {
      uint16_t modID = i * f;
      std::cout << "FPGAIORegs::writeCnvLayer: " << layer.name <<  " layerID " << layerID << " modID " << modID << " first weight address " 
		<< pData << " first weight "  << *pData << std::endl;
      //write nwMod Conv weights for this module
      pData=this->writeParameters(layerID, modID, nWMod, pData);
    }
  }
  
  //write "divide by" for all layers
  std::cout << "FPGAIORegs::writeCnvLayer: " << layer.name <<  " layerID " << layerID << " divide by " << m_divideBy << std::endl;
  this->writeParameters(layerID, 0xFFFE, 1, &m_divideBy);
  
  //write biases for nFilters at modID 0xFFFF
  std::cout << "FPGAIORegs::writeCnvLayer: " << layer.name <<  " layerID " << layerID << " biases " << *(layer.biases.data()) << std::endl;
  this->writeParameters(layerID, 0xFFFF, layer.nBiases, layer.biases.data());

  return true;
}


bool
FPGAIORegs::writeImgBatch(const ImageBatch_t& imgs) const {
  return true;
}


bool
FPGAIORegs::readResults(Results_t& res) const {
  return true;
}

int
FPGAIORegs::openMMapFile() {
  if ( (m_fd = open( m_mmapFilePath.c_str(), 
		   ( O_RDWR | O_SYNC | O_CREAT | O_TRUNC), 
		   (mode_t)0600) ) == -1 ) { 
    std::cerr << "FPGAIORegs::openMMapFile ERROR: could not open " + m_mmapFilePath;
    return -1;
  }
  /* Stretch the file size to the size of the (mmapped) array of ints
   */
  int result = lseek(m_fd, HW_REGS_MASK, SEEK_SET);
  if (result == -1) {
    close(m_fd);
    perror("Error calling lseek() to 'stretch' the file");
    return -1;
  }
  
  /* Something needs to be written at the end of the file to
   * have the file actually have the new size.
   * Just writing an empty string at the current file position will do.
   *
   * Note:
   *  - The current position in the file is at the end of the stretched 
   *    file due to the call to lseek().
   *  - An empty string is actually a single '\0' character, so a zero-byte
   *    will be written at the last byte of the file.
   */
  result = write(m_fd, "", 1);
  if (result != 1) {
    close(m_fd);
    perror("Error writing last byte of the file");
    return -1;
  }
  //ready to go
  return m_fd;
}

FPGAIORegs::FPGAIORegs(const std::string& mmapFilePath, int16_t divideBy) :
  m_mmapFilePath(mmapFilePath), m_divideBy(divideBy)
{
  // map the address space for the IO registers into user space so we can interact with them.
  // we'll actually map in the entire CSR span of the HPS since we want to access various registers within that span

  if ( (openMMapFile()) == -1 ) throw std::runtime_error("could not open mmap file");
  
  p_virtual_base = (uint32_t*)mmap( NULL, HW_REGS_SPAN, ( PROT_READ | PROT_WRITE ), MAP_SHARED, m_fd, HW_REGS_BASE );
  
  if( p_virtual_base == MAP_FAILED ) {
    printf( "ERROR: mmap() failed...\n" );
    close( m_fd );
  }
  // std::cout << std::hex << ALT_LWFPGASLVS_OFST <<std::endl;
  // std::cout << std::hex << IO1_PIO_BASE <<std::endl;
  // std::cout << std::hex << IO2_PIO_BASE <<std::endl;
  // std::cout << std::hex << HW_REGS_MASK <<std::endl;
  // std::cout << std::hex << (IO1_PIO_BASE & HW_REGS_MASK) <<std::endl;
  assert((uint32_t)( ALT_LWFPGASLVS_OFST + IO1_PIO_BASE )< (uint32_t)( HW_REGS_MASK) );
  p_h2p_lw_IO1_addr = p_virtual_base + ((uint32_t)( ALT_LWFPGASLVS_OFST + IO1_PIO_BASE )); 
  assert((uint32_t)( ALT_LWFPGASLVS_OFST + IO2_PIO_BASE )< (uint32_t)( HW_REGS_MASK) );
  p_h2p_lw_IO2_addr = p_virtual_base + ((uint32_t)( ALT_LWFPGASLVS_OFST + IO2_PIO_BASE )); 
  std::cout << "FPGAIORegs::FPGAIORegs addresses " << std::hex
	    << p_h2p_lw_IO1_addr << " and " << p_h2p_lw_IO2_addr << " in virtual space "
	    << p_virtual_base << "\n mapped to " 
	    << m_mmapFilePath << std::dec << std::endl;
}


FPGAIORegs::~FPGAIORegs() {
	// clean up our memory mapping and exit
	
	if( munmap( p_virtual_base, HW_REGS_SPAN ) != 0 ) {
		printf( "ERROR: munmap() failed...\n" );
		close( m_fd );
	}

	close( m_fd );

}
