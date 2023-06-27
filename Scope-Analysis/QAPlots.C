// Make the quality assurance plots for data taken by scopedaq program
//

#include <stdlib.h>
#include <string>
#include <TCanvas.h>

using namespace std;

TCanvas *plotChannels(string fileName,int nPlot=5, int nSkip=0, float tMin=0., float tMax=0., float vMin=0., float vMax=0.); 
TCanvas *plotTimes(string fileName,int maxRec=0,double threshold=0);

void QAPlots(string fileName,int maxRec=0) {
  plotChannels(fileName);
  plotTimes(fileName,maxRec);
}

int main(int argc, char *argv[]) {
   string fileName;
   if(argc>1) { 
     fileName=argv[1];
   } else { 
     fileName = "test.dat";
   }
   
   int maxRec=0;
   if(argc>2) maxRec=atoi(argv[2]);

   plotChannels(fileName);
   plotTimes(fileName,maxRec);
   return(0);
   
}

