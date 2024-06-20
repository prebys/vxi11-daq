// File to read out the data file created by the Mu2e test beam data acquisition scope
// 
// Usage:
// #include "ScopeDataFile.h"
// ScopeRecord r;
// ScopeDataFile f(fileName);
// while(f.getRecord(r)) {
//    for(int i=0; i<r.nData; i++) {
//       double x = r.data[i];  // Raw data (0-255)
//       double t = f.minTime + i*f.nsPerCount;  // time in ns (rel to trigger)
//       double v = r.voltage[i];
//    }
//  }
//
// Change log:
//   EjP Oct. 9, 2013 - Added routines to decode header
//   EjP Oct. 10, 2013 - Fix first record problem. Properly decode the header length
//                       Read convert signals to voltage based on scope data.
//   EjP Oct. 12, 2013 - Fixed problem with open mode that caused it to fail under
//                       Windows.  
//   EjP Oct. 22, 2013 - Made fully C++ compatible, for use with compiled version.
//   EjP Jun. 29, 2015 - Cleaned up to be compatible with fussier Root 6 compiler
//   EjP Jul. 9,  2015 - Make compatible with both the Agilent and the Tektronix scopes.
//                       Also, set up to deal with segment sizes other than 1000
//   EjP Jul. 13, 2015 - Subheader code was causing trouble with TDS3000 data.  Removed
//                       it.
//   EjP Jul. 15, 2015 - Lots of stuff to deal with ASCII files and header info.
//   EjP Aug. 19, 2015 - Handles the fact that TDS3054B scope truncates long records
//                       to 4096.
//   EjP Jan.  4, 2017 - Fixed it to handle 16 bit data from TDS3000

#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <sstream>
#include <math.h>

using namespace std;


#include "ScopeDataFile.h"



ScopeDataFile::ScopeDataFile(string fileName, int recSize) {

  f = new ifstream(fileName.c_str(),ios_base::binary);
  if(!findScopedaqVersion(f)) return;  // version of scopedaq  
  if(!findScopeType(f)) return;        // find scope type
  if(!findASCIIMode(f)) return;        // ascii readout mode
  if(!findSegmentLength(f)) return;    // find segment length
  if(!findDataWidth(f)) return;   	   // find data width

  getline(*f,header);
  subHeader = "";
  recordSize = recSize;
  debugFlag = false;
  decodeHeader();

}

ScopeDataFile::~ScopeDataFile(){
 f->close();
}

bool ScopeDataFile::getRecord(ScopeRecord &rec) {
   string buff;


//Trigger time
     if(!getline(*f,buff,',')) return(false);
     
     // take care of occasional spurious <cr>  
     if(buff.length()<2) {  
       if(!getline(*f,buff,',')) return(false);
     }


// Sometimes instead of the next record, it finds a new timestamp header
//  Obsoleted
//     if(buff.length()>24) {
//        cout << "Found subheader..."<<endl;
//        istringstream iss(buff);
//        getline(iss,subHeader);
//        getline(iss,buff,',');
//     }
     rec.subHeader = subHeader;  // legacy.  Set to blank now.
// Do sanity checks on length before converting
     if(buff.length()<20) {
        rec.triggerTime = atof(buff.c_str());
     } else {
        rec.triggerTime = -1.;
     }
    if(debugFlag) cerr<<"Trigger time: "<<rec.triggerTime<<endl;   
   
   //Channel #
     if(!getline(*f,buff,',')) return(false);
     
     
     if(buff.length()>3&&buff.length()<20) {
       rec.channel=atoi(buff.substr(3).c_str());
     } else {
       rec.channel=-1;
     }
     if(rec.channel<1||rec.channel>4) {
       cerr << "Bad channel "<<rec.channel<<" found. Ignore record."<<endl;
     }


   //Timestamp
   if(!getline(*f,buff,',')) return(false);
   
   if(buff.length()<50) {
     rec.timeStamp = atof(buff.c_str());
   } else {
     rec.timeStamp = -1.;
   }
   if(debugFlag) cerr<<"Timestamp: "<<rec.timeStamp<<endl;

   // Now read out the data record.  Now we try to properly
   // decode the scope record.
   rec.data.clear();
   rec.voltage.clear();
   
	// Load into a floating vector
	double slope = voltsPerCount[rec.channel-1];
    double offs = offset[rec.channel-1];

// Binary mode   
    if(ASCIIMode==0) {
	   char recHeader[20];
	//   char* retval = f->read(recHeader,3);
	   if(!f->read(recHeader,3)) return(false);
	   recHeader[3]='\0';
	   if(recHeader[1]!='#') {
		 cerr << "Block data format incorrect"<<endl;
		 return(false);
	   }
	   int nDig=(int) recHeader[2] - 0x30;
	//   cout << "nDigits: "<<nDig<<endl;
	//   retval = f->read(recHeader,nDig);
	   if(!f->read(recHeader,nDig)) return(false);
	   recHeader[nDig]='\0';
	//   cout << "Header: "<<recHeader<<endl;
	   rec.nData = atoi(recHeader);
	   if(rec.nData!=POINTS_PER_SEGMENT) {
		  cerr<<"Warning: data record size ("<<rec.nData<<") does not equal segment size ("
			<< POINTS_PER_SEGMENT <<")"<<endl;
	   }

	// The Agilent scope uses unsigned bytes, while the Tektronix scope uses signed bytes
	   unsigned char *ub = (unsigned char*) new char[rec.nData+2];  // little buffer at end, just in case
	   char *b = (char *) ub;
	//   retval = f->read((char *) ub, rec.nData);
	   if(!f->read((char *) ub, rec.nData)) return(false);
	//   cout << "Number of bytes read: "<<f->gcount()<<endl;
	//   b[rec.nData] = '\0';
	   checkShortRecord(b,f,rec);
	   // flush the end of line
	   getline(*f,buff);
	//   cout << "flush length: "<<buff.size()<<endl;
	

   
	   for(int i = 0; i<rec.nData; i++ ) {
		   double x,v;
		   if(scopeType==SCOPE_TYPE_TDS3000&&scopedaqVersion<20150714) {
			 x = (double) b[i];
			 v = x*slope + offs;
		   } else {
			 x =(double) ub[i];
			 v = (x-127.)*slope + offs;
		   }
		   rec.data.push_back(x);
		   rec.voltage.push_back(v); 
		}
	// Cleanup
	   if(ub!=NULL) delete ub;
   
// Else do ASCII mode   
   }  else {
       double x,v;
       // records.  All separated by commas but the last
       rec.nData=0;
       string sbuff;
       getline(*f,sbuff);
       stringstream *ss = new stringstream(sbuff);
       
       for(int i = 0; i<POINTS_PER_SEGMENT;i++) {
          getline(*ss,buff,',');
          if((buff=="")||(buff==" ")||(buff=="  ")) break;
          rec.nData++;
          x = (double) atoi(buff.c_str());
          v = x*slope + offs;
		  rec.data.push_back(x);
		  rec.voltage.push_back(v); 
		  if(ss->eof()) break;
       }
       if((rec.nData!=POINTS_PER_SEGMENT)&&!recordSizeWarning) {

         cerr << "Expected "<<POINTS_PER_SEGMENT<<" points.  Only found "<<rec.nData<<". ";
         cerr << "Further warnings will be suppressed."<<endl;
         recordSizeWarning=true;
       }     
   }   
   
   return(true);
}
//
// Determine the scopedaq version.  This only started with version
// 20150714
//
bool ScopeDataFile::findScopedaqVersion(ifstream *f) {
   string header;
   cout<< "Determining scopedaq version...";
   getline(*f,header);
   f->seekg(0, ios::beg);   // Rewind for subsequent operations
   int iversion=header.find("version=");
   if (iversion<0) {
    scopedaqVersion = -1;
    cout<<"file before version 20150714."<<endl;
    } else {
    string tmp = header.substr(iversion+sizeof("version=")-1,8);
    scopedaqVersion = atoi(tmp.c_str());
    cout <<"found version: "<<scopedaqVersion<<endl;
    }
    return(true);
}
// 
// Determine if scope is being read out in ASCII mode
//
bool ScopeDataFile::findASCIIMode(ifstream *f) {
   cout<< "Determining ASCII mode...";
   ASCIIMode=0;
   if(scopedaqVersion>=20150715) {
     string header;
     getline(*f,header);
     f->seekg(0, ios::beg);   // Rewind for subsequent operations
     if(header.find("ASCIIMode=1")>0) ASCIIMode=1;
   }
   cout << "ASCIIMode: "<<ASCIIMode<<endl;
   return(true);
}

// 
// Determine scope type.  For recent versions, this is encoded in the
// header. For older versions, recognizes
// Tektronix 3000 and Agilent 7000 based on whether it
// has CH1 or CHAN1 in the header, respectively
bool ScopeDataFile::findScopeType(ifstream *f) {
   string header;
   cout<< "Determining scope type...";
   if(scopedaqVersion>=20150715) {
      getline(*f,header);
      f->seekg(0, ios::beg);   // Rewind for subsequent operations
      if(header.find("scopeType=TDS3000")>0) {
         cout << "Found type TDS3000"<<endl;
         scopeType = SCOPE_TYPE_TDS3000;
         return(true);
      } else if (header.find("scopeType=MSO7000")>0){
         cout << "Found type MSO7000"<<endl;
         scopeType = SCOPE_TYPE_MSO7000;
         return(true);
      } else {
        cout << "Unable to determine scope type. EXITING"<<endl;
        return(false);
      }
    } else {
// For older versions, use a kludgier method
	   getline(*f,header,'#');
	   f->seekg(0, ios::beg);   // Rewind for subsequent operations

	   int i3000 = header.find(":CH1");
	   int i7000 = header.find(":CHAN1");
	   if(i3000>0) {
		 cout << "Found type TDS3000"<<endl;
		 scopeType = SCOPE_TYPE_TDS3000;
		 return(true);
	   } else if (i7000>0) {
		 cout << "Found type MSO7000"<<endl;
		 scopeType = SCOPE_TYPE_MSO7000;
		 return(true);
	   } else {
		 cout << "Unable to determine scope type. EXITING"<<endl;
		 return(false);
	   }
   }
}
// Get the record length.  New versions encode it in the header.  Older versions
//  have to find the first data record
bool ScopeDataFile::findSegmentLength(ifstream *f) {
   cout<< "Decoding data segment size...";
   // Look in header for newer version
   if(scopedaqVersion>=20150715) {
      string header;
      getline(*f,header);
      f->seekg(0, ios::beg);   // Rewind for subsequent operations
      int iseg = header.find("recordSize=");
      if(iseg<0) {
        cout << "ERROR: could not find segment size."<<endl;
        return(false);
      } else {
        header = header.substr(iseg+sizeof("recordSize=")-1);
        header = header.substr(0,header.find(","));
        POINTS_PER_SEGMENT = atoi(header.c_str());
	    cout << "found POINTS_PER_SEGMENT = "<<POINTS_PER_SEGMENT<<endl;
        return(true);
      }
   // much kludgier for older versions.
   } else {
	   string dummy;
	   getline(*f,dummy,'#');  //Read up to the first hashtag
	   char recHeader[20];
	   if(!f->read(recHeader,1)) return(false);
	   int nDig=(int) recHeader[0] - 0x30;
	   if(!f->read(recHeader,nDig)) return(false);
	   recHeader[nDig]='\0';
	   POINTS_PER_SEGMENT = atoi(recHeader);
	   cout << "found POINTS_PER_SEGMENT = "<<POINTS_PER_SEGMENT<<endl;
	   // Rewind file and start again
	   f->seekg(0, ios::beg); 

	   return(true);
   }
}
// Get the scope data datawidth
bool ScopeDataFile::findDataWidth(ifstream *f) {
   cout << "Determining data width...";
   if(scopedaqVersion>=20150715) {
      string header;
      getline(*f,header);
      f->seekg(0, ios::beg);   // Rewind for subsequent operations
      int iseg = header.find("dataWidth=");
      if(iseg<0) {
        cout << "ERROR: could not find data width in header."<<endl;
        return(false);
      } else {
        header = header.substr(iseg+sizeof("dataWidth=")-1);
        header = header.substr(0,header.find(","));
        dataWidth = atoi(header.c_str());
	    cout << "found dataWidth = "<<dataWidth<<"byte(s)"<<endl;
        return(true);
      }
   // much kludgier for older versions.
   } else {
        dataWidth=1;
        cout << "setting dataWidth = 1 byte"<<endl;
        return(true);
   }
   
}
// Decode the header
bool ScopeDataFile::decodeHeader() {
   // Get the range and offset
   char buff[1000];
   string subs, val;
   
         
   cout << "Scope setup: "<<endl;

   float rangeCounts=256.;
   if(dataWidth==2) rangeCounts=65536.;
  
   if(scopeType==SCOPE_TYPE_TDS3000) {
	   for(int i=0;i<N_SCOPE_CHANNELS;i++) {
	  
		  snprintf(buff,size(buff),":CH%d",i+1);
		  string chanString = header.substr(header.find(buff)+strlen(buff));
		  subs = chanString.substr(chanString.find("SCALE")+strlen("SCALE"));
		  val = subs.substr(0,subs.find(";"));
		  voltsPerCount[i] = 8.*atof(val.c_str())/rangeCounts;
		  cout << "Channel "<<i+1<<" Scale:" <<val<<"V/div ("
			  <<voltsPerCount[i]<<" V/count)"<< endl;
		  subs = chanString.substr(chanString.find("POSITION")+strlen("POSITION"));
		  val = subs.substr(0,subs.find(";"));
		  cout << "Channel "<<i+1<<" Position:" <<val<<endl;
		  offset[i] = -atof(val.c_str())*voltsPerCount[i]*(rangeCounts/8.);  //convert from divisions
		                                                        //to volts	  
		  subs = chanString.substr(chanString.find("OFFSET")+strlen("OFFSET"));
		  val = subs.substr(0,subs.find(";"));
		  offset[i] += atof(val.c_str());
		  cout << "Channel "<<i+1<<" Offset:" <<val<<" (total: "<<offset[i]<<"V)"<<endl;
		  minVoltage[i] = -(rangeCounts/2)*voltsPerCount[i]+offset[i];
		  maxVoltage[i] = minVoltage[i] + rangeCounts*voltsPerCount[i];
	   }
   } else {
	   for(int i=0;i<N_SCOPE_CHANNELS;i++) {
	  
		  snprintf(buff,size(buff),":CHAN%d:RANG",i+1);
		  subs = header.substr(header.find(buff)+strlen(buff));
		  val = subs.substr(0,subs.find(";"));
		  voltsPerCount[i] = atof(val.c_str())/rangeCounts;
		  cout << "Channel "<<i+1<<" Range:" <<val<<"V ("
			  <<voltsPerCount[i]<<" V/count)"<< endl;
		  subs = subs.substr(subs.find("OFFS")+strlen("OFFS"));
		  val = subs.substr(0,subs.find(";"));
		  cout << "Channel "<<i+1<<" Offset:" <<val<<"V"<<endl;
		  offset[i] = atof(val.c_str());
		  minVoltage[i] = -(rangeCounts/2)*voltsPerCount[i]+offset[i];
		  maxVoltage[i] = minVoltage[i] + rangeCounts*voltsPerCount[i];
	  }
   
   }
   // Get the time range
   if(scopeType==SCOPE_TYPE_TDS3000) {
   // Find the time scale
	   string timeString = header.substr(header.find("HORIZONTAL:MAIN"));
	   subs = timeString.substr(timeString.find("SCALE")+strlen("SCALE"));
	   val = subs.substr(0,subs.find(";"));
	   nsPerCount = 1e9*atof(val.c_str())*10/POINTS_PER_SEGMENT;  // correct to per bin
	   cout << "Timing Scale: " <<val<<"s/div ("<<nsPerCount<<" ns/bin)"<<endl;
   // Find the time offset	 
       timeString = header.substr(header.find("HORIZONTAL:DELAY"));  
	   val = timeString.substr(timeString.find("TIME")+strlen("TIME"));
	   cout << "Trigger Delay" << val;
	   timeOffset = atof(val.c_str())*1e9;
	   cout << " (Time Offset = "<<timeOffset<<" ns)"<<endl;
	   minTime = -POINTS_PER_SEGMENT/2*nsPerCount + timeOffset;
	   maxTime = minTime + POINTS_PER_SEGMENT*nsPerCount;  
   } else {
	   string timeString = header.substr(header.find(":TIM:"));
	   subs = timeString.substr(timeString.find("RANG")+strlen("RANG"));
	   val = subs.substr(0,subs.find(";"));
	   nsPerCount = 1e9*atof(val.c_str())/POINTS_PER_SEGMENT;
	   cout << "Timing Range: "<<val<<"s ("<<nsPerCount<<" ns/bin)"<<endl;
	   subs = timeString.substr(timeString.find("REF")+strlen("REF"));
	   val = subs.substr(0,subs.find(";"));
	   cout << "Time reference" << val;
	   timeOffset = 0.;
	   if((int) val.find("LEFT")>0) {
		 timeOffset = .4*POINTS_PER_SEGMENT*nsPerCount;
		} else if ((int) val.find("RIGH")>0) {
		 timeOffset = -.4*POINTS_PER_SEGMENT*nsPerCount;
		}
	   subs = timeString.substr(timeString.find("POS")+strlen("POS"));
	   val = subs.substr(0,subs.find(";"));
	   cout <<val<<"s ";
	   timeOffset += atof(val.c_str())*1e9;  // Total offset in NS
	   cout << " (Time Offset = "<<timeOffset<<" ns)"<<endl;
	   minTime = -POINTS_PER_SEGMENT/2*nsPerCount + timeOffset;
	   maxTime = minTime + POINTS_PER_SEGMENT*nsPerCount;  
   
   }
   return(true);

}
// Determine the pedestal 
double ScopeDataFile::findPedestal(ScopeRecord &rec) {
   int hist[256];
   
// Easier to find pedestal using raw counts, and then convert to voltage
// at the end
//
   for(int i=0;i<256;i++) hist[i]=0;
   
// Count frequency of bin values (only look at the good part of the record)
   for(int i=0;i<rec.nData;i++) {
     int bin = (int) rec.data[i];
     if(bin<0||bin>255) continue;
       hist[bin]++;
    }
    
    double pedestal=0.;
    int max=hist[0];
    for(int i=1;i<256;i++) {
      if(hist[i]>max) {
        max=hist[i];
        pedestal=(double) i;
      }
    }
    
    // Convert to voltage. Different for the two types of scopes
    if((ASCIIMode==1)||(scopeType==SCOPE_TYPE_TDS3000&&scopedaqVersion<20150714)) {
       pedestal = (pedestal)*voltsPerCount[rec.channel-1] + offset[rec.channel-1];
	} else {
       pedestal = (pedestal-127)*voltsPerCount[rec.channel-1] + offset[rec.channel-1];
	}
    return(pedestal);
}
// Find peaks
int ScopeDataFile::findPeaks(double threshold, ScopeRecord &rec, PeakList &list) {

  int polarity = 1;
  if(offset[rec.channel-1]<0.) polarity = -1;
  double pedestal = findPedestal(rec); 
  
  list.pedestal = pedestal;
  list.threshold = threshold;
  list.channel = rec.channel;
  list.time.clear();
  list.peak.clear();
  list.TOT.clear();
  list.area.clear();
  
  bool inPeak=false;
  // Record has some junk at the beginning and the end.  Only look at the good part
  for(int i=0;i<rec.nData;i++) {
    double t = minTime+i*nsPerCount;
    double signal = rec.voltage[i]-pedestal;
    if(signal*polarity>threshold) {
      if(!inPeak) {  //If this is the beginning of a peak, then create a new peak
         list.time.push_back(t);
         list.peak.push_back(signal);
         list.area.push_back(signal*nsPerCount);
         list.TOT.push_back(nsPerCount);
         inPeak = true;
      } else {  //Else, we're already in a peak, so we update the record
         int ip = list.time.size()-1;
         if(fabs(signal)>fabs(list.peak[ip])) list.peak[ip]=signal;
         list.TOT[ip]+=nsPerCount;
         list.area[ip]+=signal*nsPerCount;
      }
    } else {
      inPeak=false;
    }   
  }
  return(list.time.size());
}

int ScopeDataFile::getRecordSize() {
  return(recordSize);
}
string ScopeDataFile::getHeader() {
  return(header);
}
//  Very kludgy.   Look for short records and correct position in 
// data stream accordingly.
void ScopeDataFile::checkShortRecord(char *b, ifstream *f,ScopeRecord &rec) {
   // have to be careful here, because there could be zeroes in the 
   // record.
   char testString[] = ", ch";
   int ifound=-1;
   for(unsigned long i = 0; i<(rec.nData-strlen(testString)); i++) {
      if(strncmp(testString,b+i,strlen(testString))==0) {
         ifound=i;
         break;
      }
   }
   if(ifound<0) return;
   cout << "Short record found...";
   int iEnd = -1;
   for(int i = ifound; i>=0 ; i--) {
      if(b[i]=='\n') {
        iEnd = i;
        break;
      }
    }
    cout << "Channel:"<<rec.channel<<". Expected: "<<rec.nData<<", found: "<<iEnd<<endl;
    // Correct position in file
    f->seekg(iEnd-rec.nData,ios_base::cur);
}