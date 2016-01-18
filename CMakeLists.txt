CMAKE_MINIMUM_REQUIRED(VERSION 2.6)
PROJECT(microblx_rsg_bridge)

set(CMAKE_CXX_FLAGS "-Wall -Werror -fvisibility=hidden")
set(CMAKE_CXX_COMPILER clang++ )
set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake") ## temporary resources, for UBX

# Global
set(INSTALL_LIB_TYPES_DIR lib/ubx/types CACHE PATH "Installation directory for types libraries")
set(INSTALL_LIB_BLOCKS_DIR lib/ubx/blocks CACHE PATH "Installation directory for blocks libraries")
set(INSTALL_INCLUDE_DIR include/ubx CACHE PATH "Installation directory for header files (types)")
set(INSTALL_CMAKE_DIR  share/ubx/cmake CACHE PATH  "Installation directory for CMake files") # default ${DEF_INSTALL_CMAKE_DIR}

# Make relative paths absolute
foreach(p LIB_TYPES LIB_BLOCKS BIN INCLUDE CMAKE)
  set(var INSTALL_${p}_DIR)
  if(NOT IS_ABSOLUTE "${${var}}")
    set(${var} "${CMAKE_INSTALL_PREFIX}/${${var}}")
  endif()
endforeach()

##
# Add uninstall target.
##
configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/cmake_uninstall.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/cmake/cmake_uninstall.cmake"
    IMMEDIATE @ONLY)
    
add_custom_target(uninstall
    "${CMAKE_COMMAND}" -P "${CMAKE_CURRENT_BINARY_DIR}/cmake/cmake_uninstall.cmake"
)

##
# Generate config package
##
set(CONF_INCLUDE_DIRS "${INSTALL_INCLUDE_DIR}")
set(CONF_CMAKE_DIR "${INSTALL_CMAKE_DIR}")
configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/microblx-rsg-bridge-config.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/cmake/microblx-rsg-bridge-config.cmake" @ONLY
)

set (UBX_ROOT $ENV{UBX_ROOT} )
if (NOT UBX_ROOT)
 message( FATAL_ERROR "UBX_ROOT is not defined. CMake install willl abort." )
endif()

find_package(Microblx REQUIRED)
FIND_PACKAGE(Eigen REQUIRED)
FIND_PACKAGE(Boost COMPONENTS regex)
find_package(BRICS_3D REQUIRED)
ADD_DEFINITIONS(-DEIGEN3)



# search for HDF5 (>= 1.8.9)
#
# for version 1.8.12 or higher please define the following flag : HDF_1_8_12_OR_HIGHER
# due to incompatible API changes in the HDF5 library.
OPTION(HDF5_1_8_12_OR_HIGHER "Enable HDF5 Support for version 1.8.12 or higher. This is rquired due to HDF5 API changes." OFF)
IF(HDF5_1_8_12_OR_HIGHER)
    ADD_DEFINITIONS(-DHDF_1_8_12_OR_HIGHER)  
ENDIF(HDF5_1_8_12_OR_HIGHER)

# only newer versions of FindHDF5.cmake exctract the HDF5_ROOT from the environment variable
# - so we do it manually here
IF("$ENV{HDF5_ROOT}" STREQUAL "")
    #nothing special here      
ELSE("$ENV{HDF5_ROOT}" STREQUAL "")
    SET(HDF5_ROOT $ENV{HDF5_ROOT})
    SET(USE_HDF5_NON_DEFAULT_PATH TRUE) 
    MESSAGE(STATUS "INFO: HDF5_ROOT set as: $ENV{HDF5_ROOT}")
ENDIF("$ENV{HDF5_ROOT}" STREQUAL "")
OPTION(USE_HDF5_DEBUG_LIBS "If enabled, the debug libraries of HDF5 are used." OFF) 
  
FIND_PACKAGE(HDF5 REQUIRED COMPONENTS CXX HL)
IF(USE_HDF5_NON_DEFAULT_PATH) #override results
    SET(HDF5_CXX_INCLUDE_DIR ${HDF5_ROOT}/include)       
    IF(USE_HDF5_DEBUG_LIBS) 
        SET(HDF5_LIBRARIES         
            ${HDF5_ROOT}/lib/libhdf5_hl_debug.so
            ${HDF5_ROOT}/lib/libhdf5_cpp_debug.so
            ${HDF5_ROOT}/lib/libhdf5_debug.so  
        )
    ELSE(USE_HDF5_DEBUG_LIBS) 
        SET(HDF5_LIBRARIES         
            ${HDF5_ROOT}/lib/libhdf5_hl.so
            ${HDF5_ROOT}/lib/libhdf5_cpp.so
            ${HDF5_ROOT}/lib/libhdf5.so    
        )
    ENDIF(USE_HDF5_DEBUG_LIBS) 
ENDIF(USE_HDF5_NON_DEFAULT_PATH)   
MESSAGE(STATUS "INFO: used HDF5_LIBRARIES: ${HDF5_LIBRARIES}")

# search for LibVariant to parse JSON files
OPTION(USE_JSON "Enable JSON Support. Requires libvariant to be installed." OFF)
IF(USE_JSON)  
    FIND_PACKAGE(LibVariant)
    IF (LIBVARIANT_FOUND)
      MESSAGE(STATUS "SUCCESSFUL: LIBVARIANT_FOUND found")  
    ELSE (LIBVARIANT_FOUND)
      MESSAGE(SEND_ERROR "WARNING: LIBVARIANT_FOUND not found.")
    ENDIF (LIBVARIANT_FOUND)
ENDIF(USE_JSON)

include_directories(
  ${Boost_INCLUDE_DIR}
  ${UBX_INCLUDE_DIR}
  $ENV{FBX_MODULES}
  ${BRICS_3D_INCLUDE_DIRS}
  ${EIGEN_INCLUDE_DIR}
  ${HDF5_CXX_INCLUDE_DIR}
)

LINK_DIRECTORIES(${BRICS_3D_LINK_DIRECTORIES})

# Compile library rsgsenderlib
add_library(rsgsenderlib SHARED src/rsg_sender.cpp )
set_target_properties(rsgsenderlib PROPERTIES PREFIX "")
target_link_libraries(rsgsenderlib ${BRICS_3D_LIBRARIES} ${HDF5_LIBRARIES} ${UBX_LIBRARIES} ${Boost_LIBRARIES})

# Install rsgsenderlib
install(TARGETS rsgsenderlib DESTINATION ${INSTALL_LIB_BLOCKS_DIR} EXPORT rsgsenderlib-block)
set_property(TARGET rsgsenderlib PROPERTY INSTALL_RPATH_USE_LINK_PATH TRUE)
install(EXPORT rsgsenderlib-block DESTINATION ${INSTALL_CMAKE_DIR})

# Compile library rsgrecieverlib
add_library(rsgrecieverlib SHARED src/rsg_reciever.cpp )
set_target_properties(rsgrecieverlib PROPERTIES PREFIX "")
target_link_libraries(rsgrecieverlib ${BRICS_3D_LIBRARIES} ${HDF5_LIBRARIES} ${UBX_LIBRARIES} ${Boost_LIBRARIES})

# Install rsgrecieverlib
install(TARGETS rsgrecieverlib DESTINATION ${INSTALL_LIB_BLOCKS_DIR} EXPORT rsgrecieverlib-block)
set_property(TARGET rsgrecieverlib PROPERTY INSTALL_RPATH_USE_LINK_PATH TRUE)
install(EXPORT rsgrecieverlib-block DESTINATION ${INSTALL_CMAKE_DIR})

IF(USE_JSON)
    INCLUDE_DIRECTORIES(${LIBVARIANT_INCLUDE_DIRS})

    # Compile library rsgsenderlib
    add_library(rsgjsonsenderlib SHARED src/rsg_json_sender.cpp )
    set_target_properties(rsgjsonsenderlib PROPERTIES PREFIX "")
    target_link_libraries(rsgjsonsenderlib ${BRICS_3D_LIBRARIES} ${HDF5_LIBRARIES} ${UBX_LIBRARIES} ${LIBVARIANT_LIBRARIES} ${Boost_LIBRARIES})
    
    # Install rsgsenderlib
    install(TARGETS rsgjsonsenderlib DESTINATION ${INSTALL_LIB_BLOCKS_DIR} EXPORT rsgjsonsenderlib-block)
    set_property(TARGET rsgjsonsenderlib PROPERTY INSTALL_RPATH_USE_LINK_PATH TRUE)
    install(EXPORT rsgjsonsenderlib-block DESTINATION ${INSTALL_CMAKE_DIR})

    # Compile library rsgjsonrecieverlib
    add_library(rsgjsonrecieverlib SHARED src/rsg_json_reciever.cpp )
    set_target_properties(rsgjsonrecieverlib PROPERTIES PREFIX "")
    target_link_libraries(rsgjsonrecieverlib ${BRICS_3D_LIBRARIES} ${HDF5_LIBRARIES} ${UBX_LIBRARIES} ${LIBVARIANT_LIBRARIES} ${Boost_LIBRARIES})
    
    # Install rsgjsonrecieverlib
    install(TARGETS rsgjsonrecieverlib DESTINATION ${INSTALL_LIB_BLOCKS_DIR} EXPORT rsgjsonrecieverlib-block)
    set_property(TARGET rsgjsonrecieverlib PROPERTY INSTALL_RPATH_USE_LINK_PATH TRUE)
    install(EXPORT rsgjsonrecieverlib-block DESTINATION ${INSTALL_CMAKE_DIR})
    
    # Compile library rsgjsonquerylib
    add_library(rsgjsonquerylib SHARED src/rsg_json_query.cpp )
    set_target_properties(rsgjsonquerylib PROPERTIES PREFIX "")
    target_link_libraries(rsgjsonquerylib ${BRICS_3D_LIBRARIES} ${HDF5_LIBRARIES} ${UBX_LIBRARIES} ${LIBVARIANT_LIBRARIES} ${Boost_LIBRARIES})
    
    # Install rsgjsonquerylib
    install(TARGETS rsgjsonquerylib DESTINATION ${INSTALL_LIB_BLOCKS_DIR} EXPORT rsgjsonquerylib-block)
    set_property(TARGET rsgjsonquerylib PROPERTY INSTALL_RPATH_USE_LINK_PATH TRUE)
    install(EXPORT rsgjsonquerylib-block DESTINATION ${INSTALL_CMAKE_DIR})

    # Compile library rsgscenesetuplib
    add_library(rsgscenesetuplib SHARED src/rsg_scene_setup.cpp )
    set_target_properties(rsgscenesetuplib PROPERTIES PREFIX "")
    target_link_libraries(rsgscenesetuplib ${BRICS_3D_LIBRARIES} ${HDF5_LIBRARIES} ${UBX_LIBRARIES} ${LIBVARIANT_LIBRARIES} ${Boost_LIBRARIES})
    
    # Install rsgscenesetuplib
    install(TARGETS rsgscenesetuplib DESTINATION ${INSTALL_LIB_BLOCKS_DIR} EXPORT rsgscenesetuplib-block)
    set_property(TARGET rsgscenesetuplib PROPERTY INSTALL_RPATH_USE_LINK_PATH TRUE)
    install(EXPORT rsgscenesetuplib-block DESTINATION ${INSTALL_CMAKE_DIR})
    
        
ENDIF(USE_JSON)

# Compile library rsgdumplib
add_library(rsgdumplib SHARED src/rsg_dump.cpp )
set_target_properties(rsgdumplib PROPERTIES PREFIX "")
target_link_libraries(rsgdumplib ${BRICS_3D_LIBRARIES} ${HDF5_LIBRARIES} ${UBX_LIBRARIES} ${Boost_LIBRARIES})

# Install rsgdumplib
install(TARGETS rsgdumplib DESTINATION ${INSTALL_LIB_BLOCKS_DIR} EXPORT rsgdumplib-block)
set_property(TARGET rsgdumplib PROPERTY INSTALL_RPATH_USE_LINK_PATH TRUE)
install(EXPORT rsgdumplib-block DESTINATION ${INSTALL_CMAKE_DIR})

# To compile the rsg_bridge_test_app uncomment this section and update all mudules paths within src/rsg_bridge_test_app.c
#add_executable(rsg_bridge_test_app src/rsg_bridge_test_app.c)
#target_link_libraries(rsg_bridge_test_app ${UBX_LIBRARIES})
#add_dependencies(rsg_bridge_test_app gen_hexarr)

