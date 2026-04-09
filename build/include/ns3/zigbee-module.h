#ifdef NS3_MODULE_COMPILATION 
    error "Do not include ns3 module aggregator headers from other modules these are meant only for end user scripts." 
#endif 
#ifndef NS3_MODULE_ZIGBEE
    // Module headers: 
    #include <ns3/zigbee-helper.h>
    #include <ns3/zigbee-stack-container.h>
    #include <ns3/zigbee-nwk-fields.h>
    #include <ns3/zigbee-nwk.h>
    #include <ns3/zigbee-stack.h>
    #include <ns3/zigbee-nwk-header.h>
    #include <ns3/zigbee-nwk-payload-header.h>
    #include <ns3/zigbee-nwk-tables.h>
#endif 