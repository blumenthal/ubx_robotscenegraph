#!/bin/bash

# set up some environtment scripts
source $FBX_MODULES/env.sh


# configure ports
# reassign JSON ports 
export SWM_LOCAL_JSON_IN_PORT=12921
export SWM_LOCAL_JSON_OUT_PORT=12922
# individual query server port
export SWM_LOCAL_JSON_QUERY_PORT=22432

# swap the inter robot communication ports
export SWM_LOCAL_OUT_PORT=11511 # this is ME
export SWM_REMOTE_OUT_PORT=11411
export SWM_REMOTE_OUT_PORT_SECONDARY=11611

# activate osm filter
export SWM_ENABLE_INPUT_FILTER=1


# Start the ubx system
exec $UBX_ROOT/tools/ubx_launch -webif 8889 -c rsg_robot_1.usc
#exec $UBX_ROOT/tools/ubx_launch -webif 8889 -c rsg_robot_2.usc	
