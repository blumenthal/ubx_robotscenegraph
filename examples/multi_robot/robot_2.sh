#!/bin/bash

# set up some environtment scripts
source $FBX_MODULES/env.sh

export SWM_WMA_NAME=uav
export SWM_WMA_ID=e379121f-06c6-4e21-ae9d-ae78ec1986a1

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
