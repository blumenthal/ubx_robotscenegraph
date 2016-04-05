#!/bin/bash

# set up some environtment scripts
source $FBX_MODULES/env.sh

# Example configuration for a sherpa_box. Note that all varibles have default values,
# that get overridden by this script.
# Assumption: wasp and sherpa_box are launched on the same PC with an IP = 192.168.1.101
# while the HMI is launched on a second PC with IP = 192.168.1.104

# Logger
export SWM_LOG_LEVEL=0  #  default value = 1

# World Model Agent 
export SWM_WMA_NAME=sherpa_box   #  default value = swm
export SWM_WMA_ID=c253cebe-9b00-4c77-9f07-9520023bd122  # default value = e379121f-06c6-4e21-ae9d-ae78ec1986a1
export SWM_GLOBAL_ID=e2d399ac-b4ee-4ba4-acad-91709b522bc5   # default value =

# Communication setup with ZMQ (version without mediator component)
export SWM_LOCAL_IP=localhost   # default value = localhost
export SWM_LOCAL_OUT_PORT=11511 # default value = 11411
export SWM_REMOTE_IP=127.0.0.1   #default value = 127.0.0.1
export SWM_REMOTE_OUT_PORT=11411  # default value = 11511
export SWM_LOCAL_JSON_IP=127.0.0.1   #default value = 127.0.0.1
export SWM_LOCAL_JSON_IN_PORT=12914  #default value = 12911
export SWM_LOCAL_JSON_IN_PORT_SECONDARY=13911  #default value = 13911
export SWM_LOCAL_JSON_OUT_PORT=12914   #  default value = 12912
export SWM_LOCAL_JSON_QUERY_PORT=22423  # default value = 22422
export SWM_REMOTE_IP_SECONDARY=192.168.1.104 # default value = 127.0.0.1
export SWM_REMOTE_OUT_PORT_SECONDARY=11411 # default value = 11611

# Filter settings (optional)
export SWM_ENABLE_INPUT_FILTER=0 # default value = 0
export SWM_INPUT_FILTER_PATTERN= # default value = os(m|g)

# Map files settings (optional)
export SWM_RSG_MAP_FILE=../maps/rsg/cesena_lab.json # default value = examples/maps/rsg/cesena_lab.json
export SWM_OSM_MAP_FILE=../maps/osm/map_micro_champoluc.osm # default value = examples/maps/osm/map_micro_champoluc.osm

# Start the ubx system
exec $UBX_ROOT/tools/ubx_launch -webif 8889 -c sherpa_world_model.usc	
