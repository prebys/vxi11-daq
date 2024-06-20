// Plot 10 traces on all 4 channels

#include "ScopeDataFile.h"
#include <TCanvas.h>
#include <TH1F.h>

#define NCHAN 4
#define MAXPLOT 10

#define STRING_BUFF 256


TCanvas *plotChannels(string fileName,int nPlot=5, int nSkip=0, float tMin=0., float tMax=0.,float vMin=0., float vMax=0.) {
    
    if(nPlot>MAXPLOT) {
        cerr << "Maximum number of plots per channel is "<<MAXPLOT<<endl;
        nPlot = MAXPLOT;
    }
    
    ScopeDataFile f(fileName);
    ScopeRecord rec;
    
    
    TH1F *h[NCHAN][MAXPLOT];
    
    TCanvas *c= new TCanvas(fileName.c_str());
    
    c->Divide(nPlot,NCHAN);
    
    int count[NCHAN];
    for(int i=0;i<NCHAN;i++) count[i]=0;
    
    int nRec = 0;
    
    int nDone=0;
    while(f.getRecord(rec)) {
        int chan = rec.channel;
        if(chan<1||chan>NCHAN) {
            cerr << "Bad record found. Skipping"<<endl;
            continue;
        }
        if(nRec==0) {
            cerr << "Record size: "<<rec.nData<<endl;
        }
        
        nRec++;
        if(nRec<=nSkip) continue;
        
        if(count[chan-1]==nPlot) continue;
        
        char name[STRING_BUFF],title[STRING_BUFF];
        snprintf(name,STRING_BUFF,"trace%d%d",chan,count[chan-1]);
        snprintf(title,STRING_BUFF,"Scope Trace, channel %d, trace %d",chan,count[chan-1]);
        int nBins=(tMax-tMin)/f.nsPerCount;
        // Use the scope range, unless limits are explicitly specified
        if(tMin>=tMax) {
          tMin = f.minTime;
          nBins=rec.nData;
          tMax = f.minTime+nBins*f.nsPerCount;
        } 
        float vLo = vMin;
        float vHi = vMax;
        if(vLo>=vHi) {
          vLo = f.minVoltage[chan-1];
          vHi = f.maxVoltage[chan-1];
        }
        TH1F *hist = h[chan-1][count[chan-1]] =  new TH1F(name,title,nBins,tMin,tMax);
        hist = h[chan-1][count[chan-1]];
        hist->SetMaximum(vHi);
		hist->SetMinimum(vLo);
		hist->GetYaxis()->SetTitle("Volatge (V)");
		hist->GetYaxis()->SetTitleOffset(1.5);
		hist->GetXaxis()->SetTitle("Time (ns)");
		hist->GetXaxis()->SetTitleOffset(1.5);
        
        int iStart=(tMin-f.minTime)/f.nsPerCount;
        for(int i=0;i<nBins;i++) {
            hist->SetBinContent(i+1,rec.voltage[i+iStart]);
        }
        
        c->cd((chan-1)*nPlot+count[chan-1]+1);
        
        hist->Draw();
        
        count[chan-1]++;
        if(count[chan-1]==nPlot) nDone++;
        if(nDone==NCHAN) break;
    }
    string plotFileName = "channels-"+fileName+".pdf";
    c->Print(plotFileName.c_str(),"pdf");
    // system(Form("open %s", plotFileName.c_str())); 
    return(c);
}
