These are some simple programs to talk to a Tektronix scope over a VXI interface
Test on MacOS and Linux (Fermilab Scientific Linux).  Currently generates some
warnings on MacOS that can be safely ignored.

Before running programs, environmental variable VXI_IP should be set to the
(physical) IP address of the scope.


Contents:
  talkscope.cpp - simple program to send GPIB commands to the scope
  scopedaq.cpp - data acquisition program for scope.  Described in header
  scopedaq.cfg - configuration file for scope.  List of GPIP commands to override 
                 defaults
  Makefile - make programs
  README.txt - this file
  
Building and usage:

  > make         ! This will use rpcgen to make vxi11.h and then build
                 ! scopedaq and talkscope.  Makes a couple of other files
                 ! that arent needed
  > make clean   ! clean up everything to start over               
  (define VXI_IP to be the IP address of scope), then
  
  > talkscope (command) to test
  > scopedaq to take data
  

Change log:

2-Jun-2015 E. Prebys  	Cleaned up and updated from original version.  Added VXI_IP
                      	to replace hardcoding.
                      
20-JUN-2024 E. Prebys 	Modified to use the VXI11 library at 
						https://github.com/Lew-Engineering/libvxi11
                                       