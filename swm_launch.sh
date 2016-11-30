#!/bin/bash
#
# Script that starts the SWM in one shot.
# Sacrifices the ability to use the interactive
# console but still usefull for autstart scripts.
#

cat swm_launch.sh - | ./run_sherpa_world_model.sh $1
  
s()
scene_setup()
