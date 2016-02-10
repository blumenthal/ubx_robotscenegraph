#!/bin/bash

# set up some environtment scripts
source $FBX_MODULES/env.sh


# Retrive software version information to ease debugging.
echo "Version of BRICS_3D:"
git --git-dir=${BRICS_3D_DIR}/.git --work-tree=${BRICS_3D_DIR} describe --abbrev=8 --always --dirty

echo "Version of FBX:"
git --git-dir=${FBX_MODULES}/.git --work-tree=${FBX_MODULES} describe --abbrev=8 --always --dirty

echo "Version of UBX_RSG:"
git describe --abbrev=8 --always --dirty 


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

