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
export SWM_WMA_NAME=uav2   #  default value = swm
export SWM_WMA_ID=5c30349a-a474-4a01-8678-d3a646ee08d8  # default value = e379121f-06c6-4e21-ae9d-ae78ec1986a1
export SWM_WMA_ID= # An empty World Model Agent Id will trigger to generate one.
export SWM_GLOBAL_ID=e379121f-06c6-4e21-ae9d-ae78ec1986a1   # default value =

# Communication setup with ZMQ (version without mediator component)
#export SWM_USE_GOSSIP=0 # 1 use gossip; 0 use UDP beaconing;
#export SWM_BIND_ZYRE=0  # 1 => this SWM "binds". There must be exactly one Zyre nore that binds. In case a Mediator is used, it will bind. Thus 0 is default  
export SWM_LOCAL_JSON_QUERY_PORT=22424  # default value = 22422

export SWM_GOSSIP_ENDPOINT=ipc:///tmp/uav2-hub
export SWM_USE_GOSSIP=1
export SWM_ZYRE_GROUP=uav2-local


# Filter settings (optional)
export SWM_ENABLE_INPUT_FILTER=1 # default value = 0
export SWM_CONSTRAINTS_FILE=constraints.lua
#export SWM_CONSTRAINTS_FILE= #empty string turns it off
export SWM_MAX_TRANSFORM_FREQ=5 # default value = 5

# Map files settings (optional)
export SWM_RSG_MAP_FILE=../maps/rsg/sherpa_basic_mission_setup.json # default value = examples/maps/rsg/cesena_lab.json
export SWM_OSM_MAP_FILE=../maps/osm/map_micro_champoluc.osm # default value = examples/maps/osm/map_micro_champoluc.osm

# Start the ubx system
exec $UBX_ROOT/tools/ubx_launch -webif 8890 -c sherpa_world_model_no_ros.usc	
