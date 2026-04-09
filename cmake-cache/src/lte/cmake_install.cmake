# Install script for directory: /Users/daiyan/Codes/NS3_Project_v3.45/ns-3.45/src/lte

# Set the install prefix
if(NOT DEFINED CMAKE_INSTALL_PREFIX)
  set(CMAKE_INSTALL_PREFIX "/usr/local")
endif()
string(REGEX REPLACE "/$" "" CMAKE_INSTALL_PREFIX "${CMAKE_INSTALL_PREFIX}")

# Set the install configuration name.
if(NOT DEFINED CMAKE_INSTALL_CONFIG_NAME)
  if(BUILD_TYPE)
    string(REGEX REPLACE "^[^A-Za-z0-9_]+" ""
           CMAKE_INSTALL_CONFIG_NAME "${BUILD_TYPE}")
  else()
    set(CMAKE_INSTALL_CONFIG_NAME "default")
  endif()
  message(STATUS "Install configuration: \"${CMAKE_INSTALL_CONFIG_NAME}\"")
endif()

# Set the component getting installed.
if(NOT CMAKE_INSTALL_COMPONENT)
  if(COMPONENT)
    message(STATUS "Install component: \"${COMPONENT}\"")
    set(CMAKE_INSTALL_COMPONENT "${COMPONENT}")
  else()
    set(CMAKE_INSTALL_COMPONENT)
  endif()
endif()

# Is this installation the result of a crosscompile?
if(NOT DEFINED CMAKE_CROSSCOMPILING)
  set(CMAKE_CROSSCOMPILING "FALSE")
endif()

# Set path to fallback-tool for dependency-resolution.
if(NOT DEFINED CMAKE_OBJDUMP)
  set(CMAKE_OBJDUMP "/usr/bin/objdump")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib" TYPE SHARED_LIBRARY FILES "/Users/daiyan/Codes/NS3_Project_v3.45/ns-3.45/build/lib/libns3.45-lte-default.dylib")
  if(EXISTS "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/libns3.45-lte-default.dylib" AND
     NOT IS_SYMLINK "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/libns3.45-lte-default.dylib")
    execute_process(COMMAND /usr/bin/install_name_tool
      -delete_rpath "/Users/daiyan/Codes/NS3_Project_v3.45/ns-3.45/build/lib"
      -add_rpath "/usr/local/lib:$ORIGIN/:$ORIGIN/../lib:/usr/local/lib64:$ORIGIN/:$ORIGIN/../lib64"
      "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/libns3.45-lte-default.dylib")
    if(CMAKE_INSTALL_DO_STRIP)
      execute_process(COMMAND "/usr/bin/strip" -x "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/libns3.45-lte-default.dylib")
    endif()
  endif()
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include/ns3" TYPE FILE FILES
    "/Users/daiyan/Codes/NS3_Project_v3.45/ns-3.45/src/lte/helper/cc-helper.h"
    "/Users/daiyan/Codes/NS3_Project_v3.45/ns-3.45/src/lte/helper/epc-helper.h"
    "/Users/daiyan/Codes/NS3_Project_v3.45/ns-3.45/src/lte/helper/lte-global-pathloss-database.h"
    "/Users/daiyan/Codes/NS3_Project_v3.45/ns-3.45/src/lte/helper/lte-helper.h"
    "/Users/daiyan/Codes/NS3_Project_v3.45/ns-3.45/src/lte/helper/lte-hex-grid-enb-topology-helper.h"
    "/Users/daiyan/Codes/NS3_Project_v3.45/ns-3.45/src/lte/helper/lte-stats-calculator.h"
    "/Users/daiyan/Codes/NS3_Project_v3.45/ns-3.45/src/lte/helper/mac-stats-calculator.h"
    "/Users/daiyan/Codes/NS3_Project_v3.45/ns-3.45/src/lte/helper/no-backhaul-epc-helper.h"
    "/Users/daiyan/Codes/NS3_Project_v3.45/ns-3.45/src/lte/helper/phy-rx-stats-calculator.h"
    "/Users/daiyan/Codes/NS3_Project_v3.45/ns-3.45/src/lte/helper/phy-stats-calculator.h"
    "/Users/daiyan/Codes/NS3_Project_v3.45/ns-3.45/src/lte/helper/phy-tx-stats-calculator.h"
    "/Users/daiyan/Codes/NS3_Project_v3.45/ns-3.45/src/lte/helper/point-to-point-epc-helper.h"
    "/Users/daiyan/Codes/NS3_Project_v3.45/ns-3.45/src/lte/helper/radio-bearer-stats-calculator.h"
    "/Users/daiyan/Codes/NS3_Project_v3.45/ns-3.45/src/lte/helper/radio-bearer-stats-connector.h"
    "/Users/daiyan/Codes/NS3_Project_v3.45/ns-3.45/src/lte/helper/radio-environment-map-helper.h"
    "/Users/daiyan/Codes/NS3_Project_v3.45/ns-3.45/src/lte/model/a2-a4-rsrq-handover-algorithm.h"
    "/Users/daiyan/Codes/NS3_Project_v3.45/ns-3.45/src/lte/model/a3-rsrp-handover-algorithm.h"
    "/Users/daiyan/Codes/NS3_Project_v3.45/ns-3.45/src/lte/model/component-carrier-enb.h"
    "/Users/daiyan/Codes/NS3_Project_v3.45/ns-3.45/src/lte/model/component-carrier-ue.h"
    "/Users/daiyan/Codes/NS3_Project_v3.45/ns-3.45/src/lte/model/component-carrier.h"
    "/Users/daiyan/Codes/NS3_Project_v3.45/ns-3.45/src/lte/model/cqa-ff-mac-scheduler.h"
    "/Users/daiyan/Codes/NS3_Project_v3.45/ns-3.45/src/lte/model/epc-enb-application.h"
    "/Users/daiyan/Codes/NS3_Project_v3.45/ns-3.45/src/lte/model/epc-enb-s1-sap.h"
    "/Users/daiyan/Codes/NS3_Project_v3.45/ns-3.45/src/lte/model/epc-gtpc-header.h"
    "/Users/daiyan/Codes/NS3_Project_v3.45/ns-3.45/src/lte/model/epc-gtpu-header.h"
    "/Users/daiyan/Codes/NS3_Project_v3.45/ns-3.45/src/lte/model/epc-mme-application.h"
    "/Users/daiyan/Codes/NS3_Project_v3.45/ns-3.45/src/lte/model/epc-pgw-application.h"
    "/Users/daiyan/Codes/NS3_Project_v3.45/ns-3.45/src/lte/model/epc-s11-sap.h"
    "/Users/daiyan/Codes/NS3_Project_v3.45/ns-3.45/src/lte/model/epc-s1ap-sap.h"
    "/Users/daiyan/Codes/NS3_Project_v3.45/ns-3.45/src/lte/model/epc-sgw-application.h"
    "/Users/daiyan/Codes/NS3_Project_v3.45/ns-3.45/src/lte/model/epc-tft-classifier.h"
    "/Users/daiyan/Codes/NS3_Project_v3.45/ns-3.45/src/lte/model/epc-tft.h"
    "/Users/daiyan/Codes/NS3_Project_v3.45/ns-3.45/src/lte/model/epc-ue-nas.h"
    "/Users/daiyan/Codes/NS3_Project_v3.45/ns-3.45/src/lte/model/epc-x2-header.h"
    "/Users/daiyan/Codes/NS3_Project_v3.45/ns-3.45/src/lte/model/epc-x2-sap.h"
    "/Users/daiyan/Codes/NS3_Project_v3.45/ns-3.45/src/lte/model/epc-x2.h"
    "/Users/daiyan/Codes/NS3_Project_v3.45/ns-3.45/src/lte/model/eps-bearer-tag.h"
    "/Users/daiyan/Codes/NS3_Project_v3.45/ns-3.45/src/lte/model/eps-bearer.h"
    "/Users/daiyan/Codes/NS3_Project_v3.45/ns-3.45/src/lte/model/fdbet-ff-mac-scheduler.h"
    "/Users/daiyan/Codes/NS3_Project_v3.45/ns-3.45/src/lte/model/fdmt-ff-mac-scheduler.h"
    "/Users/daiyan/Codes/NS3_Project_v3.45/ns-3.45/src/lte/model/fdtbfq-ff-mac-scheduler.h"
    "/Users/daiyan/Codes/NS3_Project_v3.45/ns-3.45/src/lte/model/ff-mac-common.h"
    "/Users/daiyan/Codes/NS3_Project_v3.45/ns-3.45/src/lte/model/ff-mac-csched-sap.h"
    "/Users/daiyan/Codes/NS3_Project_v3.45/ns-3.45/src/lte/model/ff-mac-sched-sap.h"
    "/Users/daiyan/Codes/NS3_Project_v3.45/ns-3.45/src/lte/model/ff-mac-scheduler.h"
    "/Users/daiyan/Codes/NS3_Project_v3.45/ns-3.45/src/lte/model/lte-amc.h"
    "/Users/daiyan/Codes/NS3_Project_v3.45/ns-3.45/src/lte/model/lte-anr-sap.h"
    "/Users/daiyan/Codes/NS3_Project_v3.45/ns-3.45/src/lte/model/lte-anr.h"
    "/Users/daiyan/Codes/NS3_Project_v3.45/ns-3.45/src/lte/model/lte-as-sap.h"
    "/Users/daiyan/Codes/NS3_Project_v3.45/ns-3.45/src/lte/model/lte-asn1-header.h"
    "/Users/daiyan/Codes/NS3_Project_v3.45/ns-3.45/src/lte/model/lte-ccm-mac-sap.h"
    "/Users/daiyan/Codes/NS3_Project_v3.45/ns-3.45/src/lte/model/lte-ccm-rrc-sap.h"
    "/Users/daiyan/Codes/NS3_Project_v3.45/ns-3.45/src/lte/model/lte-chunk-processor.h"
    "/Users/daiyan/Codes/NS3_Project_v3.45/ns-3.45/src/lte/model/lte-common.h"
    "/Users/daiyan/Codes/NS3_Project_v3.45/ns-3.45/src/lte/model/lte-control-messages.h"
    "/Users/daiyan/Codes/NS3_Project_v3.45/ns-3.45/src/lte/model/lte-enb-cmac-sap.h"
    "/Users/daiyan/Codes/NS3_Project_v3.45/ns-3.45/src/lte/model/lte-enb-component-carrier-manager.h"
    "/Users/daiyan/Codes/NS3_Project_v3.45/ns-3.45/src/lte/model/lte-enb-cphy-sap.h"
    "/Users/daiyan/Codes/NS3_Project_v3.45/ns-3.45/src/lte/model/lte-enb-mac.h"
    "/Users/daiyan/Codes/NS3_Project_v3.45/ns-3.45/src/lte/model/lte-enb-net-device.h"
    "/Users/daiyan/Codes/NS3_Project_v3.45/ns-3.45/src/lte/model/lte-enb-phy-sap.h"
    "/Users/daiyan/Codes/NS3_Project_v3.45/ns-3.45/src/lte/model/lte-enb-phy.h"
    "/Users/daiyan/Codes/NS3_Project_v3.45/ns-3.45/src/lte/model/lte-enb-rrc.h"
    "/Users/daiyan/Codes/NS3_Project_v3.45/ns-3.45/src/lte/model/lte-ffr-algorithm.h"
    "/Users/daiyan/Codes/NS3_Project_v3.45/ns-3.45/src/lte/model/lte-ffr-distributed-algorithm.h"
    "/Users/daiyan/Codes/NS3_Project_v3.45/ns-3.45/src/lte/model/lte-ffr-enhanced-algorithm.h"
    "/Users/daiyan/Codes/NS3_Project_v3.45/ns-3.45/src/lte/model/lte-ffr-rrc-sap.h"
    "/Users/daiyan/Codes/NS3_Project_v3.45/ns-3.45/src/lte/model/lte-ffr-sap.h"
    "/Users/daiyan/Codes/NS3_Project_v3.45/ns-3.45/src/lte/model/lte-ffr-soft-algorithm.h"
    "/Users/daiyan/Codes/NS3_Project_v3.45/ns-3.45/src/lte/model/lte-fr-hard-algorithm.h"
    "/Users/daiyan/Codes/NS3_Project_v3.45/ns-3.45/src/lte/model/lte-fr-no-op-algorithm.h"
    "/Users/daiyan/Codes/NS3_Project_v3.45/ns-3.45/src/lte/model/lte-fr-soft-algorithm.h"
    "/Users/daiyan/Codes/NS3_Project_v3.45/ns-3.45/src/lte/model/lte-fr-strict-algorithm.h"
    "/Users/daiyan/Codes/NS3_Project_v3.45/ns-3.45/src/lte/model/lte-handover-algorithm.h"
    "/Users/daiyan/Codes/NS3_Project_v3.45/ns-3.45/src/lte/model/lte-handover-management-sap.h"
    "/Users/daiyan/Codes/NS3_Project_v3.45/ns-3.45/src/lte/model/lte-harq-phy.h"
    "/Users/daiyan/Codes/NS3_Project_v3.45/ns-3.45/src/lte/model/lte-interference.h"
    "/Users/daiyan/Codes/NS3_Project_v3.45/ns-3.45/src/lte/model/lte-mac-sap.h"
    "/Users/daiyan/Codes/NS3_Project_v3.45/ns-3.45/src/lte/model/lte-mi-error-model.h"
    "/Users/daiyan/Codes/NS3_Project_v3.45/ns-3.45/src/lte/model/lte-net-device.h"
    "/Users/daiyan/Codes/NS3_Project_v3.45/ns-3.45/src/lte/model/lte-pdcp-header.h"
    "/Users/daiyan/Codes/NS3_Project_v3.45/ns-3.45/src/lte/model/lte-pdcp-sap.h"
    "/Users/daiyan/Codes/NS3_Project_v3.45/ns-3.45/src/lte/model/lte-pdcp-tag.h"
    "/Users/daiyan/Codes/NS3_Project_v3.45/ns-3.45/src/lte/model/lte-pdcp.h"
    "/Users/daiyan/Codes/NS3_Project_v3.45/ns-3.45/src/lte/model/lte-phy-tag.h"
    "/Users/daiyan/Codes/NS3_Project_v3.45/ns-3.45/src/lte/model/lte-phy.h"
    "/Users/daiyan/Codes/NS3_Project_v3.45/ns-3.45/src/lte/model/lte-radio-bearer-info.h"
    "/Users/daiyan/Codes/NS3_Project_v3.45/ns-3.45/src/lte/model/lte-radio-bearer-tag.h"
    "/Users/daiyan/Codes/NS3_Project_v3.45/ns-3.45/src/lte/model/lte-rlc-am-header.h"
    "/Users/daiyan/Codes/NS3_Project_v3.45/ns-3.45/src/lte/model/lte-rlc-am.h"
    "/Users/daiyan/Codes/NS3_Project_v3.45/ns-3.45/src/lte/model/lte-rlc-header.h"
    "/Users/daiyan/Codes/NS3_Project_v3.45/ns-3.45/src/lte/model/lte-rlc-sap.h"
    "/Users/daiyan/Codes/NS3_Project_v3.45/ns-3.45/src/lte/model/lte-rlc-sdu-status-tag.h"
    "/Users/daiyan/Codes/NS3_Project_v3.45/ns-3.45/src/lte/model/lte-rlc-sequence-number.h"
    "/Users/daiyan/Codes/NS3_Project_v3.45/ns-3.45/src/lte/model/lte-rlc-tag.h"
    "/Users/daiyan/Codes/NS3_Project_v3.45/ns-3.45/src/lte/model/lte-rlc-tm.h"
    "/Users/daiyan/Codes/NS3_Project_v3.45/ns-3.45/src/lte/model/lte-rlc-um.h"
    "/Users/daiyan/Codes/NS3_Project_v3.45/ns-3.45/src/lte/model/lte-rlc.h"
    "/Users/daiyan/Codes/NS3_Project_v3.45/ns-3.45/src/lte/model/lte-rrc-header.h"
    "/Users/daiyan/Codes/NS3_Project_v3.45/ns-3.45/src/lte/model/lte-rrc-protocol-ideal.h"
    "/Users/daiyan/Codes/NS3_Project_v3.45/ns-3.45/src/lte/model/lte-rrc-protocol-real.h"
    "/Users/daiyan/Codes/NS3_Project_v3.45/ns-3.45/src/lte/model/lte-rrc-sap.h"
    "/Users/daiyan/Codes/NS3_Project_v3.45/ns-3.45/src/lte/model/lte-spectrum-phy.h"
    "/Users/daiyan/Codes/NS3_Project_v3.45/ns-3.45/src/lte/model/lte-spectrum-signal-parameters.h"
    "/Users/daiyan/Codes/NS3_Project_v3.45/ns-3.45/src/lte/model/lte-spectrum-value-helper.h"
    "/Users/daiyan/Codes/NS3_Project_v3.45/ns-3.45/src/lte/model/lte-ue-ccm-rrc-sap.h"
    "/Users/daiyan/Codes/NS3_Project_v3.45/ns-3.45/src/lte/model/lte-ue-cmac-sap.h"
    "/Users/daiyan/Codes/NS3_Project_v3.45/ns-3.45/src/lte/model/lte-ue-component-carrier-manager.h"
    "/Users/daiyan/Codes/NS3_Project_v3.45/ns-3.45/src/lte/model/lte-ue-cphy-sap.h"
    "/Users/daiyan/Codes/NS3_Project_v3.45/ns-3.45/src/lte/model/lte-ue-mac.h"
    "/Users/daiyan/Codes/NS3_Project_v3.45/ns-3.45/src/lte/model/lte-ue-net-device.h"
    "/Users/daiyan/Codes/NS3_Project_v3.45/ns-3.45/src/lte/model/lte-ue-phy-sap.h"
    "/Users/daiyan/Codes/NS3_Project_v3.45/ns-3.45/src/lte/model/lte-ue-phy.h"
    "/Users/daiyan/Codes/NS3_Project_v3.45/ns-3.45/src/lte/model/lte-ue-power-control.h"
    "/Users/daiyan/Codes/NS3_Project_v3.45/ns-3.45/src/lte/model/lte-ue-rrc.h"
    "/Users/daiyan/Codes/NS3_Project_v3.45/ns-3.45/src/lte/model/lte-vendor-specific-parameters.h"
    "/Users/daiyan/Codes/NS3_Project_v3.45/ns-3.45/src/lte/model/no-op-component-carrier-manager.h"
    "/Users/daiyan/Codes/NS3_Project_v3.45/ns-3.45/src/lte/model/no-op-handover-algorithm.h"
    "/Users/daiyan/Codes/NS3_Project_v3.45/ns-3.45/src/lte/model/pf-ff-mac-scheduler.h"
    "/Users/daiyan/Codes/NS3_Project_v3.45/ns-3.45/src/lte/model/pss-ff-mac-scheduler.h"
    "/Users/daiyan/Codes/NS3_Project_v3.45/ns-3.45/src/lte/model/rem-spectrum-phy.h"
    "/Users/daiyan/Codes/NS3_Project_v3.45/ns-3.45/src/lte/model/rr-ff-mac-scheduler.h"
    "/Users/daiyan/Codes/NS3_Project_v3.45/ns-3.45/src/lte/model/simple-ue-component-carrier-manager.h"
    "/Users/daiyan/Codes/NS3_Project_v3.45/ns-3.45/src/lte/model/tdbet-ff-mac-scheduler.h"
    "/Users/daiyan/Codes/NS3_Project_v3.45/ns-3.45/src/lte/model/tdmt-ff-mac-scheduler.h"
    "/Users/daiyan/Codes/NS3_Project_v3.45/ns-3.45/src/lte/model/tdtbfq-ff-mac-scheduler.h"
    "/Users/daiyan/Codes/NS3_Project_v3.45/ns-3.45/src/lte/model/tta-ff-mac-scheduler.h"
    "/Users/daiyan/Codes/NS3_Project_v3.45/ns-3.45/build/include/ns3/lte-module.h"
    "/Users/daiyan/Codes/NS3_Project_v3.45/ns-3.45/build/include/ns3/lte-export.h"
    )
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for each subdirectory.
  include("/Users/daiyan/Codes/NS3_Project_v3.45/ns-3.45/cmake-cache/src/lte/examples/cmake_install.cmake")

endif()

string(REPLACE ";" "\n" CMAKE_INSTALL_MANIFEST_CONTENT
       "${CMAKE_INSTALL_MANIFEST_FILES}")
if(CMAKE_INSTALL_LOCAL_ONLY)
  file(WRITE "/Users/daiyan/Codes/NS3_Project_v3.45/ns-3.45/cmake-cache/src/lte/install_local_manifest.txt"
     "${CMAKE_INSTALL_MANIFEST_CONTENT}")
endif()
