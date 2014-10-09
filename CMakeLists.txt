CMAKE_MINIMUM_REQUIRED(VERSION 2.6)
PROJECT(microblx_rsg_bridge)

set(CMAKE_CXX_FLAGS "-Wall -Werror -fvisibility=hidden")
set(CMAKE_CXX_COMPILER clang++ )
set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake") ## temporary resources, for UBX

# Global
set(INSTALL_LIB_TYPES_DIR lib/microblx/types CACHE PATH "Installation directory for types libraries")
set(INSTALL_LIB_BLOCKS_DIR lib/microblx/blocks CACHE PATH "Installation directory for blocks libraries")
set(INSTALL_INCLUDE_DIR include/microblx CACHE PATH "Installation directory for header files (types)")
set(INSTALL_CMAKE_DIR  share/microblx/cmake CACHE PATH  "Installation directory for CMake files") # default ${DEF_INSTALL_CMAKE_DIR}

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
find_package(BRICS_3D REQUIRED)
ADD_DEFINITIONS(-DEIGEN3)

# search for HDF5 (>= 1.8.9)
#
# for version 1.8.12 or higher please define the following flag : HDF_1_8_12_OR_HIGHER
# due to incompatible API changes in the HDF5 library.

# only newer versions of FindHDF5.cmake exctract the HDF5_ROOT from the environment variable
# - so we do it manually here
IF("$ENV{HDF5_ROOT}" STREQUAL "")
    #nothing special here      
ELSE("$ENV{HDF5_ROOT}" STREQUAL "")
    SET(HDF5_ROOT $ENV{HDF5_ROOT})
    SET(USE_HDF5_NON_DEFAULT_PATH TRUE) 
    MESSAGE(STATUS "INFO: HDF5_ROOT set as: $ENV{HDF5_ROOT}")
ENDIF("$ENV{HDF5_ROOT}" STREQUAL "")
  
FIND_PACKAGE(HDF5 REQUIRED COMPONENTS CXX HL)
IF(USE_HDF5_NON_DEFAULT_PATH) #override results
    SET(HDF5_CXX_INCLUDE_DIR ${HDF5_ROOT}/include)        
    SET(HDF5_LIBRARIES 
        ${HDF5_ROOT}/lib/libhdf5_hl_debug.so
        ${HDF5_ROOT}/lib/libhdf5_cpp_debug.so
        ${HDF5_ROOT}/lib/libhdf5_debug.so   
    )
ENDIF(USE_HDF5_NON_DEFAULT_PATH)   
MESSAGE(STATUS "INFO: used HDF5_LIBRARIES: ${HDF5_LIBRARIES}")


include_directories(
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
target_link_libraries(rsgsenderlib ${BRICS_3D_LIBRARIES} ${HDF5_LIBRARIES} ${UBX_LIBRARIES})

# Install rsgsenderlib
install(TARGETS rsgsenderlib DESTINATION ${INSTALL_LIB_BLOCKS_DIR} EXPORT rsgsenderlib-block)
set_property(TARGET rsgsenderlib PROPERTY INSTALL_RPATH_USE_LINK_PATH TRUE)
install(EXPORT rsgsenderlib-block DESTINATION ${INSTALL_CMAKE_DIR})

# Compile library rsgrecieverlib
add_library(rsgrecieverlib SHARED src/rsg_reciever.cpp )
set_target_properties(rsgrecieverlib PROPERTIES PREFIX "")
target_link_libraries(rsgrecieverlib ${BRICS_3D_LIBRARIES} ${HDF5_LIBRARIES} ${UBX_LIBRARIES})

# Install rsgrecieverlib
install(TARGETS rsgrecieverlib DESTINATION ${INSTALL_LIB_BLOCKS_DIR} EXPORT rsgrecieverlib-block)
set_property(TARGET rsgrecieverlib PROPERTY INSTALL_RPATH_USE_LINK_PATH TRUE)
install(EXPORT rsgrecieverlib-block DESTINATION ${INSTALL_CMAKE_DIR})

# Compile library rsgscenesetuplib
add_library(rsgscenesetuplib SHARED src/rsg_scene_setup.cpp )
set_target_properties(rsgscenesetuplib PROPERTIES PREFIX "")
target_link_libraries(rsgscenesetuplib ${BRICS_3D_LIBRARIES} ${HDF5_LIBRARIES} ${UBX_LIBRARIES})

# Install rsgscenesetuplib
install(TARGETS rsgscenesetuplib DESTINATION ${INSTALL_LIB_BLOCKS_DIR} EXPORT rsgscenesetuplib-block)
set_property(TARGET rsgscenesetuplib PROPERTY INSTALL_RPATH_USE_LINK_PATH TRUE)
install(EXPORT rsgscenesetuplib-block DESTINATION ${INSTALL_CMAKE_DIR})


# Compile library tstnodesetuplib
#add_library(tstnodesetuplib SHARED src/tst_node_setup.cpp )
#set_target_properties(tstnodesetuplib PROPERTIES PREFIX "")
#target_link_libraries(tstnodesetuplib ${BRICS_3D_LIBRARIES} ${HDF5_LIBRARIES} ${UBX_LIBRARIES})

# Install rsgscenesetuplib
#install(TARGETS tstnodesetuplib DESTINATION ${INSTALL_LIB_BLOCKS_DIR} EXPORT tstnodesetuplib-block)
#set_property(TARGET tstnodesetuplib PROPERTY INSTALL_RPATH_USE_LINK_PATH TRUE)
#install(EXPORT tstnodesetuplib-block DESTINATION ${INSTALL_CMAKE_DIR})
