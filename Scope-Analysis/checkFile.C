//
// Simple program to check the header and count events in a scopedaq file.
//
#include "ScopeDataFile.h"
#define DEFAULT_PEAK_THRESHOLD .05
#define NCHAN 4


void checkFile(string fileName) {
    
   
    cout << endl <<endl<< "Reading header information for file: "<<fileName<<"..."<<endl<<endl;

    ScopeDataFile f(fileName);
    
    ScopeRecord r;
    int count=0;
    
    cout << endl<<endl<<"About to loop over records..."<<endl<<endl;
    
    while(f.getRecord(r)) {
        if(r.channel<1||r.channel>NCHAN) continue;
        
        count++;
        if(!(count%1000)) {
            cout<<"Processed "<<count<<" records"<<endl;
        }
    }
    
    cout << "Found "<<count<<" records ("<<count/4<<" events)"<<endl;
    
} 
