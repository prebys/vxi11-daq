
#ifndef __SCOPEDATAFILE_H
#define __SCOPEDATAFILE_H

#include <iostream>
#include <fstream>
#include <string>
#include <vector>

using namespace std;

#define N_SCOPE_CHANNELS 4

struct ScopeRecord {
  string subHeader;  // Left over from LabView file.  Not used now.
  int channel;  
  double triggerTime;
  double timeStamp;
  int nData;
  //
  // For historical reasons, will keep raw data and unpacked data
  //
  vector<double> data;
  vector<double> voltage;
};



// List of peaks.  Have to do it a really stupid way because
// of CINT bug
struct PeakList {
  double channel;
  double pedestal;
  double threshold;
  vector<double> time;
  vector<double> peak;
  vector<double> area;
  vector<double> TOT;
};

class ScopeDataFile {
  public:
     int scopedaqVersion = -1;           // Version of the scopedaq program
     int ASCIIMode=0;                    // Set to one for ASCII readout
     const static int SCOPE_TYPE_TDS3000=0;     // TDS3000 series scope
     const static int SCOPE_TYPE_MSO7000=1;     // MSO7000 series scope
     int scopeType = -1;                          // Must determine the scope type
                                                   // From the header
                                                   
     bool recordSizeWarning=false;       // Set to true if record size wrong.
     
     int POINTS_PER_SEGMENT=-1;  // Will be initialized on the first channel
     // information about the scope setup
     int dataWidth=1;           // bytes per word
     
     double voltsPerCount[N_SCOPE_CHANNELS];  // Vertical calibration
     double minVoltage[N_SCOPE_CHANNELS],maxVoltage[N_SCOPE_CHANNELS];
     
     double offset[N_SCOPE_CHANNELS];         // Vertical Offset
     double nsPerCount;                         // Horizontal calibration
     double timeOffset;                       // Horizontal offset
     double minTime,maxTime;
          
     bool debugFlag;
     
     ScopeDataFile(string fileName, int recSize=0);
     int getRecordSize();
     string getHeader();     
     ~ScopeDataFile();
     bool findScopedaqVersion(ifstream *);
     bool findASCIIMode(ifstream *);
     bool findScopeType(ifstream *);
     bool findSegmentLength(ifstream *);
     bool findDataWidth(ifstream *);
     bool decodeHeader();
     bool getRecord(ScopeRecord &);
     int findPeaks(double threshold, ScopeRecord &rec, PeakList &list);
     double findPedestal(ScopeRecord &rec);
     void checkShortRecord(char *, ifstream *, ScopeRecord &);
  private:
     ifstream *f;
     int recordSize;
     string header;    // Appears at the beginning of the file
     string subHeader; //Appear throughout the file
};


#endif
