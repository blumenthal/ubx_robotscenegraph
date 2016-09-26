#!/bin/bash

# set up some environtment scripts
source $FBX_MODULES/env.sh

# Example configuration for a HMI. Note that all varibles have default values,
# that get overridden by this script.
# Assumption: wasp and sherpa_box are launched on the same PC with an IP = 192.168.1.101
# while the HMI is launched on a second PC with IP = 192.168.1.104

# Logger
export SWM_LOG_LEVEL=0  #  default value = 1

# World Model Agent 
export SWM_WMA_NAME=hmi   #  default value = swm
export SWM_WMA_ID=5c30349a-a474-4a01-8678-d3a646ee08d8  # default value = e379121f-06c6-4e21-ae9d-ae78ec1986a1
export SWM_WMA_ID= # An empty World Model Agent Id will trigger to generate one.
export SWM_GLOBAL_ID=e379121f-06c6-4e21-ae9d-ae78ec1986a1   # default value =

# Communication setup with ZMQ (version without mediator component)
export SWM_LOCAL_IP=localhost   # default value = localhost
export SWM_LOCAL_OUT_PORT=11411 # default value = 11411
export SWM_REMOTE_IP=192.168.1.101   #default value = 127.0.0.1
export SWM_REMOTE_OUT_PORT=11411  # default value = 11511
export SWM_LOCAL_JSON_IP=127.0.0.1   #default value = 127.0.0.1
export SWM_LOCAL_JSON_IN_PORT=12911  #default value = 12911
export SWM_LOCAL_JSON_IN_PORT_SECONDARY=13911  #default value = 13911
export SWM_LOCAL_JSON_OUT_PORT=12912   #  default value = 12912
export SWM_LOCAL_JSON_QUERY_PORT=22424  # default value = 22422
export SWM_REMOTE_IP_SECONDARY=192.168.1.101 # default value = 127.0.0.1
export SWM_REMOTE_OUT_PORT_SECONDARY=11511 # default value = 11611

# Filter settings (optional)
export SWM_ENABLE_INPUT_FILTER=0 # default value = 0
export SWM_INPUT_FILTER_PATTERN= # default value = os(m|g)
export SWM_MAX_TRANSFORM_FREQ=5 # default value = 5

# Map files settings (optional)
export SWM_RSG_MAP_FILE=../maps/rsg/cesena_lab.json # default value = examples/maps/rsg/cesena_lab.json
export SWM_OSM_MAP_FILE=../maps/osm/map_micro_champoluc.osm # default value = examples/maps/osm/map_micro_champoluc.osm

# Start the ubx system
exec $UBX_ROOT/tools/ubx_launch -webif 8890 -c sherpa_world_model_no_ros.usc	
