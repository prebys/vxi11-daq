// Program to talk to scope
// Expects environment variable VXI_IP to be set to scope IP
//
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "vxi11.h" 
#include "vxi11_xdr.c" 
#include "vxi11_clnt.c"



#define MAX_BUFF 100000
#define MODE_ASCII 0
#define MODE_BINARY 1

// headers for VXI utilities
int vxi_init(char *);
int vxi_write_command(char *);
int vxi_read_response(char *,int);
int vxi_check_status();
int vxi_close();

CLIENT *VXI11Client;
Create_LinkParms MyCreate_LinkParms; 
Create_LinkResp *MyCreate_LinkResp; 
Device_Link MyLink; 
Device_WriteParms MyDevice_WriteParms; 
Device_WriteResp *MyDevice_WriteResp; 
Device_ReadParms MyDevice_ReadParms; 
Device_ReadResp *MyDevice_ReadResp; 
Device_GenericParms MyDevice_GenericParms;
Device_ReadStbResp *MyDevice_ReadStbResp;


char vxi_ip[256];

int main(int argc,char *argv[]) {

char command[256]="*IDN?\n";
int command_len;
int query=0;
int mode = MODE_ASCII;



// Get IP address from environmental variable
if(!getenv("VXI_IP")) {
  printf("VXI IP address not defined. Please define environmental variable VXI_IP.\n");
  exit(-1);
} else {
  sprintf(vxi_ip,"%s",getenv("VXI_IP"));
}


if(argc>1) sprintf(command,"%s\n",argv[1]);

command_len = strlen(command);
if(command[command_len-2]=='?') {
   query =1;
   printf("Sending query %s",command);
} else {
   printf("Sending command %s",command);
}

if(argc>2) mode = MODE_BINARY;

vxi_init(vxi_ip);

vxi_write_command(command);





// Only read it out if the command ends in a ?
if(query==1) {
  char DataRead[MAX_BUFF]; 

  int nRead;
  nRead=vxi_read_response(DataRead,MAX_BUFF);
 
     if(mode==MODE_ASCII) {
       DataRead[nRead]=0; 
       printf("Instrument responds (%d bytes): %s\n",nRead,DataRead);
     } else {
       printf("Dumping %d points:",MyDevice_ReadResp->data.data_len);
       int i;
       for(i=0;i<nRead;i++) { 
         unsigned char datword = (unsigned char) DataRead[i];
         printf("byte[%d]=%d\n",i,datword);
     }
  } 
}

vxi_close();

}

int vxi_init(char *vxi_ip) {
   printf("Initializing connection to VXI_IP: %s\n",vxi_ip);
   if((VXI11Client=clnt_create(vxi_ip,DEVICE_CORE,DEVICE_CORE_VERSION,"tcp"))
       ==NULL){ 
       printf("Error creating client\n") ;
       return(0);
   }



   MyCreate_LinkParms.clientId = 0; // Not used 
   MyCreate_LinkParms.lockDevice = 0; // No exclusive access 
   MyCreate_LinkParms.lock_timeout = 0; 
   MyCreate_LinkParms.device = "inst0"; // Logical device name 
   if((MyCreate_LinkResp=create_link_1(&MyCreate_LinkParms,VXI11Client))==NULL) {
      /* Do error handling here */
       printf("Error creating link\n") ;
       return(0);
    }
    MyLink = MyCreate_LinkResp->lid; // Save link ID for further use 
    MyDevice_ReadParms.lid = MyLink; 
    return(1);
} 
int vxi_write_command(char *command) {
   MyDevice_WriteParms.lid = MyLink; 
   MyDevice_WriteParms.io_timeout = 10000; // in ms 
   MyDevice_WriteParms.lock_timeout = 10000; // in ms 
   MyDevice_WriteParms.flags = 0; 
   MyDevice_WriteParms.data.data_val = command; 
   MyDevice_WriteParms.data.data_len = strlen(command); 

   if((MyDevice_WriteResp=device_write_1(&MyDevice_WriteParms,VXI11Client))
     ==NULL) { 
     printf("Error writing\n");
     return(1);
   }
}
int vxi_read_response(char *buff,int max_length){
  MyDevice_ReadParms.requestSize = max_length; 
  MyDevice_ReadParms.io_timeout = 10000; 
  MyDevice_ReadParms.lock_timeout = 10000;
  MyDevice_ReadParms.flags = 0;
  MyDevice_ReadParms.termChar = '\n'; 
// Sometimes data comes out in multiple chunks

  int nRead = 0;
  do {
     if((MyDevice_ReadResp=device_read_1(&MyDevice_ReadParms,VXI11Client))==NULL) {
        printf("Error reading\n");
        return(0);
     }
     strncpy(buff+nRead,MyDevice_ReadResp->data.data_val,MyDevice_ReadResp->data.data_len);
     nRead += MyDevice_ReadResp->data.data_len;
  } while(MyDevice_ReadResp->data.data_val[MyDevice_ReadResp->data.data_len-1]!=MyDevice_ReadParms.termChar);
  return(nRead);
}
int check_status(){
  MyDevice_GenericParms.lid=MyLink; 
  MyDevice_GenericParms.io_timeout = 10000; 
  MyDevice_GenericParms.lock_timeout = 10000;
  MyDevice_GenericParms.flags = 0;
  if((MyDevice_ReadStbResp=device_readstb_1(&MyDevice_GenericParms,VXI11Client))==NULL) {
      printf("Error reading status\n");
      return(0);
  }
  printf("Status byte: %x (%x)\n",MyDevice_ReadStbResp-> error,MyDevice_ReadStbResp->stb);
  return(1);
}

int vxi_close() {
   if(destroy_link_1(&MyLink,VXI11Client)==NULL) {
    /* Do error handling here */ 
      printf("Error closing VXI connection\n");
      return(0);
   }

   clnt_destroy(VXI11Client);
   return(1);

}

