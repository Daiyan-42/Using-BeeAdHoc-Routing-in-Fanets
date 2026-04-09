#ifdef NS3_MODULE_COMPILATION 
    error "Do not include ns3 module aggregator headers from other modules these are meant only for end user scripts." 
#endif 
#ifndef NS3_MODULE_AQM_EVAL_SUITE
    // Module headers: 
    #include <ns3/eval-topology.h>
    #include <ns3/eval-app.h>
    #include <ns3/aqm-eval-suite-helper.h>
    #include <ns3/aqm-eval-suite-output-manager.h>
    #include <ns3/aqm-eval-suite-plot-manager.h>
#endif 