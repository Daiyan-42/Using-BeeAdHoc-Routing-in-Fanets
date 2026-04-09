#ifdef NS3_MODULE_COMPILATION 
    error "Do not include ns3 module aggregator headers from other modules these are meant only for end user scripts." 
#endif 
#ifndef NS3_MODULE_ORAN
    // Module headers: 
    #include <ns3/oran-near-rt-ric.h>
    #include <ns3/oran-lm.h>
    #include <ns3/oran-lm-noop.h>
    #include <ns3/oran-lm-lte-2-lte-distance-handover.h>
    #include <ns3/oran-lm-lte-2-lte-rsrp-handover.h>
    #include <ns3/oran-cmm.h>
    #include <ns3/oran-cmm-handover.h>
    #include <ns3/oran-cmm-noop.h>
    #include <ns3/oran-cmm-single-command-per-node.h>
    #include <ns3/oran-command.h>
    #include <ns3/oran-command-lte-2-lte-handover.h>
    #include <ns3/oran-report.h>
    #include <ns3/oran-report-apploss.h>
    #include <ns3/oran-report-lte-ue-rsrp-rsrq.h>
    #include <ns3/oran-report-location.h>
    #include <ns3/oran-report-lte-ue-cell-info.h>
    #include <ns3/oran-reporter.h>
    #include <ns3/oran-reporter-apploss.h>
    #include <ns3/oran-reporter-lte-ue-rsrp-rsrq.h>
    #include <ns3/oran-reporter-location.h>
    #include <ns3/oran-reporter-lte-ue-cell-info.h>
    #include <ns3/oran-data-repository.h>
    #include <ns3/oran-data-repository-sqlite.h>
    #include <ns3/oran-near-rt-ric-e2terminator.h>
    #include <ns3/oran-e2-node-terminator.h>
    #include <ns3/oran-e2-node-terminator-wired.h>
    #include <ns3/oran-e2-node-terminator-lte-enb.h>
    #include <ns3/oran-e2-node-terminator-lte-ue.h>
    #include <ns3/oran-e2-node-terminator-container.h>
    #include <ns3/oran-report-trigger.h>
    #include <ns3/oran-report-trigger-periodic.h>
    #include <ns3/oran-report-trigger-lte-ue-handover.h>
    #include <ns3/oran-report-trigger-location-change.h>
    #include <ns3/oran-query-trigger.h>
    #include <ns3/oran-query-trigger-custom.h>
    #include <ns3/oran-helper.h>
    #include <ns3/oran-lm-lte-2-lte-torch-handover.h>
#endif 