@echo off
set OMNETPP_ROOT=D:\Downloads\omnetpp-6.2.0-windows-x86_64\omnetpp-6.2.0
set PATH=%OMNETPP_ROOT%\bin;%PATH%
opp_makemake -f
make
