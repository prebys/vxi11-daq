//
// Program to do data acquisition from a Tektronix TDS 3000 series scope
// using VXI11 interface.  It will run on Linux or MacOS.
//
// It uses the VXI11 library found at
//   https://github.com/Lew-Engineering/libvxi11
//
// which must be installed prior to compiling it.
//
// Usage:
//   Set environmental variable VXI_IP to the scope IP address and
//   start with
//   >scopedaq (datafile)
//   Stop with <cntl>-c
//
//  If (datafile) is omitted, it will default to 'scopedaq-(timestamp).dat'
//
//  The program will perform some basic configuration, then transmit the
//  contents of the file 'scopedaq.cfg' verbatim to the scope.
//  
//  If will create the data file and dump the entire scope configuration to the header.
//
//  It will then arm the scope, wait for a trigger, and read out all four channels
//  and write the data to the data file.
//
//  Version history
//  ---------------
//  Distant past  	EjP Lots of undocumented evolution over time.
//  20-JUN-2024		EjP	Converted to use Vxi11 library rather than home built stuff.
//
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include <signal.h>

#include "libvxi11.h" 

#define STRING_BUFF 256		// buffer size for character buffer
#define MAX_BUFF 100000		// buffer size for data. Big enough for ASCII readout
#define NCHAN 4				// number of channels

int recordSize=500;  //default recordsize.  Can be overridden by .cfg file
int dataWidth=1;     //detault data witdh.  Can be overridden by .cfg file

#define VERSION 20240620

char vxi_ip[STRING_BUFF];

int ASCIIMode=1;  // Set to true to acquire in ASCII (bigger files)



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
char timestamp[STRING_BUFF];
char filename[STRING_BUFF];

// Get IP address from environmental variable
if(!getenv("VXI_IP")) {
  printf("VXI IP address not defined. Please define environmental variable VXI_IP.\n");
  exit(-1);
} else {
  snprintf(vxi_ip,STRING_BUFF,"%s",getenv("VXI_IP"));
}

snprintf(timestamp,STRING_BUFF,"%ld",(long) time(NULL));

if(argc<2) {
  snprintf(filename,STRING_BUFF,"scopedaq-%s.dat",timestamp);
} else {
  snprintf(filename,STRING_BUFF,"%s",argv[1]);
}

FILE *configfile;

Vxi11 scope(vxi_ip);
scope.clear();


printf("Running DAQ.  Writing to data file: %s.\n Hit <cntl>-c to exit.\n",filename);

// Scope initialization commands
printf("Performing default setup...\n"); 
scope.printf("*RST\n");
int i;

// We're going to set everything to defaults and then read in a configuration
// for any special settings.
// Turn on channels and set ranges
for(i=0;i<NCHAN;i++) {
  snprintf(command,MAX_BUFF,"SEL:CH%d ON\n",i+1);      // Turns CH%d display on and selects CH%d
  scope.printf(command);
  snprintf(command,MAX_BUFF,"CH%d:VOL 1.00E-1\n",i+1); // Volts command sets units/div
  scope.printf(command);
  snprintf(command,MAX_BUFF,"CH%d:IMP FIF\n",i+1);     // Volts command sets units/div
  scope.printf(command);
  snprintf(command,MAX_BUFF,"CH%d:POS 3.0E+00\n",i+1); // Position on display
  scope.printf(command);
}

 scope.printf("HOR:MAI:SCA 5.00E-8\n");     //Scale command sets .1us/div
 

// Set up trigger
scope.printf("TRIG:A:EDG:SOU CH1\n");
// scope.printf("TRIG:A:LOGI:INPUT2:SLO FALL\n"); // Sets input 1 signal slope to falling
 scope.printf("TRIG:A:LEV -2.00E-1\n");

// Set up acquire mode
 scope.printf("ACQ:STOPA SEQ\n"); // One curve at a time
 if(ASCIIMode==1) {
   scope.printf("WFMPre:ENC ASCII\n"); // Sets waveform data in binary format
   scope.printf("DAT:ENCDG ASCII\n"); // Positive integer 0-255
 } else {
   printf("Binary mode currently not supported!");
   //scope.printf("WFMPre:ENC BIN\n"); // Sets waveform data in binary format
   //scope.printf("DAT:ENCDG RPBINARY\n"); // Positive integer 0-255
 }
 
 snprintf(command,STRING_BUFF,"DAT:WID %d\n",dataWidth);
 scope.printf(command); // Sets byte width

// set the default record size 
 snprintf(command,STRING_BUFF,"HOR:RECO %d\n",recordSize);
 scope.printf(command); // Length of record
 snprintf(command,STRING_BUFF,"WFMP:NR_P %d\n",recordSize); // Sets record length of reference waveform specified by DATA:DEST
 scope.printf(command);
 snprintf(command,STRING_BUFF,"DATA:STOP %d\n",recordSize);  // Set record length to read out
 scope.printf(command);

//////////////////////////////////////////////////////////////////////////
// Now look for a config file.  This will override anything already set.
//
if((configfile=fopen("scopedaq.cfg","r"))) {
  printf("Found configuration file.  Will send custom commands\n");

  while(fgets(command,sizeof(command),configfile)) {
    // See if it's a comment or a bare <cr>
    if(command[0]=='#'||command[0]=='!'||
        strlen(command)<2) continue;
      printf("Sending custom command: %s",command);
      scope.printf(command);
  }
}
fclose(configfile);

// Querty the record size and datawidth to see if they've changed 
scope.printf("HEADER 0\n");  //Turn off headers for a moment
scope.printf("HOR:RECO?\n");
scope.read(data,MAX_BUFF);
nRead = strlen(data);
recordSize = atoi(data);
printf("Scope set to record length; %d\n",recordSize);
scope.printf("DAT:WID?\n");
scope.read(data,MAX_BUFF);
nRead = strlen(data);
dataWidth = atoi(data);
printf("Scope set to data width: %d\n",dataWidth);


// Open the data file and write the header
FILE *datafile = fopen(filename,"w");
// To keep this compatible with the scope program, we're going to write all the header stuff to one line
// Starting with a late version, add a bunch of header stuff
fprintf(datafile,"%s, version=%d, scopeType=TDS3000, recordSize=%d, ASCIIMode=%d, dataWidth=%d, ",timestamp,VERSION,recordSize,ASCIIMode,dataWidth);


// Now query everything and write it to the data file
scope.printf("HEADER 1\n");  //Return verbose headers
scope.printf("VERBOSE 1\n");

printf("Final configuration=============================================\n");
for(i=0;i<NCHAN;i++) {
  snprintf(command,STRING_BUFF,"CH%d?\n",i+1);
  scope.printf(command);
  scope.read(data,MAX_BUFF);
  nRead = strlen(data);
  printf("%s",data);
  data[strlen(data)-1] = '\0';
  fprintf(datafile,"%s, ",data);
}  
scope.printf("HOR?\n");
scope.read(data,MAX_BUFF);
nRead = strlen(data);
// For some weird reason, this line comes back with an extra carriage return and some extra information
for(i=0;i<strlen(data);i++) {
  if(data[i]=='\n') {
    data[i+1]='\0';
    break;
  }
}
printf("%s",data);
fprintf(datafile,"%s, ",data);
scope.printf("TRIG:A?\n");
scope.read(data,MAX_BUFF);
nRead = strlen(data);
printf("%s",data);
scope.printf("ACQ?\n");
scope.read(data,MAX_BUFF);
nRead = strlen(data);
printf("%s",data);

printf("==============================================================\n");

int nEvents=0;
int nRecords=0;

scope.printf("HEADER 0\n");  // Turn off headers, so BUSY? not
                                  // screwed up 

while(keepRunning) {
  printf("Starting DAQ...\n");
  scope.printf("ACQ:STATE RUN\n");

  snprintf(timestamp,STRING_BUFF,"%ld",(long) time(NULL));

  printf("Waiting for DAQ to stop...\n");
  int running;
  do {
    scope.printf("ACQ:STATE?\n"); //
    // printf("About to read state....");
    scope.read(data,MAX_BUFF);
    nRead = strlen(data);
    // printf("Readback: %s",data);
    running = atoi(data)&0x0001;  // Probably a better way to do this
  
  } while(running&&keepRunning);
  
  if(!keepRunning) break;

  printf("Run stopped.  Reading out data...\n");

  char timetag[STRING_BUFF];
   
  for(i=0;i<NCHAN;i++) {
    scope.printf("TIM?\n"); // Returns time
    scope.read(timetag,MAX_BUFF);
    nRead = strlen(data);
    // strip the <cr>
    timetag[strlen(timetag)-1] = '\0';
    // printf("timetag %s\n",timetag);
    // set the channel number
    snprintf(command,STRING_BUFF,"DAT:SOU CH%d\n",i+1);// Sets the location of the waveform data transferred by curve? query  
    scope.printf(command); 
    // Read in the data
    // printf("Reading out channel %d\n",i+1);
     
    scope.printf("CURV?\n");// Sends data from scope to ext device
    scope.read(data,MAX_BUFF);
    nRead = strlen(data);

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
scope.local();

}


