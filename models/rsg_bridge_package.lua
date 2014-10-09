return pkg
{
  name="microblx_rsg_bridge",
  path="../",
      
  dependencies = {
    { name="ros_bridge", type="cmake" },
    { name="rsg_types", type="cmake" },
  },
  
  
  blocks = {
    { name="rsg_sender", file="examples/rsg_sender.lua", src_dir="src" },
    { name="rsg_reciever", file="examples/rsg_reciever.lua", src_dir="src" },
  },
  
  libraries = {
    { name="rsgsenderlib", blocks={"rsg_sender"} },
    { name="rsgrecieverlib", blocks={"rsg_reciever"} },
  },  
}
