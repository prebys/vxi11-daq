//
// Load all the routines I need (interpreted version)
//
{
cout << "Executing logon script...";
gROOT->ProcessLine(".L ScopeDataFile.C");
gROOT->ProcessLine(".L plotChannels.C");
gROOT->ProcessLine(".L plotTimes.C");
gROOT->ProcessLine(".L QAPlots.C");
gROOT->ProcessLine(".L Examples.C");
gROOT->ProcessLine(".L Efficiency.C");
gROOT->ProcessLine(".L checkFile.C");
cout << "Done."<<endl;
}
