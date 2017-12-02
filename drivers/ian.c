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




void *para_wr_addr1,*para_wr_addr2,*image_wr_addr,*result_wr_addr,*result_rd_addr;

int write_parameter(int layer, int group, int mod_num,int address, int data){
  //  if(data<0) data-=0x10000;
  *(uint32_t *) para_wr_addr1 = (1<<31) | ((layer&0xff)<<16) | ((group&0xff)<<16) | (mod_num&0xff);
  *(uint32_t *) para_wr_addr2 = ((address&0xffff)<<16) | (data&0xffff);
  *(uint32_t *) para_wr_addr1 = 0;

  return 1;
}

int write_pixel(int p, int v){
  *(uint32_t *) image_wr_addr = (1<<29) | (p<<16) | (v&0xffff);
  return 0;
}

int rst(){
  *(uint32_t *) image_wr_addr = (1<<31); //rst
  usleep(1);
  *(uint32_t *) image_wr_addr = 0; //clear rst
  return 1;
}

int start(){
  *(uint32_t *) image_wr_addr = 0; //start
  *(uint32_t *) image_wr_addr = (1<<30); //start
  *(uint32_t *) image_wr_addr = 0; //clear start
  return 1;
}

int select_output(int i){	//select output
  *(uint32_t *) para_wr_addr1 = (1<<31) | (6<<16) | (i&0xff);
  *(uint32_t *) para_wr_addr1 = 0;
  *(uint32_t *) para_wr_addr2 = 0;
  return 1;
}

int read_data(int n){
  int i;
  for(i=0;i<n;i++){
    *(uint32_t *) result_wr_addr = (1<<31) | i;
    int v = (int) (*(uint32_t *) result_rd_addr)&0xffff;
    if(v&0x8000) v=v-0x10000;
    printf("  %d)     %d                  0x%x\n",i,v,(*(uint32_t *) result_rd_addr));
  }

  return 1;
}


int main(){

	void *virtual_base;
	int fd;

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




	int i;
	//write_parameter(int layer, int group, int mod_num,int address, int data)

	rst();
	for(i=0;i<25;i++){ //write_parameters
	  if(i%2) write_parameter(0,0,0,i,-1);
	  else write_parameter(0,0,0,i,1);
	}
	write_parameter(0,0,0,25,0);
	write_parameter(0,0,1,0,14);//bias

	//write data
	for(i=0;i<200;i++){
	  write_pixel(i,i%3?i:-i);
	}


	select_output(3);
	start();
	usleep(100);


	read_data(44);




	// clean up our memory mapping and exit
	
	if( munmap( virtual_base, HW_REGS_SPAN ) != 0 ) {
		printf( "ERROR: munmap() failed...\n" );
		close( fd );
		return( 1 );
	}

	close( fd );

	return( 0 );
}
