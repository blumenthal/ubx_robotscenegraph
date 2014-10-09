return block
{
      name="rsg_sender",
      meta_data="",
      port_cache=true,

      configurations = {
	{ name="wm_handle", type_name = "struct rsg_wm_handle", doc="Handle to the world wodel instance. This parameter is mandatory."},
      },

      ports = {
	 { name="rsg_out", out_type_name="unsigned char", doc="HDF5 based byte stream for updates on RSG based world model." },
      },
      
      operations = { start=true, stop=true, step=true },
      
      cpp=true
}
