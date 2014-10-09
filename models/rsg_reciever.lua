return block
{
      name="rsg_reciever",
      meta_data="",
      port_cache=true,

      configurations= {
	{ name="wm_handle", type_name = "struct rsg_wm_handle", doc="Handle to the world wodel instance. This parameter is mandatory."},
      },

      ports = {
	 { name="rsg_in", in_type_name="unsigned char", doc="HDF5 based byte stream for updates on RSG based world model." },
      },
      
      operations = { start=true, stop=true, step=true },
      
      cpp=true
}
