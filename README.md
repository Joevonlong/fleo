### What is this repository for? ###

* FLEO, "Flow-Level Extension to OMNeT++"

### How do I get set up? ###

* Summary of set up
    * Install OMNeT++ from https://omnetpp.org/
    * Download this repository into a subfolder in OMNeT++'s samples folder. eg. ~/omnetpp-4.6/samples/fleo/
    * In this subfolder, run "opp_makemake -f --deep" and then "make"
    * The executable created defaults to the subfolder's name (eg. "fleo" using the same example)
* Traffic model
    * Download YouTube and Daum video metadata from http://an.kaist.ac.kr/traces/IMC2007.html into datasets/tubes/
    * run py/parse_yt_daum.py in its directory, eg.
        * cd ~/omnetpp-4.6/samples/fleo/py
        * python parse_yt_daum.py
* Convert *Rocketfuel* topologies into .ned format
    * Download "ISP Maps" (rocketfuel_maps_cch.tar.gz) and "Backbone topologies annotated with inferred weights and link latencies" (weights-dist.tar.gz) from http://research.cs.washington.edu/networking/rocketfuel/ into datasets/rocketfuel
    * run py/read_rf.py in its directory, eg.
        * cd ~/omnetpp-4.6/samples/fleo/py
        * python read_rf.py

### Who do I talk to? ###

* z3123175 AT student DOT unsw.edu.au