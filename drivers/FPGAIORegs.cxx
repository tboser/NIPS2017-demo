/*
This program demonstrate how to use hps communicate with FPGA through light AXI Bridge.
uses should program the FPGA by GHRD project before executing the program
refer to user manual chapter 7 for details about the demo
*/
//#define ONDE10 1
#include <cassert>
#include <iostream>
#include <stdexcept>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#ifdef ONDE10
#include "hwlib.h"
#include "socal/socal.h"
#include "socal/hps.h"
#include "socal/alt_gpio.h"
const uint32_t HW_REGS_SPAN (0x04000000);
#else
#define ALT_STM_OFST 0x0
#define ALT_LWFPGASLVS_OFST 0x0
const uint32_t HW_REGS_SPAN (1<<16);
#endif
//#include "hps_0.h"
#include "TrainedLayers.hpp"
#include "FPGAIORegs.hpp"
using namespace std;

const uint32_t IPARMS_PIO_BASE=0x3100;
const uint32_t ORES_PIO_BASE  =0x3200;
const uint32_t IADDR_PIO_BASE =0x3300;
const uint32_t IIMG_PIO_BASE  =0x3400;
const uint32_t IRES_PIO_BASE  =0x3500;

const uint32_t HW_REGS_BASE ( ALT_STM_OFST );
const uint32_t HW_REGS_MASK ( HW_REGS_SPAN - 1 );

const uint32_t STARTMASK=1<<30;
const uint32_t RESETMASK=1<<31;
const uint32_t WRITTENMASK=1<<31;

const uint16_t WEIGHTGRP=0;
const uint16_t BIASGRP=1;

inline uint32_t*
FPGAIORegs::calcRegAddress(uint32_t base) {

  /*  std::cout << std::hex << ALT_LWFPGASLVS_OFST <<std::endl;
  std::cout << std::hex << IPARMS_PIO_BASE <<std::endl;
  std::cout << std::hex << HW_REGS_MASK <<std::endl;
  std::cout << std::hex << ((uint32_t)( ALT_LWFPGASLVS_OFST + IPARMS_PIO_BASE ) & (uint32_t)( HW_REGS_MASK ))<<std::endl;
  std::cout << std::hex << pp_virtual_base + ((uint32_t)( ALT_LWFPGASLVS_OFST + IPARMS_PIO_BASE ) & (uint32_t)( HW_REGS_MASK ))<<std::endl;
  */
  //cast void* to uint8_t* to get rid of ptr arithmetic warning
  void *pReg = (uint8_t*)p_virtual_base +
    ((uint32_t)( ALT_LWFPGASLVS_OFST + base ) & (uint32_t)( HW_REGS_MASK ) );
  return (uint32_t*) pReg;
}

void
FPGAIORegs::startImgProc() const {
  cout << "FPGAIORegs::startImgProc" <<endl;
  *p_IImg_addr = 0;
  *p_IImg_addr = STARTMASK;
  *p_IImg_addr = 0;
}

int
FPGAIORegs::waitOnImgProc() const {
  cout << "FPGAIORegs::waitOnImgProc" <<endl;
  int i(0);
  while (0 != (*p_IImg_addr & STARTMASK)) {
    usleep(1);
    ++i;
  }
  return i;
}
  
void
FPGAIORegs::resetImgProc() const {
  cout << "FPGAIORegs::resetImgProc" <<endl;
  *p_IImg_addr = RESETMASK;
  usleep(1);
  *p_IImg_addr = 0;
}
const uint16_t*  
FPGAIORegs::writeData(uint16_t nData, const uint16_t *data) const { 
  for (uint16_t i=0; i<nData; ++i){
    //notice we have to cast data[i] to unsigned to avoid messing up the whole word
    //here WRITTENMASK is bit 30 not 32
    *p_IImg_addr = (1<<29) | (i<<16) | (uint16_t)data[i]; 
    if (0 != data[i]) {
      std::cout << "FPGAIORegs::writeData i=" << std::dec << i 
		<< std::dec << " data=" << data[i] 
		<< std::hex << " data=0x" << data[i] 
		<< " output is 0x" << *p_IImg_addr << std::dec << std::endl;
    }
  }
  return data + (nData * sizeof(uint16_t));
}
const int16_t*  
FPGAIORegs::writeParameters(uint16_t layerID, uint16_t group, uint16_t moduleNum,
			    uint16_t nParameters, const int16_t *data) const { 
  assert(layerID <= 0xFFFF);
  assert(group <= 0xFF);
  assert(moduleNum <= 0xFF);
  *p_IParms_addr = ((WRITTENMASK) | (layerID<<16) | (group<<8) | moduleNum);
  std::cout << std::hex 
	    << "FPGAIORegs::writeParameters IParms layer 0x" << layerID 
	    << " group 0x" << group << " mod 0x" << moduleNum 
	    << " outpus it 0x" << *p_IParms_addr << std::dec << std::endl;
  for (uint16_t i=0; i<nParameters; ++i){
    //notice we have to cast data[i] to unsigned to avoid messing up the whole word
    *p_IAddr_addr = (WRITTENMASK) | (i<<16) | (uint16_t)data[i]; 
    std::cout << std::hex 
	      << "FPGAIORegs::writeParameters IAddr 0x" << i 
	      << std::dec
	      << " data=" << data[i] 
	      << std::hex
	      << " data 0x" << data[i] 
	      << " 0x" << *p_IAddr_addr << std::dec << std::endl;
  }
  
  //write "divide by" for all weight layers
  if (group == WEIGHTGRP) {
    std::cout << "FPGAIORegs::writeParameters: layerID " << layerID << " divide by " << m_divideBy << std::endl;
    
    *p_IAddr_addr = (WRITTENMASK) |  ((1+nParameters)<<16) | m_divideBy;
    
    *p_IParms_addr=0;
  }
  return data + nParameters;
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
    pData = this->writeParameters(layerID, WEIGHTGRP, modID, nWMod, pData);
  }


  //write biases for nFilters at modID 0xFF
  std::cout << "FPGAIORegs::writeFCLayer: " << layer.name <<  " layerID " << layerID << " biases " << *(layer.biases.data()) << std::endl;
  this->writeParameters(layerID, BIASGRP, 0xFF, 
			layer.nBiases, layer.biases.data());

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
      uint16_t modID = i + f*nChannels;
      std::cout << "FPGAIORegs::writeCnvLayer: " << layer.name
		<< " filter " << f << " channel " << i
		<< " layerID " << layerID << " modID " << modID 
		<< hex << " first weight address " << pData
		<< dec << " first weight "  << *pData << std::endl;
      //write nwMod Conv weights for this module
      pData=this->writeParameters(layerID, WEIGHTGRP, modID, nWMod, pData);
    }
  }
    
  //write biases for nFilters at modID 0xFFFF
  std::cout << "FPGAIORegs::writeCnvLayer: " << layer.name <<  " layerID " << layerID << " biases " << *(layer.biases.data()) << std::endl;
  this->writeParameters(layerID, BIASGRP, 0xFF, 
			layer.nBiases, layer.biases.data());

  return true;
}


bool
FPGAIORegs::writeImgBatch(const ImageBatch_t& imgs) const {
  int i=0;
  for (auto img: imgs) {
    std::cout << "FPGAIORegs::writeImgBatch: image #" << i++
	      << " of size " << img.size() << std::endl;
    this->writeData(img.size(), img.data());
  }
  return true;
}


bool
FPGAIORegs::readResults(Results_t& res) const {
  for (size_t i=0; i<res.size(); ++i) {
    for (size_t p=0; p<res[i].size(); ++p) {
      *p_IRes_addr = (WRITTENMASK) | ((p+i*res[i].size())<<16);
      std::cout << "FPGAIORegs::readResults: Read from 0x"
		<<  std::hex << *p_IRes_addr << std::dec << std::endl; 
      int32_t r = (*p_ORes_addr)&0xFFFF;
      if (r&0x8000) r=r-0x10000;
      res[i][p]=r;
      std::cout << "FPGAIORegs::readResults: Image=" << i 
		<< " prob for " << p << " =" << res[i][p] << std::endl; 
    }
  }

  return true;
}

int
FPGAIORegs::openDevMem() {
  if ( (m_fd = open( "/dev/mem", ( O_RDWR | O_SYNC) ) ) == -1 ) { 
    std::cerr << "FPGAIORegs::openMMapFile ERROR: could not open " + m_mmapFilePath;
  }
  return m_fd;
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
  if (m_mmapFilePath == "/dev/mem") openDevMem();
  else openMMapFile();
  if ( m_fd == -1 ) throw std::runtime_error("could not open mmap file");
  
  p_virtual_base = mmap( NULL, HW_REGS_SPAN, ( PROT_READ | PROT_WRITE ), MAP_SHARED, m_fd, HW_REGS_BASE );
  
  if( p_virtual_base == MAP_FAILED ) {
    printf( "ERROR: mmap() failed...\n" );
    close( m_fd );
  }
  p_IParms_addr = calcRegAddress(IPARMS_PIO_BASE);
  p_IAddr_addr  = calcRegAddress(IADDR_PIO_BASE);
  p_IImg_addr  = calcRegAddress(IIMG_PIO_BASE);
  p_ORes_addr  = calcRegAddress(ORES_PIO_BASE);
  p_IRes_addr  = calcRegAddress(IRES_PIO_BASE);
  std::cout << "FPGAIORegs::FPGAIORegs addresses " << std::hex
	    << p_IParms_addr << " and " << p_IAddr_addr << " in virtual space "
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
