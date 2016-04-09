#!/bin/bash
#
# Script that starts the SWM in one shot.
# Sacrifices the ability to use the interactive
# console but still usefull for autstart scripts.
#

./run_sherpa_world_model.sh  <<-EOF

-- Set all relevant functions to be called here:
start_wm()
start_auto_sync()


-- Keeps the system alive by blocking the interacive console: 
--while(true) do io.stdin:read'*l' end
while(true) do 
  os.execute("sleep 1") -- ignores CTRL C, but puts no extra load
end

EOF
