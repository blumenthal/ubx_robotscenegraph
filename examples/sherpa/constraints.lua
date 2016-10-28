print("Adding graph constraints")
-- Constraints for sending data:
wm:addNodeAttribute(rootId, "rsg:agent_policy", "send no PointClouds");
--wm:addNodeAttribute(rootId, "rsg:agent_policy", "send no Atoms from context osm"); 
--wm:addNodeAttribute(rootId, "rsg:agent_policy", "send no Connections"); 
wm:addNodeAttribute(rootId, "rsg:agent_policy", "send no Transforms with freq > 5 Hz");  

-- Constraint for rceiving data:
--wm:addNodeAttribute(rootId, "rsg:agent_policy", "send no Connections from context osm"); 
--wm:addNodeAttribute(rootId, "rsg:agent_policy", "receive no Atoms from context osm");
