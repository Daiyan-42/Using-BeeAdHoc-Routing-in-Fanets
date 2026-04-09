#ifdef NS3_MODULE_COMPILATION 
    error "Do not include ns3 module aggregator headers from other modules these are meant only for end user scripts." 
#endif 
#ifndef NS3_MODULE_NETSIMULYZER
    // Module headers: 
    #include <ns3/area-helper.h>
    #include <ns3/building-configuration-container.h>
    #include <ns3/building-configuration-helper.h>
    #include <ns3/logical-link-helper.h>
    #include <ns3/node-configuration-container.h>
    #include <ns3/node-configuration-helper.h>
    #include <ns3/throughput-sink-helper.h>
    #include <ns3/json.hpp>
    #include <ns3/event-message.h>
    #include <ns3/log-stream.h>
    #include <ns3/logical-link.h>
    #include <ns3/netsimulyzer-3D-models.h>
    #include <ns3/netsimulyzer-ns3-compatibility.h>
    #include <ns3/netsimulyzer-version.h>
    #include <ns3/node-configuration.h>
    #include <ns3/optional.h>
    #include <ns3/optional-types.h>
    #include <ns3/building-configuration.h>
    #include <ns3/category-axis.h>
    #include <ns3/category-value-series.h>
    #include <ns3/color.h>
    #include <ns3/color-palette.h>
    #include <ns3/decoration.h>
    #include <ns3/ecdf-sink.h>
    #include <ns3/orchestrator.h>
    #include <ns3/rectangular-area.h>
    #include <ns3/series-collection.h>
    #include <ns3/state-transition-sink.h>
    #include <ns3/value-axis.h>
    #include <ns3/xy-series.h>
    #include <ns3/throughput-sink.h>
#endif 