#!/bin/bash

# set up some environtment scripts
source $FBX_MODULES/env.sh

# Start the ubx system
exec $UBX_ROOT/tools/ubx_launch -webif 8890 -c sherpa_world_model_simulated_robot_3.usc	
