#ifdef NS3_MODULE_COMPILATION 
    error "Do not include ns3 module aggregator headers from other modules these are meant only for end user scripts." 
#endif 
#ifndef NS3_MODULE_UART_NET_DEVICE
    // Module headers: 
    #include <ns3/uart-lr-wpan-helper.h>
    #include <ns3/uart-lr-wpan-net-device.h>
    #include <ns3/uart-lr-wpan-mac.h>
#endif 