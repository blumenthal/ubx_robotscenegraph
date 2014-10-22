#include "rsg_reciever.hpp"

/* microblx type for the robot scene graph */
#include "types/rsg/types/rsg_types.h"

/* BRICS_3D includes */
#include <brics_3d/core/Logger.h>
#include <brics_3d/worldModel/WorldModel.h>
#include <brics_3d/worldModel/sceneGraph/DotVisualizer.h>
#include <brics_3d/worldModel/sceneGraph/HDF5UpdateDeserializer.h>

using namespace brics_3d;
using brics_3d::Logger;


UBX_MODULE_LICENSE_SPDX(BSD-3-Clause)

#define DEFAULT_HDF5_BUFFER_SIZE 20000

/* define a structure for holding the block local state. By assigning an
 * instance of this struct to the block private_data pointer (see init), this
 * information becomes accessible within the hook functions.
 */
struct rsg_reciever_info
{
        /* add custom block local data here */
		brics_3d::WorldModel* wm;
		brics_3d::rsg::DotVisualizer* wm_printer;
		brics_3d::rsg::HDF5UpdateDeserializer* wm_deserializer;

        /* this is to have fast access to ports for reading and writing, without
         * needing a hash table lookup */
        struct rsg_reciever_port_cache ports;

        unsigned char* hdf_5_input_buffer;		/* A buffer to buffer somplete HDF5 messgage. */
        unsigned long hdf_5_input_buffer_size;	/* Buffer size in bytes.
        										 * This depends on the data used and transmitted
        										 * 5-20.000 bytes are sufficinet for transform updates, etc,
        										 * More should be reserved for point clouds or mesh data.
        										 * E.g a Kinect consumes around 10,000,000 bytes per
        										 * message.
         	 	 	 	 	 	 	 	 	 	 */

};

/* init */
int rsg_reciever_init(ubx_block_t *b)
{
        int ret = -1;
        struct rsg_reciever_info *inf;

    	/* Configure the logger - default level won't tell us much */
    	brics_3d::Logger::setMinLoglevel(brics_3d::Logger::LOGDEBUG);

        /* allocate memory for the block local state */
        if ((inf = (struct rsg_reciever_info*)calloc(1, sizeof(struct rsg_reciever_info)))==NULL) {
                ERR("rsg_reciever: failed to alloc memory");
                ret=EOUTOFMEM;
                return ret;
        }
        b->private_data=inf;
        update_port_cache(b, &inf->ports);

       	unsigned int clen;
        rsg_wm_handle tmpWmHandle =  *((rsg_wm_handle*) ubx_config_get_data_ptr(b, "wm_handle", &clen));
        assert(clen != 0);
        inf->wm = reinterpret_cast<brics_3d::WorldModel*>(tmpWmHandle.wm); // We know that this pointer stores the world model type
        if(inf->wm == 0) {
//        	LOG(FATAL) << " World model handle could not be initialized.";
//        	return -1;
    		LOG(ERROR) << "World model handle could not be optained via configuration parameter."
    					  "Creating a new world model instance instead (mostly for debugging purposes)."
    					  "Please check your system design if this is intended!";
    		inf->wm = new brics_3d::WorldModel();
        }

        /* Attach debug graph printer */
        inf->wm_printer = new brics_3d::rsg::DotVisualizer(&inf->wm->scene);
        inf->wm_printer->setFileName("ubx_current_replica_graph");
        inf->wm->scene.attachUpdateObserver(inf->wm_printer);

        /* Attach deserializer (invoked at step function) */
        inf->wm_deserializer = new brics_3d::rsg::HDF5UpdateDeserializer(inf->wm);

        /* Setup input buffer for hdf5 messages */
        inf->hdf_5_input_buffer_size = *((uint32_t*) ubx_config_get_data_ptr(b, "buffer_len", &clen));
    	if((clen == 0) || (inf->hdf_5_input_buffer_size == 0)) {
            inf->hdf_5_input_buffer_size = DEFAULT_HDF5_BUFFER_SIZE;
    		LOG(WARNING) << "Invalid or missing configuration for buffer_len. "
    	                    "Falling back to default value buffer_len = " << DEFAULT_HDF5_BUFFER_SIZE;
    	}
    	LOG(DEBUG) << "HDF5 input buffer len set to " << inf->hdf_5_input_buffer_size;

        if((inf->hdf_5_input_buffer = (unsigned char *)malloc(inf->hdf_5_input_buffer_size)) == NULL) {
          ERR("failed to allocate hdf5 input buffer");
          free(inf->hdf_5_input_buffer);
          return 0;
        }

        return 0;
}

/* start */
int rsg_reciever_start(ubx_block_t *b)
{
        /* struct rsg_reciever_info *inf = (struct rsg_reciever_info*) b->private_data; */
        int ret = 0;
        return ret;
}

/* stop */
void rsg_reciever_stop(ubx_block_t *b)
{
        /* struct rsg_reciever_info *inf = (struct rsg_reciever_info*) b->private_data; */
}

/* cleanup */
void rsg_reciever_cleanup(ubx_block_t *b)
{
        struct rsg_reciever_info *inf = (struct rsg_reciever_info*) b->private_data;
        free(inf->hdf_5_input_buffer);
        free(b->private_data);
}

/* step */
void rsg_reciever_step(ubx_block_t *b)
{

        struct rsg_reciever_info *inf = (struct rsg_reciever_info*) b->private_data;
        LOG(INFO) << "rsg_reciever: Processing an incoming update";

		/* read data */
		ubx_port_t* port = inf->ports.rsg_in;
		assert(port != 0);

		ubx_data_t msg;
		checktype(port->block->ni, port->in_type, "unsigned char", port->name, 1);
		msg.type = port->in_type;
		msg.len = inf->hdf_5_input_buffer_size;
		msg.data = (void *)inf->hdf_5_input_buffer;
		int readBytes = __port_read(port, &msg);
		LOG(DEBUG) << "rsg_reciever: Port returned " << readBytes <<
                      " bytes, while data message length is " << msg.len <<
                      " bytes. Resulting size = " << data_size(&msg);

		const char *dataBuffer = (char *)msg.data;
		int transferred_bytes;
		if ((dataBuffer!=0) && (msg.len > 1) && (readBytes > 1)) {
			inf->wm_deserializer->write(dataBuffer, msg.len, transferred_bytes);
			LOG(DEBUG) << "rsg_reciever: \t transferred_bytes = " << transferred_bytes;
		} else if (dataBuffer == 0) {
			LOG(ERROR) << "Pointer to data buffer is zero. Aborting this update.";
		} else {
			LOG(ERROR) << "Incoming updare has not enough data to be processed. Aborting this update.";
		}

}

