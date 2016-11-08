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

add_executable(swm_zyre swm_zyre.c)
target_link_libraries(swm_zyre swmzyre ${CZMQ_LIBRARIES} ${ZYRE_LIBRARIES} ${JANSSON_LIBRARIES})

add_executable(add_victim_example add_victim_example.c)
target_link_libraries(add_victim_example swmzyre ${CZMQ_LIBRARIES} ${ZYRE_LIBRARIES} ${JANSSON_LIBRARIES})
