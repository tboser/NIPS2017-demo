/*
This program demonstrate how to use hps communicate with FPGA through light AXI Bridge.
uses should program the FPGA by GHRD project before executing the program
refer to user manual chapter 7 for details about the demo
*/
//nc -l -p 1234 > main.c && make clean ; make && ./HPS_FPGA_LED 
//nc -w 3 root@10.0.1.41:. 1234 < main.c

#include <stdio.h>
#include <stdlib.h>
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
  int CheckDoneBit(int n=100);

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
       

  para_wr_addr1  = (uint8_t *) virtual_base + ( ( unsigned long  )( ALT_LWFPGASLVS_OFST + REG_WR_PARA1_BASE)  & ( unsigned long)( HW_REGS_MASK ) );
  para_wr_addr2  = (uint8_t *) virtual_base + ( ( unsigned long  )( ALT_LWFPGASLVS_OFST + REG_WR_PARA2_BASE)  & ( unsigned long)( HW_REGS_MASK ) );
  image_wr_addr  = (uint8_t *) virtual_base + ( ( unsigned long  )( ALT_LWFPGASLVS_OFST + REG_WR_IMAGE_BASE)  & ( unsigned long)( HW_REGS_MASK ) );
  result_wr_addr = (uint8_t *) virtual_base + ( ( unsigned long  )( ALT_LWFPGASLVS_OFST + REG_WR_RESULT_BASE) & ( unsigned long)( HW_REGS_MASK ) );
  result_rd_addr = (uint8_t *) virtual_base + ( ( unsigned long  )( ALT_LWFPGASLVS_OFST + REG_RD_RESULT_BASE) & ( unsigned long)( HW_REGS_MASK ) );
  return 0;
}

int CNNHardwareInterface::WriteParameter(int layer, int group, int mod_num,int address, int data){
  //  if(data<0) data-=0x10000;
  *(uint32_t *) para_wr_addr2 = ((address&0xffff)<<16) | (data&0xffff);
  *(uint32_t *) para_wr_addr1 = (0<<31) | ((layer&0xff)<<16) | ((group&0xff)<<8) | (mod_num&0xff);
  *(uint32_t *) para_wr_addr1 = (1<<31) | ((layer&0xff)<<16) | ((group&0xff)<<8) | (mod_num&0xff);
  *(uint32_t *) para_wr_addr1 = (0<<31) | ((layer&0xff)<<16) | ((group&0xff)<<8) | (mod_num&0xff);
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
  *(uint32_t *) image_wr_addr = (0<<29) | ((28*28*im+28*py+px)<<16) | (v&0xffff);
  *(uint32_t *) image_wr_addr = (1<<29) | ((28*28*im+28*py+px)<<16) | (v&0xffff);
  *(uint32_t *) image_wr_addr = (0<<29) | ((28*28*im+28*py+px)<<16) | (v&0xffff);
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
  *(uint32_t *) para_wr_addr1 = (0<<31) | (6<<16) | (i&0xff);
  *(uint32_t *) para_wr_addr1 = (1<<31) | (6<<16) | (i&0xff);
  *(uint32_t *) para_wr_addr1 = (0<<31) | (6<<16) | (i&0xff);
  return 1;
}

int CNNHardwareInterface::CheckDoneBit(int n){
  int i;
  for(i=0;i<n;i++){
    int v = (int) (*(uint32_t *) result_rd_addr);
    if(v&(1<<31)){
      printf(" done...\n");
      return 1;
    }else if(v&(1<<30)){
      printf(" running ...\n");
      usleep(1);
    }else{
      printf(" not running or done ???\n");
      return 0;
    }
  }
  return 0;
}

int CNNHardwareInterface::ReadData(int n, int* data, bool print){
  int i;
  for(i=0;i<n;i++){
    *(uint32_t *) result_wr_addr = (1<<31) | i<<16;
    int raw = (int) (*(uint32_t *) result_rd_addr);
    int v = raw&0xffff;
    if(v&0x8000) v=v-0x10000;    
    if(data)  data[i] = v;
    if(print){
      printf("  %d)     %d  ",i,v);
      if(raw&(0x40000)) printf("  first row");
      if(raw&(0x10000)) printf("  first column");
      if(raw&(0x20000)) printf("  last row");
      if(raw&(0x80000)) printf("  last column");
      printf("\n");
    }
  }

  return 1;
}



int main(int argc, char* argv[]){

  int select_num = argc>1 ? atoi(argv[1]):0;

  CNNHardwareInterface* cnn = new CNNHardwareInterface();

  //WriteParameter(int layer, int group, int mod_num,int address, int data)

  cnn->Reset();
  cnn->SelectOutput(select_num);//13 output

  for(int i=0;i<6;i++){ //cnn->WriteParameters
    for(int j=0;j<25;j++){ //cnn->WriteParameters
      cnn->WriteParameter(0,0,i,j,j);//i%5+j);
    }	
    cnn->WriteParameter(0,0,i,25,8);//divide

    cnn->WriteParameter(0,1,i,0,1);//first layer bias
  }

////////////////////////////
//layer 2
  for(int f=0;f<6;f++){ //cnn->WriteParameters
   for(int s=0;s<16;s++){ //cnn->WriteParameters
    for(int j=0;j<25;j++){ //cnn->WriteParameters
      if(f==0&&s==0) cnn->WriteParameter(1,0,16*f+s,j,j/5);
      else           cnn->WriteParameter(1,0,16*f+s,j,0);
    }	
    cnn->WriteParameter(1,0,16*f+s,25,8);//divide
   }
  }

  for(int i=0;i<16;i++){ 
    cnn->WriteParameter(1,1,i,0,2);//divide power after sum (not in this version!)
  }

  for(int i=0;i<16;i++){ //cnn->WriteParameters
    cnn->WriteParameter(1,2,i,0,22);//bias layer 2
  }

  //fine till here...
  //here
  int loc=1;
  for(int i=0;i<256*120;i++) cnn->WriteParameter(2,0,0,i,1);//i==loc?1:0);//desne1 mult first dense layer
  cnn->WriteParameter(2,1,0,0,0);//desne1 divide after mult
  for(int i=0;i<120;i++){
    cnn->WriteParameter(2,2,0,i,-10);//desne1 biases
  }

  for(int i=0;i<120*84;i++) cnn->WriteParameter(3,0,0,i,i<120?1:0);//dense2 mult values
  cnn->WriteParameter(3,1,0,i,1);//dense2 divide by power
  for(int i=0;i<120;i++){
    cnn->WriteParameter(3,2,0,i,i);//dense2 bias second dense
  }

  for(int i=0;i<840;i++) cnn->WriteParameter(4,0,0,i,i<84?1:0);//dense2 mult values
  cnn->WriteParameter(4,1,0,1,1);//dense2 divide by power
  for(int i=0;i<10;i++){
    cnn->WriteParameter(4,2,0,i,10);//dense3 bias second dense
  }

  //write data
  for(int h=0;h<1;h++){
    for(int i=0;i<28;i++){
      for(int j=0;j<28;j++){
	cnn->SetImagePixel(h,i,j,28*j+i);
      }
    }
  }

  //when  0 => result_data_stream <= image_data_stream;
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

  cnn->Start();
  usleep(1000);

  cnn->CheckDoneBit();
  cnn->ReadData(10);

  delete cnn;

  return( 0 );
}
