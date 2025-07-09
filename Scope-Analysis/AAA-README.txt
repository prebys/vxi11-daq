This directory contains a set of routine to analyze data take with either
an Agilent 7000 series scope or a Tektronix 3000 series scope.

It was originally written to analyze data taken with
the Agilent MSO7054B oscilloscope during the Mu2e test beam run in 2013.
This should be considered the first official release since the run, and there
are some incompatibilities with the versions used during the run. Specifically:

  - No longer compatible with data files taken with the LabView program.
  - Both the plotChannels routine and the plotTimes routine now work in ns and Volts,
    rather than bin numbers and raw counts.  
    
The main routine utilities to access the scope data files are encapsulated in the
ScopeDataFile class, defined by ScopeDataFile.C and ScopeDataFile.h. 

Rootlogon.C will load ScopeDataFile.C, plotChannels.C, plotTimes.C, 
QAPlots.C. Examples.C 
 
General access to the datafile is as follows:

#include "ScopeDataFile.h"
ScopeRecord r;
ScopeDataFile f(fileName);  // This will dump the scope setup info to screen
while(f.getRecord(r)) {
    int channel = r.channel;  // Channel number (1-4)
    for(int i=0; i<r.nData; i++) {
       double x = r.data[i];  // Raw data (0-255)
       double t = f.minTime + i*f.nsPerCount;  // time in ns (rel to trigger)
       double v = r.voltage[i]; // Calibrated data, in volts
    }
  }

You can read the headers for the use of each one, but to simply plot all 4 channels
for the first few events, do

root[] plotChannels("filename.dat")

The full form of this command is:
     plotChanneTCanvas *plotChannels(string fileName,int nPlot=5, int nSkip=0, 
          float tMin=0., float tMax=0.,float vMin=0., float vMax=0.) {
          
where: 
  - filename: data file name
  - nPlot: number of events to plot
  - nSkip: number of events to skip
// The next few variables all default to the scope limits if not specified
  - tMin, TMax:  time range (in ns)
  - vMin, vMax: voltage range (in V)

The ScopeDataFile object also contains a simple peak finding routine.  You can see an
example of its use in the plotTimes.C program.


Tested with MacOS, Linux, and Windows 7.

  
Release notes;

 Version    Date          Name       Comments
   1.0      10-OCT-2013   E.Prebys   Gave up compatibility with LabView files.
                                     Used header information to give proper times
                                        and voltages.
                                     Fixed problem that sometimes gave bad records.
   1.1      12-OCT-2013   E.Prebys   Fixed problem with file open that caused premature
                                     termination under windows.   Also, explicitly
                                     close the file in the destructor class.
                                     Files modified:
                                          ScopeDataFile.C
   1.2      25-OCT-2013   E.Prebys   Cleaned up and made compatible with compiled 
                                     running.  Example Makefile included.  See
                                     usage notes above.
   1.2.1    25-OCT-2013   E.Prebys   created rootlogon.C_nocompiler and 
                                     rootlogon.C_interpreted.
   1.2.2    29-JUN-2015   E.Prebys   Cleaned up ScopeDataFile.C to deal with fussier
                                     v6 compiler	
   2.0       9-JUL-2015   E.Prebys   Make compatible with both Tektronix 3000 and Agilent
                                     5000 scope.  Will automatically detect based on
                                     format of header
                                     
   2.1       9-JUL-2015   E.Prebys   Renamed directory to Scope-analysis, since this
                                     is now more general than the FTBF case. Also
                                     Added example script to show how to use routines. 
   2.2      14-JUL-2015   E.Prebys   implemented version readout for scopedaq.  Went
                                     to positive binary readout for TDS3000  
   2.3      15-JUL-2015   E.Prebys   Switched to ASCII file for TDS3000
   2.4      19-AUG-2015   E.Prebys   Handle the fact that the TDS3000 returns short
                                     records for recordsize=10000
   2.8      01-JUL-2016   E.Prebys   Made a bunch of minor changes to the routines to
                                     run in 2016 test beam.
   2.10	    19-DEC-2016   E.Prebys   Minor tweaks.  Added "checkFile.C" routine
   2.12     04-JAN-2017   E.Prebys   Fixed ScopeDataFile so it would handle 16 bit 
                                     data from TDS3000
   2.13     03-MAY-2023   E.Prebys	 Fixed a bug in plotChannels that made all the
                                     vertical scales the same as CH1.  Also got
                                     rid of the "compiled" and "interpreted" stuff,
                                     which is meaningless for root6.
   2.14     20-JUN-2024	  E.Prebys   Replaced all sprinf() with snprintf() to avoid a 
                                     bunch of warnings when loading.
                                                             
                                     



									 
   
