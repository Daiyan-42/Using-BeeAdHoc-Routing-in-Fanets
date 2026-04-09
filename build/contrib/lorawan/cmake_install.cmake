# Install script for directory: /Users/daiyan/Codes/NS3_Project_v3.45/ns-3.45/contrib/lorawan

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
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib" TYPE SHARED_LIBRARY FILES "/Users/daiyan/Codes/NS3_Project_v3.45/ns-3.45/build/lib/libns3.45-lorawan-debug.dylib")
  if(EXISTS "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/libns3.45-lorawan-debug.dylib" AND
     NOT IS_SYMLINK "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/libns3.45-lorawan-debug.dylib")
    execute_process(COMMAND /usr/bin/install_name_tool
      -delete_rpath "/Users/daiyan/Codes/NS3_Project_v3.45/ns-3.45/build/lib"
      -add_rpath "/usr/local/lib:$ORIGIN/:$ORIGIN/../lib:/usr/local/lib64:$ORIGIN/:$ORIGIN/../lib64"
      "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/libns3.45-lorawan-debug.dylib")
    if(CMAKE_INSTALL_DO_STRIP)
      execute_process(COMMAND "/usr/bin/strip" -x "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/libns3.45-lorawan-debug.dylib")
    endif()
  endif()
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include/ns3" TYPE FILE FILES
    "/Users/daiyan/Codes/NS3_Project_v3.45/ns-3.45/contrib/lorawan/model/lora-net-device.h"
    "/Users/daiyan/Codes/NS3_Project_v3.45/ns-3.45/contrib/lorawan/model/lorawan-mac.h"
    "/Users/daiyan/Codes/NS3_Project_v3.45/ns-3.45/contrib/lorawan/model/lora-phy.h"
    "/Users/daiyan/Codes/NS3_Project_v3.45/ns-3.45/contrib/lorawan/model/building-penetration-loss.h"
    "/Users/daiyan/Codes/NS3_Project_v3.45/ns-3.45/contrib/lorawan/model/correlated-shadowing-propagation-loss-model.h"
    "/Users/daiyan/Codes/NS3_Project_v3.45/ns-3.45/contrib/lorawan/model/lora-channel.h"
    "/Users/daiyan/Codes/NS3_Project_v3.45/ns-3.45/contrib/lorawan/model/lora-interference-helper.h"
    "/Users/daiyan/Codes/NS3_Project_v3.45/ns-3.45/contrib/lorawan/model/gateway-lorawan-mac.h"
    "/Users/daiyan/Codes/NS3_Project_v3.45/ns-3.45/contrib/lorawan/model/end-device-lorawan-mac.h"
    "/Users/daiyan/Codes/NS3_Project_v3.45/ns-3.45/contrib/lorawan/model/class-a-end-device-lorawan-mac.h"
    "/Users/daiyan/Codes/NS3_Project_v3.45/ns-3.45/contrib/lorawan/model/gateway-lora-phy.h"
    "/Users/daiyan/Codes/NS3_Project_v3.45/ns-3.45/contrib/lorawan/model/end-device-lora-phy.h"
    "/Users/daiyan/Codes/NS3_Project_v3.45/ns-3.45/contrib/lorawan/model/simple-end-device-lora-phy.h"
    "/Users/daiyan/Codes/NS3_Project_v3.45/ns-3.45/contrib/lorawan/model/simple-gateway-lora-phy.h"
    "/Users/daiyan/Codes/NS3_Project_v3.45/ns-3.45/contrib/lorawan/model/sub-band.h"
    "/Users/daiyan/Codes/NS3_Project_v3.45/ns-3.45/contrib/lorawan/model/logical-lora-channel.h"
    "/Users/daiyan/Codes/NS3_Project_v3.45/ns-3.45/contrib/lorawan/model/logical-lora-channel-helper.h"
    "/Users/daiyan/Codes/NS3_Project_v3.45/ns-3.45/contrib/lorawan/model/periodic-sender.h"
    "/Users/daiyan/Codes/NS3_Project_v3.45/ns-3.45/contrib/lorawan/model/one-shot-sender.h"
    "/Users/daiyan/Codes/NS3_Project_v3.45/ns-3.45/contrib/lorawan/model/forwarder.h"
    "/Users/daiyan/Codes/NS3_Project_v3.45/ns-3.45/contrib/lorawan/model/lorawan-mac-header.h"
    "/Users/daiyan/Codes/NS3_Project_v3.45/ns-3.45/contrib/lorawan/model/lora-frame-header.h"
    "/Users/daiyan/Codes/NS3_Project_v3.45/ns-3.45/contrib/lorawan/model/mac-command.h"
    "/Users/daiyan/Codes/NS3_Project_v3.45/ns-3.45/contrib/lorawan/model/lora-device-address.h"
    "/Users/daiyan/Codes/NS3_Project_v3.45/ns-3.45/contrib/lorawan/model/lora-device-address-generator.h"
    "/Users/daiyan/Codes/NS3_Project_v3.45/ns-3.45/contrib/lorawan/model/lora-tag.h"
    "/Users/daiyan/Codes/NS3_Project_v3.45/ns-3.45/contrib/lorawan/model/network-server.h"
    "/Users/daiyan/Codes/NS3_Project_v3.45/ns-3.45/contrib/lorawan/model/network-status.h"
    "/Users/daiyan/Codes/NS3_Project_v3.45/ns-3.45/contrib/lorawan/model/network-controller.h"
    "/Users/daiyan/Codes/NS3_Project_v3.45/ns-3.45/contrib/lorawan/model/network-controller-components.h"
    "/Users/daiyan/Codes/NS3_Project_v3.45/ns-3.45/contrib/lorawan/model/network-scheduler.h"
    "/Users/daiyan/Codes/NS3_Project_v3.45/ns-3.45/contrib/lorawan/model/end-device-status.h"
    "/Users/daiyan/Codes/NS3_Project_v3.45/ns-3.45/contrib/lorawan/model/gateway-status.h"
    "/Users/daiyan/Codes/NS3_Project_v3.45/ns-3.45/contrib/lorawan/model/lora-radio-energy-model.h"
    "/Users/daiyan/Codes/NS3_Project_v3.45/ns-3.45/contrib/lorawan/model/lora-tx-current-model.h"
    "/Users/daiyan/Codes/NS3_Project_v3.45/ns-3.45/contrib/lorawan/model/lora-utils.h"
    "/Users/daiyan/Codes/NS3_Project_v3.45/ns-3.45/contrib/lorawan/model/adr-component.h"
    "/Users/daiyan/Codes/NS3_Project_v3.45/ns-3.45/contrib/lorawan/model/hex-grid-position-allocator.h"
    "/Users/daiyan/Codes/NS3_Project_v3.45/ns-3.45/contrib/lorawan/helper/lora-radio-energy-model-helper.h"
    "/Users/daiyan/Codes/NS3_Project_v3.45/ns-3.45/contrib/lorawan/helper/lora-helper.h"
    "/Users/daiyan/Codes/NS3_Project_v3.45/ns-3.45/contrib/lorawan/helper/lora-phy-helper.h"
    "/Users/daiyan/Codes/NS3_Project_v3.45/ns-3.45/contrib/lorawan/helper/lorawan-mac-helper.h"
    "/Users/daiyan/Codes/NS3_Project_v3.45/ns-3.45/contrib/lorawan/helper/periodic-sender-helper.h"
    "/Users/daiyan/Codes/NS3_Project_v3.45/ns-3.45/contrib/lorawan/helper/one-shot-sender-helper.h"
    "/Users/daiyan/Codes/NS3_Project_v3.45/ns-3.45/contrib/lorawan/helper/forwarder-helper.h"
    "/Users/daiyan/Codes/NS3_Project_v3.45/ns-3.45/contrib/lorawan/helper/network-server-helper.h"
    "/Users/daiyan/Codes/NS3_Project_v3.45/ns-3.45/contrib/lorawan/helper/lora-packet-tracker.h"
    "/Users/daiyan/Codes/NS3_Project_v3.45/ns-3.45/contrib/lorawan/test/utilities.h"
    "/Users/daiyan/Codes/NS3_Project_v3.45/ns-3.45/build/include/ns3/lorawan-module.h"
    )
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for each subdirectory.
  include("/Users/daiyan/Codes/NS3_Project_v3.45/ns-3.45/build/contrib/lorawan/examples/cmake_install.cmake")

endif()

string(REPLACE ";" "\n" CMAKE_INSTALL_MANIFEST_CONTENT
       "${CMAKE_INSTALL_MANIFEST_FILES}")
if(CMAKE_INSTALL_LOCAL_ONLY)
  file(WRITE "/Users/daiyan/Codes/NS3_Project_v3.45/ns-3.45/build/contrib/lorawan/install_local_manifest.txt"
     "${CMAKE_INSTALL_MANIFEST_CONTENT}")
endif()
