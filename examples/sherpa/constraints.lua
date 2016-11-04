print("Adding graph constraints")
-- Constraints for sending data:
wm:addNodeAttribute(rootId, "rsg:agent_policy", "send no PointClouds"); -- (default) exclude point cloud nodes. In SHERPA we use the Mediator + file transfer instead 
--wm:addNodeAttribute(rootId, "rsg:agent_policy", "send no Atoms from context osm"); 
--wm:addNodeAttribute(rootId, "rsg:agent_policy", "send no Connections"); 
wm:addNodeAttribute(rootId, "rsg:agent_policy", "send no Transforms with freq > 5 Hz"); -- (default) highly recommended limit on the update frequency  

-- Constraint for rceiving data:
--wm:addNodeAttribute(rootId, "rsg:agent_policy", "receive no Atoms from context osm"); -- use this to exclude OpenStreetMap data
--wm:addNodeAttribute(rootId, "rsg:agent_policy", "receive no Connections"); 
--wm:addNodeAttribute(rootId, "rsg:agent_policy", "receive no Connections from context osm"); -- this one still does not work