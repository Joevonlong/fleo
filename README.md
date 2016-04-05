### What is this repository for? ###

* FLEO, "Flow-Level Extension to OMNeT++"

### How do I get set up? ###

* Summary of set up
    * Install OMNeT++ from https://omnetpp.org/
    * Download this repository into a subfolder in OMNeT++'s samples folder. eg. omnetpp-4.6/samples/fleo/
    * In this subfolder, run "opp_makemake -f --deep" and then "make"
    * The executable created defaults to the subfolder's name (eg. "fleo" using the same example)
* Traffic model
    * Place YouTube and Daum video metadata from http://an.kaist.ac.kr/traces/IMC2007.html in datasets/tubes/
    * run py/parse_yt_daum.py
* Convert *Rocketfuel* topologies into .ned format
    * Place ISP Maps (rocketfuel_maps_cch.tar.gz) from http://research.cs.washington.edu/networking/rocketfuel/ in datasets/rocketfuel
    * run py/read_rf.py

### Who do I talk to? ###

* z3123175 AT student DOT unsw.edu.au