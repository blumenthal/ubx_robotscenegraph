#!/bin/bash

# set up some environtment scripts
source $FBX_MODULES/env.sh

# Start the ubx system
exec $UBX_ROOT/tools/ubx_launch -webif 8889 -c rsg_robot_2.usc	
