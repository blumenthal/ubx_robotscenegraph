#!/bin/bash

# pointers to the relveant modules (this has to be needed to be adopted for individual systms)
#export UBX_MODULES=/opt/src/sandbox/microblx/microblx_ros_bridge
#export FBX_MODULES=/opt/src/sandbox/brics_3d_function_blocks

# set up some other environtment scripts
#source $UBX_ROOT/env.sh
source $FBX_MODULES/env.sh

# Start the micoiblox system
exec $UBX_ROOT/tools/ubx_launch -webif 8888 -c rsgbridgetest2.usc
