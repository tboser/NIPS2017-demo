/*
This program demonstrate how to use hps communicate with FPGA through light AXI Bridge.
uses should program the FPGA by GHRD project before executing the program
refer to user manual chapter 7 for details about the demo
*/
//nc -l -p 1234 > main.c && make clean ; make && ./HPS_FPGA_LED 
//nc -w 3 root@10.0.1.41:. 1234 < main.c

#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include "hwlib.h"
#include "socal/socal.h"
#include "socal/hps.h"
#include "socal/alt_gpio.h"
//#include "hps_0.h"

#define HW_REGS_BASE ( ALT_STM_OFST )
#define HW_REGS_SPAN ( 0x04000000 )
#define HW_REGS_MASK ( HW_REGS_SPAN - 1 )

#define REG_WR_PARA1_BASE  0x3100
#define REG_WR_PARA2_BASE  0x3300
#define REG_WR_IMAGE_BASE  0x3400
#define REG_WR_RESULT_BASE 0x3500
#define REG_RD_RESULT_BASE 0x3200


#ifndef CNNHARDWAREINTERFACE_H
#define CNNHARDWAREINTERFACE_H

class CNNHardwareInterface{

 private:
  int fd;
  void *virtual_base;
  void *para_wr_addr1;
  void *para_wr_addr2;
  void *image_wr_addr;
  void *result_wr_addr;
  void *result_rd_addr;

  int Init();
 public:
  CNNHardwareInterface();
  ~CNNHardwareInterface();

  int WriteParameter(int layer, int group, int mod_num,int address, int data);
  int SetImagePixel(int im, int px, int py, int v);
  int Reset();
  int Start();
  int ReadData(int n, int* data=0, bool print=1);

  int SelectOutput(int i);
};

#endif


CNNHardwareInterface::CNNHardwareInterface(){Init();}

CNNHardwareInterface::~CNNHardwareInterface(){
  // clean up our memory mapping and exit
	
  if( munmap( virtual_base, HW_REGS_SPAN ) != 0 ) {
    printf( "ERROR: munmap() failed...\n" );
    close( fd );
  }

  close( fd );
}

int CNNHardwareInterface::Init(){
  if( ( fd = open( "/dev/mem", ( O_RDWR | O_SYNC ) ) ) == -1 ) {
    printf( "ERROR: could not open \"/dev/mem\"...\n" );
    return( 1 );
  }

  virtual_base = mmap( NULL, HW_REGS_SPAN, ( PROT_READ | PROT_WRITE ), MAP_SHARED, fd, HW_REGS_BASE );
  
  if( virtual_base == MAP_FAILED ) {
    printf( "ERROR: mmap() failed...\n" );
    close( fd );
    return( 1 );
  }
       

  para_wr_addr1  = virtual_base + ( ( unsigned long  )( ALT_LWFPGASLVS_OFST + REG_WR_PARA1_BASE)  & ( unsigned long)( HW_REGS_MASK ) );
  para_wr_addr2  = virtual_base + ( ( unsigned long  )( ALT_LWFPGASLVS_OFST + REG_WR_PARA2_BASE)  & ( unsigned long)( HW_REGS_MASK ) );
  image_wr_addr  = virtual_base + ( ( unsigned long  )( ALT_LWFPGASLVS_OFST + REG_WR_IMAGE_BASE)  & ( unsigned long)( HW_REGS_MASK ) );
  result_wr_addr = virtual_base + ( ( unsigned long  )( ALT_LWFPGASLVS_OFST + REG_WR_RESULT_BASE) & ( unsigned long)( HW_REGS_MASK ) );
  result_rd_addr = virtual_base + ( ( unsigned long  )( ALT_LWFPGASLVS_OFST + REG_RD_RESULT_BASE) & ( unsigned long)( HW_REGS_MASK ) );
  return 0;
}

int CNNHardwareInterface::WriteParameter(int layer, int group, int mod_num,int address, int data){
  //  if(data<0) data-=0x10000;
  *(uint32_t *) para_wr_addr1 = (1<<31) | ((layer&0xff)<<16) | ((group&0xff)<<8) | (mod_num&0xff);
  *(uint32_t *) para_wr_addr2 = ((address&0xffff)<<16) | (data&0xffff);
  *(uint32_t *) para_wr_addr1 = 0;

  return 1;
}

/*
int SetImagePixel(int p, int v){
  *(uint32_t *) image_wr_addr = (1<<29) | (p<<16) | (v&0xffff);
  return 0;
}
*/

int CNNHardwareInterface::SetImagePixel(int im, int px, int py, int v){
  *(uint32_t *) image_wr_addr = (1<<29) | ((28*28*im+28*py+px)<<16) | (v&0xffff);
  return 0;
}

int CNNHardwareInterface::Reset(){
  *(uint32_t *) image_wr_addr = (1<<31); //rst
  usleep(1);
  *(uint32_t *) image_wr_addr = 0; //clear rst
  return 1;
}

int CNNHardwareInterface::Start(){
  Reset();
  *(uint32_t *) image_wr_addr = 0; //start
  *(uint32_t *) image_wr_addr = (1<<30); //start
  *(uint32_t *) image_wr_addr = 0; //clear start
  return 1;
}

int CNNHardwareInterface::SelectOutput(int i){	//select output
  //  *(uint32_t *) para_wr_addr1 = (1<<31) | (6<<16) | (i&0xff);
  *(uint32_t *) para_wr_addr1 = (1<<31) | (6<<16) | (i&0xff);
  *(uint32_t *) para_wr_addr1 = (1<<31) | (6<<16) | (i&0xff);
  *(uint32_t *) para_wr_addr1 = 0;
  *(uint32_t *) para_wr_addr2 = 0;
  return 1;
}

int CNNHardwareInterface::ReadData(int n, int* data, bool print){
  int i;
  for(i=0;i<n;i++){
    *(uint32_t *) result_wr_addr = (1<<31) | i<<16;
    int v = (int) (*(uint32_t *) result_rd_addr)&0xffff;
    if(v&0x8000) v=v-0x10000;    
    if(data)  data[i] = v;
    if(print) printf("  %d)     %d     0x%x\n",i,v,(*(uint32_t *) result_rd_addr));
  }

  return 1;
}



int main(){



  CNNHardwareInterface* cnn = new CNNHardwareInterface();

  int i,j;
  //WriteParameter(int layer, int group, int mod_num,int address, int data)

  cnn->Reset();
  
  for(j=0;j<2;j++){ //cnn->WriteParameters
    for(i=0;i<25;i++){ //cnn->WriteParameters
      //cnn->WriteParameter(0,0,0,i,i+1);
      if(i%2) cnn->WriteParameter(j,0,0,i,-1);
      else cnn->WriteParameter(j,0,0,i,1);
      cnn->WriteParameter(j,0,0,i,i%5+j);
    }	
    cnn->WriteParameter(j,0,0,25,j);
  }

  cnn->WriteParameter(0,1,0,0,1);//bias
  cnn->WriteParameter(1,1,0,0,0);//divide power after sum
  cnn->WriteParameter(1,2,0,0,1);//bias layer 2
  
  int loc=20;
  for(i=0;i<256*120;i++) cnn->WriteParameter(2,0,0,i,i==loc?loc:0);//desne1 mult first dense layer
  cnn->WriteParameter(2,1,0,0,0);//desne1 divide after mult
  for(i=0;i<120;i++){
    cnn->WriteParameter(2,2,0,i,-10);//desne1 biases
  }

  for(i=0;i<120*84;i++) cnn->WriteParameter(3,0,0,i,i<120?1:0);//dense2 mult values
  cnn->WriteParameter(3,1,0,i,1);//dense2 divide by power
  for(i=0;i<120;i++){
    cnn->WriteParameter(3,2,0,i,i);//dense2 bias second dense
  }

  for(i=0;i<840;i++) cnn->WriteParameter(4,0,0,i,i<84?1:0);//dense2 mult values
  cnn->WriteParameter(4,1,0,1,1);//dense2 divide by power
  for(i=0;i<10;i++){
    cnn->WriteParameter(4,2,0,i,10);//dense3 bias second dense
  }

  //write data
  for(i=0;i<28;i++){
    for(j=0;j<28;j++){
      cnn->SetImagePixel(0,i,j,28*j+i+10);
    }
  }
  
  cnn->SelectOutput(8);

  cnn->Start();
  usleep(10000);

  cnn->ReadData(20);

  delete cnn;

  return( 0 );
}
