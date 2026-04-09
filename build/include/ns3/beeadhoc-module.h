#ifdef NS3_MODULE_COMPILATION 
    error "Do not include ns3 module aggregator headers from other modules these are meant only for end user scripts." 
#endif 
#ifndef NS3_MODULE_BEEADHOC
    // Module headers: 
    #include <ns3/beeadhoc-packet.h>
    #include <ns3/beeadhoc-rtable.h>
    #include <ns3/beeadhoc-routing-protocol.h>
    #include <ns3/beeadhoc-helper.h>
#endif 