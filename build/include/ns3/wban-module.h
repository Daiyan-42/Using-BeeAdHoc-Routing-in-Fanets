#ifdef NS3_MODULE_COMPILATION 
    error "Do not include ns3 module aggregator headers from other modules these are meant only for end user scripts." 
#endif 
#ifndef NS3_MODULE_WBAN
    // Module headers: 
    #include <ns3/wban.h>
    #include <ns3/wban-helper.h>
    #include <ns3/wban-error-model.h>
    #include <ns3/wban-interference-helper.h>
    #include <ns3/wban-lqi-tag.h>
    #include <ns3/wban-spectrum-signal-parameters.h>
    #include <ns3/wban-spectrum-value-helper.h>
    #include <ns3/wban-phy-header.h>
    #include <ns3/wban-phy.h>
    #include <ns3/wban-net-device.h>
    #include <ns3/wban-propagation-model.h>
    #include <ns3/wban-channel.h>
#endif 