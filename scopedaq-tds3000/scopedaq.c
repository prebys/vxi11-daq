//
// Very ugly program adapted form an example VXI program I found on the web.
// Reads out the Tektronix TDS 3000 series scope using VXI interface.
// Will set up scope to acquire on all 4 channels, triggered by 
// a NIM pulse to the rear input Then will read out segments and write them to 
// file.  Filename is automatically generated from the timestamp.
// Usage:
//   Set environmental variable VXI_IP to the scope IP address and
//   start with
//   >scopedaq
//   Stop with <cntl>-c
//

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include <signal.h>

#include "vxi11.h" 
#include "vxi11_xdr.c" 
#include "vxi11_clnt.c"


#define MAX_BUFF 100000
#define MODE_ASCII 0
#define MODE_BINARY 1
#define NCHAN 4

int recordSize=500;  //default recordsize.  Can be overridden by .cfg file
int dataWidth=1;     //detault data witdh.  Can be overridden by .cfg file

#define VERSION 20170104

char vxi_ip[256];

int ASCIIMode=1;  // Set to true to acquire in ASCII (bigger files)

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

int keepRunning=1;
void intHandler(int dum) {
  printf("Caught <cntl>-C.  Will exit gracefully...\n");
  keepRunning=0;
}



int main(int argc,char *argv[]) {

// catch cntrl-c to exit gracefully
signal(SIGINT,intHandler);

int nRead;

char command[MAX_BUFF];
char data[MAX_BUFF+1];  // Room for terminating null
char timestamp[128];
char filename[128];

// Get IP address from environmental variable
if(!getenv("VXI_IP")) {
  printf("VXI IP address not defined. Please define environmental variable VXI_IP.\n");
  exit(-1);
} else {
  sprintf(vxi_ip,"%s",getenv("VXI_IP"));
}

sprintf(timestamp,"%ld",(long) time(NULL));

if(argc<2) {
  sprintf(filename,"scopedaq-%s.dat",timestamp);
} else {
  sprintf(filename,"%s",argv[1]);
}

FILE *configfile;

vxi_init(vxi_ip);


printf("Running DAQ.  Writing to data file: %s.\n Hit <cntl>-c to exit.\n",filename);

// Scope initialization commands
printf("Performing default setup...\n"); 
vxi_write_command("*RST\n");
int i;

// We're going to set everything to defaults and then read in a configuration
// for any special settings.
// Turn on channels and set ranges
for(i=0;i<NCHAN;i++) {
  sprintf(command,"SEL:CH%d ON\n",i+1); // Turns CH%d display on and selects CH%d
  vxi_write_command(command);
  sprintf(command,"CH%d:VOL 1.00E-1\n",i+1); // Volts command sets units/div
  vxi_write_command(command);
sprintf(command,"CH%d:IMP FIF\n",i+1); // Volts command sets units/div
 vxi_write_command(command);
sprintf(command,"CH%d:POS 3.0E+00\n",i+1); // Position on display
//  vxi_write_command(command);
//sprintf(command,"CH%d:OFFS -3.00E-1\n",i+1); // Volts command sets units/div
  vxi_write_command(command);
}

 vxi_write_command("HOR:MAI:SCA 5.00E-8\n"); //Scale command sets .1us/div
 vxi_write_command("ACQ:STATE RUN\n"); //Runs scope, single seq by default
 vxi_write_command("HOR:DEL:STATE ON\n"); // Turns on time delay
 vxi_write_command("HOR:DEL:TIM -4.00E-8\n"); //Sets time delay


// Set up trigger
vxi_write_command("TRIG:A:EDG:SOU CH1\n");
// vxi_write_command("TRIG:A:LOGI:INPUT2:SLO FALL\n"); // Sets input 1 signal slope to falling
 vxi_write_command("TRIG:A:LEV -2.00E-1\n");

// Set up acquire mode
 vxi_write_command("ACQ:STOPA SEQ\n"); // One curve at a time
 if(ASCIIMode==1) {
   vxi_write_command("WFMPre:ENC ASCII\n"); // Sets waveform data in binary format
   vxi_write_command("DAT:ENCDG ASCII\n"); // Positive integer 0-255
 } else {
   vxi_write_command("WFMPre:ENC BIN\n"); // Sets waveform data in binary format
   vxi_write_command("DAT:ENCDG RPBINARY\n"); // Positive integer 0-255
 }
 
 sprintf(command,"DAT:WID %d\n",dataWidth);
 vxi_write_command(command); // Sets byte width

// set the default record size 
 sprintf(command,"HOR:RECO %d\n",recordSize);
 vxi_write_command(command); // Length of record
 sprintf(command,"WFMP:NR_P %d\n",recordSize); // Sets record length of reference waveform specified by DATA:DEST
 vxi_write_command(command);
 sprintf(command,"DATA:STOP %\n",recordSize);  // Set record length to read out
 vxi_write_command(command);

//
// Now look for a config file.  This will override anything already set.
//
if((configfile=fopen("scopedaq.cfg","r"))) {
  printf("Found configuration file.  Will send custom commands\n");

  while(fgets(command,sizeof(command),configfile)) {
    // See if it's a comment or a bare <cr>
    if(command[0]=='#'||command[0]=='!'||
        strlen(command)<2) continue;
      printf("Sending custom command: %s",command);
      vxi_write_command(command);
  }
}
fclose(configfile);

// Querty the record size and datawidth to see if they've changed 
vxi_write_command("HEADER 0\n");  //Turn off headers for a moment
vxi_write_command("HOR:RECO?\n");
nRead=vxi_read_response(data,MAX_BUFF);
recordSize = atoi(data);
printf("Scope set to record length; %d\n",recordSize);
vxi_write_command("DAT:WID?\n");
nRead=vxi_read_response(data,MAX_BUFF);
dataWidth = atoi(data);
printf("Scope set to data width: %d\n",dataWidth);


// Open the data file and write the header
FILE *datafile = fopen(filename,"w");
// To keep this compatible with the scope program, we're going to write all the header stuff to one line
// Starting with a late version, add a bunch of header stuff
fprintf(datafile,"%s, version=%d, scopeType=TDS3000, recordSize=%d, ASCIIMode=%d, dataWidth=%d, ",timestamp,VERSION,recordSize,ASCIIMode,dataWidth);


// Now query everything and write it to the data file
vxi_write_command("HEADER 1\n");  //Return verbose headers
vxi_write_command("VERBOSE 1\n");

printf("Final configuration=============================================\n");
for(i=0;i<NCHAN;i++) {
  sprintf(command,"CH%d?\n",i+1);
  vxi_write_command(command);
  nRead=vxi_read_response(data,MAX_BUFF);
  printf("%s",data);
  data[strlen(data)-1] = '\0';
  fprintf(datafile,"%s, ",data);
}  
vxi_write_command("HOR?\n");
nRead=vxi_read_response(data,MAX_BUFF);
// For some weird reason, this line comes back with an extra carriage return and some extra information
for(i=0;i<strlen(data);i++) {
  if(data[i]=='\n') {
    data[i+1]='\0';
    break;
  }
}
printf("%s",data);
fprintf(datafile,"%s, ",data);
vxi_write_command("TRIG:A?\n");
nRead=vxi_read_response(data,MAX_BUFF);
printf("%s",data);
vxi_write_command("ACQ?\n");
nRead=vxi_read_response(data,MAX_BUFF);
printf("%s",data);

printf("==============================================================\n");

int nEvents=0;
int nRecords=0;

vxi_write_command("HEADER 0\n");  // Turn off headers, so BUSY? not
                                  // screwed up 

while(keepRunning) {
  printf("Starting DAQ...\n");
  vxi_write_command("ACQ:STATE RUN\n");

  sprintf(timestamp,"%ld",(long) time(NULL));

  printf("Waiting for DAQ to stop...\n");
  int running;
  do {
    vxi_write_command("ACQ:STATE?\n"); //
    // printf("About to read state....");
    nRead=vxi_read_response(data,MAX_BUFF);
    // printf("Readback: %s",data);
    running = atoi(data)&0x0001;  // Probably a better way to do this
  
  } while(running&&keepRunning);
  
  if(!keepRunning) break;

  printf("Run stopped.  Reading out data...\n");

  char timetag[128];
   
  for(i=0;i<NCHAN;i++) {
    vxi_write_command("TIM?\n"); // Returns time
    nRead=vxi_read_response(timetag,MAX_BUFF);
    // strip the <cr>
    timetag[strlen(timetag)-1] = '\0';
    // printf("timetag %s\n",timetag);
    // set the channel number
    sprintf(command,"DAT:SOU CH%d\n",i+1);// Sets the location of the waveform data transferred by curve? query  
    vxi_write_command(command); 
    // Read in the data
    // printf("Reading out channel %d\n",i+1);
     
    vxi_write_command("CURV?\n");// Sends data from scope to ext device
    nRead=vxi_read_response(data,MAX_BUFF);

    fprintf(datafile,"%s, ch %d , %s , ",timetag,i+1,timestamp);
    printf("nRead returns: %d\n",nRead);
    data[nRead] = '\n';
    fwrite(data,nRead+1,1,datafile);
      
    nRecords++;

    /*
    printf("Dumping %d points:",nRead);
    int i;
    for(i=0;i<nRead;i++) { 
       unsigned char datword = (unsigned char) data[i];
       printf("byte[%d]=%d\n",i,datword);
    } 
    */
  }
  printf("Successfully read event: %d\n",++nEvents);
  
}  

printf("Wrote a total of %d events (%d records) to %s\n",nEvents,nRecords,filename);
     
fclose(datafile);
vxi_close();

}
// Initialize
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
     printf("Error writing: %x \n",MyDevice_WriteResp->error);
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
        printf("Error reading: %x , %x\n", MyDevice_ReadResp->error,MyDevice_ReadResp->reason);
        return(0);
     }
     if(MyDevice_ReadResp->data.data_len==0) {
       break;
     }
     strncpy(buff+nRead,MyDevice_ReadResp->data.data_val,MyDevice_ReadResp->data.data_len);
     nRead += MyDevice_ReadResp->data.data_len;
  } while(MyDevice_ReadResp->data.data_val[MyDevice_ReadResp->data.data_len-1]!=MyDevice_ReadParms.termChar);
//  } while(1==2);
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

