
#include "ScopeDataFile.h"
#define DEFAULT_PEAK_THRESHOLD .05
#define NCHAN 4

#include <TCanvas.h>
#include <TH1F.h>

double beamWindow[2]={410.,500.};

TCanvas *plotTimes(string fileName,int maxRec=0,double threshold=0) {
    
    if(threshold<=0.) threshold = DEFAULT_PEAK_THRESHOLD;
    
    ScopeDataFile f(fileName);
    
    TH1F *hTime[NCHAN];
    TH1F *hNPeak[NCHAN];
    TH1F *hPeak[NCHAN];
    TH1F *hPeakInTime[NCHAN];
    
    for(int i=0; i<NCHAN; i++ ) {
        char name[128];
        char title[128];
        
        sprintf(name,"time%d",i+1);
        sprintf(title,"peak times, channel %d",i+1);
        hTime[i]=new TH1F(name,title,f.POINTS_PER_SEGMENT,f.minTime,f.maxTime);
        
        sprintf(name,"nPeak%d",i+1);
        sprintf(title,"Number of Peaks, channel %d",i+1);
        hNPeak[i]=new TH1F(name,title,100,0.,100.);
        
        sprintf(name,"peak%d",i+1);
        sprintf(title,"peak values, channel %d",i+1);
        hPeak[i]=new TH1F(name,title,256,f.minVoltage[i],f.maxVoltage[i]);
        
        sprintf(name,"peakInTime%d",i+1);
        sprintf(title,"peak values, in time with beam, channel %d",i+1);
        hPeakInTime[i]=new TH1F(name,title,256,f.minVoltage[i],f.maxVoltage[i]);
    }
    
    ScopeRecord r;
    PeakList list;
    int count=0;
    
    cout << "About to loop over records..."<<endl;
    
    while(f.getRecord(r)) {
        if(r.channel<1||r.channel>NCHAN) continue;
        
        int nPeaks = f.findPeaks(threshold,r,list);
        int indx = r.channel-1;
        
        hNPeak[indx]->Fill(nPeaks);
        
        for(int i=0;i<nPeaks;i++) {
            hTime[indx]->Fill(list.time[i]);
            hPeak[indx]->Fill(list.peak[i]);
            if(list.time[i]>beamWindow[0]&&list.time[i]<beamWindow[1]) {
                hPeakInTime[indx]->Fill(list.peak[i]);
            }
        }  
        count++;
        if(!(count%1000)) {
            cout<<"Processed "<<count<<" records"<<endl;
        }
        if(count==maxRec) {
            cout<<"Maximum number of records reached. Terminating"<<endl;
            break;
        }
    }
    TCanvas *c = new TCanvas("Peak Data");
    c->Divide(4,4);
    
    for(int iChan=0;iChan<NCHAN;iChan++ ) {
        c->cd(iChan+1);
        hNPeak[iChan]->Draw();
        c->cd(iChan+NCHAN+1);
        hTime[iChan]->Draw();
        c->cd(iChan+2*NCHAN+1);
        hPeak[iChan]->Draw();
        c->cd(iChan+3*NCHAN+1);
        hPeakInTime[iChan]->Draw();
    }
    
    
    string plotFileName = "times-"+fileName+".pdf";
    c->Print(plotFileName.c_str(),"pdf");
    // system(Form("open %s", plotFileName.c_str()));
    
    cout << "Analyzed "<<count<<" records"<<endl;
    
    return(c);
} 
