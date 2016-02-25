#!/bin/bash

# set up some environtment scripts
source $FBX_MODULES/env.sh

export SWM_WMA_NAME=hmi_view
export SWM_WMA_ID=e379121f-06c6-4e21-ae9d-ae78ec1986a1
#export SWM_WMA_ID=631488ca-8b8f-4ce7-b149-997d6e0ed1cf

# configure ports
# reassign JSON ports 
export SWM_LOCAL_JSON_IN_PORT=12931
export SWM_LOCAL_JSON_OUT_PORT=12932
# individual query server port
export SWM_LOCAL_JSON_QUERY_PORT=22442

# swap the inter robot communication ports
export SWM_LOCAL_OUT_PORT=11611 # this is ME
export SWM_REMOTE_OUT_PORT=11511
export SWM_REMOTE_OUT_PORT_SECONDARY=11411

# activate osm filter
export SWM_ENABLE_INPUT_FILTER=1
#export SWM_INPUT_FILTER_PATTERN=(osm|osg)

# Start the ubx system
exec $UBX_ROOT/tools/ubx_launch -webif 8890 -c rsg_robot_1.usc

