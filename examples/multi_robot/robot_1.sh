#!/bin/bash

# set up some environtment scripts
source $FBX_MODULES/env.sh

export SWM_WMA_NAME=ugv_view
export SWM_WMA_ID=e379121f-06c6-4e21-ae9d-ae78ec1986a1

# Start the ubx system
exec $UBX_ROOT/tools/ubx_launch -webif 8888 -c rsg_robot_1.usc	
