The Codebase contains a copy of NS3 (Network Simulator-3) and my implementation of the BeeAdHoc Routing Algo from the BeeAdHoc paper done in ns3. Inside the scratch folder, the beeadhoc folder contains the following files
- BeeAdHoc.pdf       -> A pdf of the BeeAdHoc Routing Algorithm Research Paper which is publicly available
- bee-adhoc.h        -> Header file containing the Definitions
- bee-adhoc.cc       -> File containing the Implementations
- bee-adhoc-sim.cc   -> A simulation files with tweakable Parameters, that simulates BeeAdhoc and Aodv in a Fanet and provides the stats
- fanet-aodv-bee.cc  -> similarly does simulation with Bee and Aodv but for a Fanet.

NS3 Version: 3.45

The Codebase has many issues and bugs that lead to unwanted behaviour and Incorrect Results. 
Anyone willing to use my current codebase needs to understand and fix said bugs in order to get proper results in simulations.
