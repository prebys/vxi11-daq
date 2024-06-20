// Program to talk to scope
// Expects environment variable VXI_IP to be set to scope IP
//
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "libvxi11.h" 



#define MAX_BUFF 100000
#define STRING_BUFF 256
#define MODE_ASCII 0
#define MODE_BINARY 1


char vxi_ip[STRING_BUFF];

int main(int argc,char *argv[]) {

char command[STRING_BUFF]="*IDN?\n";
int command_len;
int query=0;
int mode = MODE_ASCII;



// Get IP address from environmental variable
if(!getenv("VXI_IP")) {
  printf("VXI IP address not defined. Please define environmental variable VXI_IP.\n");
  exit(-1);
} else {
  snprintf(vxi_ip,STRING_BUFF,"%s",getenv("VXI_IP"));
}


if(argc>1) snprintf(command,STRING_BUFF,"%s\n",argv[1]);

command_len = strlen(command);
if(command[command_len-2]=='?') {
   query =1;
   printf("Sending query %s",command);
} else {
   printf("Sending command %s",command);
}

if(argc>2) mode = MODE_BINARY;

Vxi11 device(vxi_ip);
device.clear();

device.printf(command);





// Only read it out if the command ends in a ?
if(query==1) {
  char DataRead[MAX_BUFF]; 

  int nRead;
  device.read(DataRead,MAX_BUFF);
  nRead=strlen(DataRead);
 
     if(mode==MODE_ASCII) {
       DataRead[nRead]=0; 
       printf("Instrument responds (%d bytes): %s\n",nRead,DataRead);
     } else {
       printf("Binary mode not currently supported");
       /* printf("Dumping %d points:",MyDevice_ReadResp->data.data_len);
       int i;
       for(i=0;i<nRead;i++) { 
         unsigned char datword = (unsigned char) DataRead[i];
         printf("byte[%d]=%d\n",i,datword); 
     } */
  } 
}

device.local();

}


