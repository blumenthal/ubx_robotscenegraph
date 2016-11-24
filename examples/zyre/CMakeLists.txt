CMAKE_MINIMUM_REQUIRED(VERSION 2.6)
PROJECT(swm_zyre)

set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/../../cmake") # Find scripts for Zyre, etc.

ADD_DEFINITIONS(-DSWM_ZYRE_CONFIG_DIR="${CMAKE_HOME_DIRECTORY}") # folder for models (schemas)

FIND_PACKAGE(ZMQ REQUIRED) 
FIND_PACKAGE(CZMQ REQUIRED)
FIND_PACKAGE(ZYRE REQUIRED)
FIND_PACKAGE(jansson REQUIRED)

include_directories(
  ${ZMQ_INCLUDE_DIRS} 
  ${CZMQ_INCLUDE_DIRS}
  ${ZYRE_INCLUDE_DIRS}
  ${JANSSON_INCLUDE_DIRS}
)

# Compile library helper library swmzyre
add_library(swmzyre SHARED swmzyre.c)
target_link_libraries(swmzyre ${ZYRE_LIBRARIES} ${JANSSON_LIBRARIES})

# Install into system default
install(TARGETS swmzyre DESTINATION "lib" EXPORT swmzyre)
install(FILES swmzyre.h DESTINATION "include")

# Compile examples
add_executable(swm_zyre swm_zyre.c)
target_link_libraries(swm_zyre swmzyre ${CZMQ_LIBRARIES} ${ZYRE_LIBRARIES} ${JANSSON_LIBRARIES})

add_executable(sherpa_example sherpa_example.c)
target_link_libraries(sherpa_example swmzyre ${CZMQ_LIBRARIES} ${ZYRE_LIBRARIES} ${JANSSON_LIBRARIES})


