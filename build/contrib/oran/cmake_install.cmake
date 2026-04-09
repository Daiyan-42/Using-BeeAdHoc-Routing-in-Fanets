# Install script for directory: /Users/daiyan/Codes/NS3_Project_v3.45/ns-3.45/contrib/oran

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
    set(CMAKE_INSTALL_CONFIG_NAME "Debug")
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
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib" TYPE SHARED_LIBRARY FILES "/Users/daiyan/Codes/NS3_Project_v3.45/ns-3.45/build/lib/libns3.45-oran-debug.dylib")
  if(EXISTS "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/libns3.45-oran-debug.dylib" AND
     NOT IS_SYMLINK "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/libns3.45-oran-debug.dylib")
    execute_process(COMMAND /usr/bin/install_name_tool
      -delete_rpath "/Users/daiyan/Codes/NS3_Project_v3.45/ns-3.45/build/lib"
      -add_rpath "/usr/local/lib:$ORIGIN/:$ORIGIN/../lib:/usr/local/lib64:$ORIGIN/:$ORIGIN/../lib64"
      "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/libns3.45-oran-debug.dylib")
    if(CMAKE_INSTALL_DO_STRIP)
      execute_process(COMMAND "/usr/bin/strip" -x "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/libns3.45-oran-debug.dylib")
    endif()
  endif()
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include/ns3" TYPE FILE FILES
    "/Users/daiyan/Codes/NS3_Project_v3.45/ns-3.45/contrib/oran/model/oran-near-rt-ric.h"
    "/Users/daiyan/Codes/NS3_Project_v3.45/ns-3.45/contrib/oran/model/oran-lm.h"
    "/Users/daiyan/Codes/NS3_Project_v3.45/ns-3.45/contrib/oran/model/oran-lm-noop.h"
    "/Users/daiyan/Codes/NS3_Project_v3.45/ns-3.45/contrib/oran/model/oran-lm-lte-2-lte-distance-handover.h"
    "/Users/daiyan/Codes/NS3_Project_v3.45/ns-3.45/contrib/oran/model/oran-lm-lte-2-lte-rsrp-handover.h"
    "/Users/daiyan/Codes/NS3_Project_v3.45/ns-3.45/contrib/oran/model/oran-cmm.h"
    "/Users/daiyan/Codes/NS3_Project_v3.45/ns-3.45/contrib/oran/model/oran-cmm-handover.h"
    "/Users/daiyan/Codes/NS3_Project_v3.45/ns-3.45/contrib/oran/model/oran-cmm-noop.h"
    "/Users/daiyan/Codes/NS3_Project_v3.45/ns-3.45/contrib/oran/model/oran-cmm-single-command-per-node.h"
    "/Users/daiyan/Codes/NS3_Project_v3.45/ns-3.45/contrib/oran/model/oran-command.h"
    "/Users/daiyan/Codes/NS3_Project_v3.45/ns-3.45/contrib/oran/model/oran-command-lte-2-lte-handover.h"
    "/Users/daiyan/Codes/NS3_Project_v3.45/ns-3.45/contrib/oran/model/oran-report.h"
    "/Users/daiyan/Codes/NS3_Project_v3.45/ns-3.45/contrib/oran/model/oran-report-apploss.h"
    "/Users/daiyan/Codes/NS3_Project_v3.45/ns-3.45/contrib/oran/model/oran-report-lte-ue-rsrp-rsrq.h"
    "/Users/daiyan/Codes/NS3_Project_v3.45/ns-3.45/contrib/oran/model/oran-report-location.h"
    "/Users/daiyan/Codes/NS3_Project_v3.45/ns-3.45/contrib/oran/model/oran-report-lte-ue-cell-info.h"
    "/Users/daiyan/Codes/NS3_Project_v3.45/ns-3.45/contrib/oran/model/oran-reporter.h"
    "/Users/daiyan/Codes/NS3_Project_v3.45/ns-3.45/contrib/oran/model/oran-reporter-apploss.h"
    "/Users/daiyan/Codes/NS3_Project_v3.45/ns-3.45/contrib/oran/model/oran-reporter-lte-ue-rsrp-rsrq.h"
    "/Users/daiyan/Codes/NS3_Project_v3.45/ns-3.45/contrib/oran/model/oran-reporter-location.h"
    "/Users/daiyan/Codes/NS3_Project_v3.45/ns-3.45/contrib/oran/model/oran-reporter-lte-ue-cell-info.h"
    "/Users/daiyan/Codes/NS3_Project_v3.45/ns-3.45/contrib/oran/model/oran-data-repository.h"
    "/Users/daiyan/Codes/NS3_Project_v3.45/ns-3.45/contrib/oran/model/oran-data-repository-sqlite.h"
    "/Users/daiyan/Codes/NS3_Project_v3.45/ns-3.45/contrib/oran/model/oran-near-rt-ric-e2terminator.h"
    "/Users/daiyan/Codes/NS3_Project_v3.45/ns-3.45/contrib/oran/model/oran-e2-node-terminator.h"
    "/Users/daiyan/Codes/NS3_Project_v3.45/ns-3.45/contrib/oran/model/oran-e2-node-terminator-wired.h"
    "/Users/daiyan/Codes/NS3_Project_v3.45/ns-3.45/contrib/oran/model/oran-e2-node-terminator-lte-enb.h"
    "/Users/daiyan/Codes/NS3_Project_v3.45/ns-3.45/contrib/oran/model/oran-e2-node-terminator-lte-ue.h"
    "/Users/daiyan/Codes/NS3_Project_v3.45/ns-3.45/contrib/oran/model/oran-e2-node-terminator-container.h"
    "/Users/daiyan/Codes/NS3_Project_v3.45/ns-3.45/contrib/oran/model/oran-report-trigger.h"
    "/Users/daiyan/Codes/NS3_Project_v3.45/ns-3.45/contrib/oran/model/oran-report-trigger-periodic.h"
    "/Users/daiyan/Codes/NS3_Project_v3.45/ns-3.45/contrib/oran/model/oran-report-trigger-lte-ue-handover.h"
    "/Users/daiyan/Codes/NS3_Project_v3.45/ns-3.45/contrib/oran/model/oran-report-trigger-location-change.h"
    "/Users/daiyan/Codes/NS3_Project_v3.45/ns-3.45/contrib/oran/model/oran-query-trigger.h"
    "/Users/daiyan/Codes/NS3_Project_v3.45/ns-3.45/contrib/oran/model/oran-query-trigger-custom.h"
    "/Users/daiyan/Codes/NS3_Project_v3.45/ns-3.45/contrib/oran/helper/oran-helper.h"
    "/Users/daiyan/Codes/NS3_Project_v3.45/ns-3.45/contrib/oran/model/oran-lm-lte-2-lte-torch-handover.h"
    "/Users/daiyan/Codes/NS3_Project_v3.45/ns-3.45/build/include/ns3/oran-module.h"
    )
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for each subdirectory.
  include("/Users/daiyan/Codes/NS3_Project_v3.45/ns-3.45/build/contrib/oran/examples/cmake_install.cmake")

endif()

string(REPLACE ";" "\n" CMAKE_INSTALL_MANIFEST_CONTENT
       "${CMAKE_INSTALL_MANIFEST_FILES}")
if(CMAKE_INSTALL_LOCAL_ONLY)
  file(WRITE "/Users/daiyan/Codes/NS3_Project_v3.45/ns-3.45/build/contrib/oran/install_local_manifest.txt"
     "${CMAKE_INSTALL_MANIFEST_CONTENT}")
endif()
