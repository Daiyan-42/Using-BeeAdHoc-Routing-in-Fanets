#ifdef NS3_MODULE_COMPILATION 
    error "Do not include ns3 module aggregator headers from other modules these are meant only for end user scripts." 
#endif 
#ifndef NS3_MODULE_SIP
    // Module headers: 
    #include <ns3/sip-agent.h>
    #include <ns3/sip-element.h>
    #include <ns3/sip-header.h>
    #include <ns3/sip-proxy.h>
    #include <ns3/sip-transaction.h>
#endif 