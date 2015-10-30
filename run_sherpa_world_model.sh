#!/bin/bash

# set up some environtment scripts
source $FBX_MODULES/env.sh

# Start the ubx system
if [ "$1" = "--no-ros" ]; then
  # Version without ROS bridge
  exec $UBX_ROOT/tools/ubx_launch -webif 8888 -c examples/sherpa/sherpa_world_model_no_ros.usc
elif [ "$1" != "" ]; then
  # Version with explicit system composition model
	exec $UBX_ROOT/tools/ubx_launch -webif 8888 -c ${1}
else
  # Default (full-fledged) system.
  exec $UBX_ROOT/tools/ubx_launch -webif 8888 -c examples/sherpa/sherpa_world_model.usc
fi

