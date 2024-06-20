//  This routine will make a bunch or example histograms to show how to use
//  The scope analysis routines
//  Usage:  
//
//  root[] Examples(filename,channel,threshold, tMin,tMax,vMin,vMax);
//    where filename is the scope data file
//          channel is the channel number (1-4)
//          threshold is the threshold for the peak detector, in volts
//          vMin,vMax are the min and max voltages of the signal histograms
//          tMin,tMax is the time range, in ns
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


TCanvas *Examples(string filelist,int channel=1,double threshold=.01, float tMin=-200.,float tMax=200., float vMin=-.5,float vMax=.5) {

// Construct the output file names
char rootFile[STRING_BUFF];
char printFile[STRING_BUFF];
// Pull out the first filename in the list
stringstream tmp(filelist);
string filename;
tmp>>filename;
snprintf(rootFile,STRING_BUFF,"%s-chan%d.root",filename.c_str(),channel);
snprintf(printFile,STRING_BUFF,"%s-chan%d.pdf",filename.c_str(),channel);

TFile *out = new TFile(rootFile,"RECREATE");  // Output file. Histograms will be stored here

// Book a bunch of histograms.  

char title[STRING_BUFF];
// Signal height histogram
snprintf(title,STRING_BUFF,"Channel %d Signals",channel);
TH1F *s = new TH1F("s",title,256,vMin,vMax);


// Number of peaks
snprintf(title,STRING_BUFF,"Channel %d Number of peaks",channel);
TH1F *n = new TH1F("n",title,100,0.,100.);

// Peak Times
snprintf(title,STRING_BUFF,"Channel %d Peak Times",channel);
TH1F *t = new TH1F("t",title,100,tMin,tMax);

// Peak Heights
snprintf(title,STRING_BUFF,"Channel %d Peak Heights",channel);
TH1F *p = new TH1F("p",title,100,vMin,vMax);


snprintf(title,STRING_BUFF,"Channel %d Time of maximum peak",channel);
TH1F *tMaxP = new TH1F("tMaxP",title, 100, tMin, tMax);

snprintf(title,STRING_BUFF,"Channel %d Peak Height of Maximum peak",channel);
TH1F *pMaxP = new TH1F("pMaxP",title, 100, vMin, vMax);

snprintf(title,STRING_BUFF,"Channel %d Time of other peaks",channel);
TH1F *t2 = new TH1F("t2",title,100,tMin,tMax);

snprintf(title,STRING_BUFF,"Channel %d Pulse height of other peaks",channel);
TH1F *p2 = new TH1F("p2",title,100,vMin,vMax);

// Set up the storage space for the record and the peak list
ScopeRecord r;
PeakList list;

int nRec=0;

// Loop over files int the file list
char tmpbuff[STRING_BUFF];
strcpy(tmpbuff,filelist.c_str());
char *pch;


pch = strtok(tmpbuff,",");

while(pch) {
// Open the scope file
  filename = pch;
  cout << "Processing file "<<filename<<"..."<<endl;

  ScopeDataFile f(filename);


// Loop over records in file


  while(f.getRecord(r)) {
    // Only look at records corresponding to the channel we're interested in
    if(r.channel != channel) continue;
    nRec++;

    for (int i = 0; i<r.nData ; i++) {
       double v = r.voltage[i];   // value
       s->Fill(v);
       
    }
    // Find the peaks
    int nPeaks = f.findPeaks(threshold,r,list);
    
    // Fill nPeak histogram
    n->Fill(nPeaks);
    
    // Loop over peaks and fill other two histograms
    
    int iMax=-1;
    float max=0;
    
    for(int i = 0; i<nPeaks ; i++ ) {
       t->Fill(list.time[i]);    // Fill Time histogram
       p->Fill(-list.peak[i]);    // Fill Peak histogram
       if(fabs(list.peak[i])>fabs(max)) {
          iMax=i;
          max = list.peak[i];
       }
       
       
    }
    if(iMax>=0) {
      tMaxP->Fill(list.time[iMax]);
      pMaxP->Fill(-list.peak[iMax]);
    }
    for(int i=0; i<nPeaks; i++ ) {
      if(i==iMax) continue;
       t2->Fill(list.time[i]);
       p2->Fill(-list.peak[i]);  
    }   
  }
  pch = strtok(NULL,",");
}
cout << "Analyzed "<<nRec<<" records."<<endl;

// Plot histograms
TCanvas *c = new TCanvas();
// 2x2 plots
c->Divide(2,4);

// Plot 1 histogram in each of the 4 quadrants
c->cd(1);
s->Draw();
c->cd(2);
n->Draw();
c->cd(3);
t->Draw();
c->cd(4);
p->Draw();
c->cd(5);
tMaxP->Draw();
c->cd(6);
pMaxP->Draw();
c->cd(7);
t2->Draw();
c->cd(8);
p2->Draw();

c->Print(printFile,"pdf");

out->Write();
out->Close();

return(c);
}  






