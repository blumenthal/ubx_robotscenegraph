#!/bin/bash

# set up some environtment scripts
source $FBX_MODULES/env.sh

# configure ports
# swap JSON 
export SWM_LOCAL_JSON_IN_PORT=12912
export SWM_LOCAL_JSON_OUT_PORT=12911
# individual query server port
export SWM_LOCAL_JSON_QUERY_PORT=22423
# also swap the HDF5 ports
export SWM_LOCAL_OUT_PORT=11511
export SWM_REMOTE_OUT_PORT=11411

# Start the ubx system
#exec $UBX_ROOT/tools/ubx_launch -webif 8889 -c rsg_robot_1.usc
exec $UBX_ROOT/tools/ubx_launch -webif 8889 -c rsg_robot_2.usc	
