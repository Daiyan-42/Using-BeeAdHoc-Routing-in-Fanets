# Install script for directory: /Users/daiyan/Codes/NS3_Project_v3.45/ns-3.45/src/lr-wpan

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
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib" TYPE SHARED_LIBRARY FILES "/Users/daiyan/Codes/NS3_Project_v3.45/ns-3.45/build/lib/libns3.45-lr-wpan-default.dylib")
  if(EXISTS "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/libns3.45-lr-wpan-default.dylib" AND
     NOT IS_SYMLINK "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/libns3.45-lr-wpan-default.dylib")
    execute_process(COMMAND /usr/bin/install_name_tool
      -delete_rpath "/Users/daiyan/Codes/NS3_Project_v3.45/ns-3.45/build/lib"
      -add_rpath "/usr/local/lib:$ORIGIN/:$ORIGIN/../lib:/usr/local/lib64:$ORIGIN/:$ORIGIN/../lib64"
      "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/libns3.45-lr-wpan-default.dylib")
    if(CMAKE_INSTALL_DO_STRIP)
      execute_process(COMMAND "/usr/bin/strip" -x "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/libns3.45-lr-wpan-default.dylib")
    endif()
  endif()
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include/ns3" TYPE FILE FILES
    "/Users/daiyan/Codes/NS3_Project_v3.45/ns-3.45/src/lr-wpan/helper/lr-wpan-helper.h"
    "/Users/daiyan/Codes/NS3_Project_v3.45/ns-3.45/src/lr-wpan/model/lr-wpan-constants.h"
    "/Users/daiyan/Codes/NS3_Project_v3.45/ns-3.45/src/lr-wpan/model/lr-wpan-csmaca.h"
    "/Users/daiyan/Codes/NS3_Project_v3.45/ns-3.45/src/lr-wpan/model/lr-wpan-error-model.h"
    "/Users/daiyan/Codes/NS3_Project_v3.45/ns-3.45/src/lr-wpan/model/lr-wpan-fields.h"
    "/Users/daiyan/Codes/NS3_Project_v3.45/ns-3.45/src/lr-wpan/model/lr-wpan-interference-helper.h"
    "/Users/daiyan/Codes/NS3_Project_v3.45/ns-3.45/src/lr-wpan/model/lr-wpan-lqi-tag.h"
    "/Users/daiyan/Codes/NS3_Project_v3.45/ns-3.45/src/lr-wpan/model/lr-wpan-mac-header.h"
    "/Users/daiyan/Codes/NS3_Project_v3.45/ns-3.45/src/lr-wpan/model/lr-wpan-mac-pl-headers.h"
    "/Users/daiyan/Codes/NS3_Project_v3.45/ns-3.45/src/lr-wpan/model/lr-wpan-mac-trailer.h"
    "/Users/daiyan/Codes/NS3_Project_v3.45/ns-3.45/src/lr-wpan/model/lr-wpan-mac-base.h"
    "/Users/daiyan/Codes/NS3_Project_v3.45/ns-3.45/src/lr-wpan/model/lr-wpan-mac.h"
    "/Users/daiyan/Codes/NS3_Project_v3.45/ns-3.45/src/lr-wpan/model/lr-wpan-net-device.h"
    "/Users/daiyan/Codes/NS3_Project_v3.45/ns-3.45/src/lr-wpan/model/lr-wpan-phy.h"
    "/Users/daiyan/Codes/NS3_Project_v3.45/ns-3.45/src/lr-wpan/model/lr-wpan-spectrum-signal-parameters.h"
    "/Users/daiyan/Codes/NS3_Project_v3.45/ns-3.45/src/lr-wpan/model/lr-wpan-spectrum-value-helper.h"
    "/Users/daiyan/Codes/NS3_Project_v3.45/ns-3.45/build/include/ns3/lr-wpan-module.h"
    )
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for each subdirectory.
  include("/Users/daiyan/Codes/NS3_Project_v3.45/ns-3.45/cmake-cache/src/lr-wpan/examples/cmake_install.cmake")

endif()

string(REPLACE ";" "\n" CMAKE_INSTALL_MANIFEST_CONTENT
       "${CMAKE_INSTALL_MANIFEST_FILES}")
if(CMAKE_INSTALL_LOCAL_ONLY)
  file(WRITE "/Users/daiyan/Codes/NS3_Project_v3.45/ns-3.45/cmake-cache/src/lr-wpan/install_local_manifest.txt"
     "${CMAKE_INSTALL_MANIFEST_CONTENT}")
endif()
