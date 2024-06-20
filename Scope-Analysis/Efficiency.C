//  Calculate the efficiency for quartz scintillators
//
//  root[] Efficiency(filename);
//    where filename is the scope data file
//
// Include the necessary ROOT stuff
#include <stdio.h>
#include <TH1F.h>
#include <TH2F.h>
#include <TFile.h>
#include <TCanvas.h>
#include <string>
#include <sstream>

#define STRING_BUFF 256  

// Include the file to handle the scope data file
#include "ScopeDataFile.h"

#define NCH N_SCOPE_CHANNELS

TCanvas *Efficiency(string filelist, float qThresh=.1) {

// Thresholds for each channel (2nd channel is the quartz)
float threshold[NCH] = {.02,.1,.02,.03};
threshold[1] = qThresh;  // Set the threshold

// Time windows
float tMin[NCH] = {-100.,-100.,-100.,-100.};
float tMax[NCH] = {0.,0.,0.,0.};

//Voltage windows
float vMin[NCH] = {-.5,-5.,-.5,-.5};
float vMax[NCH] = {0.,0.,0.,0.};

// In time timing cuts
float tWin[NCH][2] = {{-40.,-30.},{-50.,-40.},{-40.,-30.},{-46.,-36.}};

// Construct the output file names
char rootFile[STRING_BUFF];
char printFile[STRING_BUFF];

// Pull out the first filename in the list
stringstream tmp(filelist);
string filename;
tmp>>filename;
snprintf(rootFile,STRING_BUFF,"%s.root",filename.c_str());
snprintf(printFile,STRING_BUFF,"%s.pdf",filename.c_str());

TFile out(rootFile,"RECREATE");  // Output file. Histograms will be stored here

// Book a bunch of histograms. 
// Peak times
TH1F *t[NCH];
TH1F *p[NCH];
TH1F *tMaxP[NCH];
TH1F *pMaxP[NCH];

char buf1[STRING_BUFF],buf2[STRING_BUFF];

// Book histograms
for(int i=0; i<NCH; i++) {
  snprintf(buf1,STRING_BUFF,"t%d",i+1);
  snprintf(buf2,STRING_BUFF,"Peak Times, Channel %d",i+1);
  t[i] = new TH1F(buf1,buf2,100,tMin[i],tMax[i]);
  snprintf(buf1,STRING_BUFF,"p%d",i+1);
  snprintf(buf2,STRING_BUFF,"Peak Heights, Channel %d",i+1);
  p[i] = new TH1F(buf1,buf2,100,vMin[i],vMax[i]);
  snprintf(buf1,STRING_BUFF,"tMaxP%d",i+1);
  snprintf(buf2,STRING_BUFF,"Max Peak Time, Channel %d",i+1);
  tMaxP[i] = new TH1F(buf1,buf2,100,tMin[i],tMax[i]);
  snprintf(buf1,STRING_BUFF,"pMaxP%d",i+1);
  snprintf(buf2,STRING_BUFF,"Max Peak Height, Channel %d",i+1);
  pMaxP[i] = new TH1F(buf1,buf2,100,vMin[i],vMax[i]);
}  

// Open the scope file




// Set up the storage space for the record and the peak list
ScopeRecord r;
PeakList list;

// Loop over records in file

int nRec=0;

int nFullRec=0;
int nTrace=0;  // Number of traces for one trigger
int nScint=0;  // Number of scintillators with pulse in window
int nQuartzPeak=0;  //Number of quartz peaks for triple coincidence
int nQuartzExtra=0;  //Number of quartz peaks for triple coincidence
int nQuartz=0; // Whether or not the Quartz has a hit in Window
int nTriple=0; // Number of triple scintillator coincidences
int nQuadruple=0;  // Number with triple + quartz


char tmpbuff[256];
strcpy(tmpbuff,filelist.c_str());
char *pch;


pch = strtok(tmpbuff,",");

while(pch) {
  filename = pch;
  cout << "Processing file "<<filename<<"..."<<endl;
  ScopeDataFile f(filename);

  while(f.getRecord(r)) {
    int iPnt = r.channel-1;
    // Reset all counters on first channel
    if(r.channel==1) {
      nTrace=nScint=nQuartz=0;
    }

    // Find the peaks
    int nPeaks = f.findPeaks(threshold[iPnt],r,list);
    if(r.channel==2) nQuartzPeak=nPeaks;   
    
    int iMax=-1;
    float max=0;
    
    for(int i = 0; i<nPeaks ; i++ ) {
       t[iPnt]->Fill(list.time[i]);    // Fill Time histogram
       p[iPnt]->Fill(list.peak[i]);    // Fill Peak histogram
       if(fabs(list.peak[i])>fabs(max)) {
          iMax=i;
          max = list.peak[i];
       }
// There seems to be a bug that finds spurious peaks at the end.  Put in kludgy fix
       if((r.channel==2)&&(list.time[i]>3000.)) nQuartzPeak--;       
       
    }
// Do analysis on maximum peak
    if(iMax>=0) {
      tMaxP[iPnt]->Fill(list.time[iMax]);
      pMaxP[iPnt]->Fill(list.peak[iMax]);
      if((list.time[iMax]>tWin[iPnt][0])&&(list.time[iMax]<tWin[iPnt][1])) {
        if(r.channel==2) {
          nQuartz++;
        } else {
          nScint++;
        }
      }
    }
    nRec++;
    nTrace++;
// Check the coincidences on the 4th channel
    if(r.channel==4) {
       // Should be 4 traces, or something is wrong
       if(nTrace!=4) {
         cerr << "Wrong number of traces found: "<<nTrace<<endl;
       } else {
         nFullRec++;
         //  All three scintillators hit?
         if(nScint==3) {
           nTriple++;
           if(nQuartz==1) { 
              nQuadruple++;  // Quartz hit, too?
              nQuartzExtra += (nQuartzPeak-1);
           }
         }
       } 
    }
  }
  pch = strtok(NULL,",");
}

cout << "Analyzed "<<nRec<<" records. Found:"<<endl;
cout << "   Full Triggers: "<<nFullRec<<endl;
cout << "   Triple Scintillator Coincidences: "<<nTriple<<endl;
cout << "   Triple Scint. + Quartz: "<<nQuadruple<<" ("<<nQuadruple*100./nTriple<<"%)"<<endl;
cout << "   Average number of extra quartz peaks: "<<(float) nQuartzExtra/nQuadruple<<endl;
cout << "   Mean Signal Height: "<<pMaxP[1]->GetMean()<<endl;

// Plot histograms
TCanvas *c = new TCanvas();
// 2x2 plots
c->Divide(4,4);

// Plot 1 histogram in each of the 4 quadrants
for(int i=0;i<NCH;i++) {
c->cd(1+i);
t[i]->Draw();
c->cd(5+i);
p[i]->Draw();
c->cd(9+i);
tMaxP[i]->Draw();
c->cd(13+i);
pMaxP[i]->Draw();
}

c->Print(printFile,"pdf");

out.Write();
out.Close();

return(c);
}  






