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
};

/* init */
int rsg_reciever_init(ubx_block_t *b)
{
        int ret = -1;
        struct rsg_reciever_info *inf;

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
        	LOG(FATAL) << " World model handle could not be initialized.";
        	return -1;
        }

        /* Attach debug graph printer */
        inf->wm_printer = new brics_3d::rsg::DotVisualizer(&inf->wm->scene);
        inf->wm_printer->setFileName("ubx_current_replica_graph");
        inf->wm->scene.attachUpdateObserver(inf->wm_printer);

//        /* Attach the UBX port to the world model */
//        ubx_type_t* type =  ubx_type_get(b->ni, "unsigned char");
//        RsgToUbxPort* wmUpdatesUbxPort = new RsgToUbxPort(inf->ports.rsg_out, type);
//        brics_3d::rsg::HDF5UpdateSerializer* wmUpdatesToHdf5Serializer = new brics_3d::rsg::HDF5UpdateSerializer(wmUpdatesUbxPort);
//        inf->wm->scene.attachUpdateObserver(wmUpdatesToHdf5Serializer);

        inf->wm_deserializer = new brics_3d::rsg::HDF5UpdateDeserializer(inf->wm);

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
		msg.len = 1;
		__port_read(port, &msg);

		const char *dataBuffer = (char *)msg.data;
		int transferred_bytes;
		LOG(ERROR) << "rsg_reciever: deserializing " << msg.len << " bytes.";
		LOG(ERROR) << " data_size = " << data_size(&msg);
		if (dataBuffer!=0 && msg.len > 1) {
			inf->wm_deserializer->write(dataBuffer, msg.len, transferred_bytes);
		}

}

