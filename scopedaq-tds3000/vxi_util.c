// 
// VXI handling utilities
//

#include "vxi11.h" 
#include "vxi11_xdr.c" 
#include "vxi11_clnt.c"

CLIENT *VXI11Client;
Create_LinkParms MyCreate_LinkParms; 
Create_LinkResp *MyCreate_LinkResp; 
Device_Link MyLink; 
Device_WriteParms MyDevice_WriteParms; 
Device_WriteResp *MyDevice_WriteResp; 
Device_ReadParms MyDevice_ReadParms; 
Device_ReadResp *MyDevice_ReadResp; 



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


  if((MyDevice_ReadResp=device_read_1(&MyDevice_ReadParms,VXI11Client))==NULL) {
      printf("Error reading\n");
      return(0);
  } 
  int nRead = MyDevice_ReadResp->data.data_len;
  
  strncpy(buff,MyDevice_ReadResp->data.data_val,nRead); 
  return(nRead);
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
