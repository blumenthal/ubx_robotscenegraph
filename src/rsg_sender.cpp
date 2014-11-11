#include "rsg_sender.hpp"

/* microblx type for the robot scene graph */
#include "types/rsg/types/rsg_types.h"

/* BRICS_3D includes */
#include <brics_3d/core/Logger.h>
#include <brics_3d/worldModel/WorldModel.h>
#include <brics_3d/worldModel/sceneGraph/DotVisualizer.h>
#include <brics_3d/worldModel/sceneGraph/HDF5UpdateSerializer.h>
#include <brics_3d/worldModel/sceneGraph/SceneGraphToUpdatesTraverser.h>
#include <brics_3d/worldModel/sceneGraph/FrequencyAwareUpdateFilter.h>


using namespace brics_3d;
using brics_3d::Logger;


UBX_MODULE_LICENSE_SPDX(BSD-3-Clause)

/* hexdump a buffer */
//static void hexdump(unsigned char *buf, unsigned long index, unsigned long width)
//{
//	unsigned long i, fill;
//
//	for (i=0;i<index;i++) { printf("%02x ", buf[i]); } /* dump data */
//	for (fill=index; fill<width; fill++) printf(" ");  /* pad on right */
//
//	printf(": ");
//
//	/* add ascii repesentation */
//	for (i=0; i<index; i++) {
//		if (buf[i] < 32) printf(".");
//		else printf("%c", buf[i]);
//	}
//	printf("\n");
//}


/*
 * Implementation of data transmission.
 */
class RsgToUbxPort : public brics_3d::rsg::IOutputPort {
public:
	RsgToUbxPort(ubx_port_t* port, ubx_type_t* type) : port(port), type(type){};
	virtual ~RsgToUbxPort(){};

	int write(const char *dataBuffer, int dataLength, int &transferredBytes) {
		LOG(INFO) << "RsgToUbxPort: Feeding data forwards.";
		assert(port != 0);

		ubx_data_t msg;
		msg.data = (void *)dataBuffer;
		msg.len = dataLength;
		msg.type = type;

		LOG(INFO) << "Sending " << msg.len << " bytes: ";
//		hexdump((unsigned char *)msg.data, msg.len, 16);
		__port_write(port, &msg);

		return 0;
	};

private:
	ubx_port_t* port;
	ubx_type_t* type;
};



/* define a structure for holding the block local state. By assigning an
 * instance of this struct to the block private_data pointer (see init), this
 * information becomes accessible within the hook functions.
 */
struct rsg_sender_info
{
        /* add custom block local data here */
		brics_3d::WorldModel* wm;
		brics_3d::rsg::DotVisualizer* wm_printer;
		brics_3d::rsg::SceneGraphToUpdatesTraverser* wm_resender;
		brics_3d::rsg::FrequencyAwareUpdateFilter* frequency_filter;

        /* this is to have fast access to ports for reading and writing, without
         * needing a hash table lookup */
        struct rsg_sender_port_cache ports;
};

/* init */
int rsg_sender_init(ubx_block_t *b)
{
        int ret = -1;
        struct rsg_sender_info *inf;

    	/* Configure the logger - default level won't tell us much */
    	brics_3d::Logger::setMinLoglevel(brics_3d::Logger::LOGDEBUG);

        /* allocate memory for the block local state */
        if ((inf = (struct rsg_sender_info*)calloc(1, sizeof(struct rsg_sender_info)))==NULL) {
                ERR("rsg_sender: failed to alloc memory");
                ret=EOUTOFMEM;
                return -1;
        }
        b->private_data=inf;
        update_port_cache(b, &inf->ports);

    	unsigned int clen;
    	rsg_wm_handle tmpWmHandle =  *((rsg_wm_handle*) ubx_config_get_data_ptr(b, "wm_handle", &clen));
    	assert(clen != 0);
    	inf->wm = reinterpret_cast<brics_3d::WorldModel*>(tmpWmHandle.wm); // We know that this pointer stores the world model type
    	if(inf->wm == 0) {
//    		LOG(FATAL) << " World model handle could not be initialized.";
    		//return -1;
    		LOG(ERROR) << "World model handle could not be optained via configuration parameter."
    					  "Creating a new world model instance instead (mostly for debugging purposes)."
    					  "Please check your system design if this is intended!";
    		inf->wm = new brics_3d::WorldModel();
    	}

    	/* Attach debug graph printe */
    	inf->wm_printer = new brics_3d::rsg::DotVisualizer(&inf->wm->scene);
    	inf->wm->scene.attachUpdateObserver(inf->wm_printer);

    	inf->frequency_filter = new brics_3d::rsg::FrequencyAwareUpdateFilter();
    	inf->frequency_filter->setMaxGeometricNodeUpdateFrequency(0); // everything;
    	inf->frequency_filter->setMaxTransformUpdateFrequency(1); // not more then x Hz;

    	/* Attach the UBX port to the world model */
    	ubx_type_t* type =  ubx_type_get(b->ni, "unsigned char");
    	RsgToUbxPort* wmUpdatesUbxPort = new RsgToUbxPort(inf->ports.rsg_out, type);
    	brics_3d::rsg::HDF5UpdateSerializer* wmUpdatesToHdf5Serializer = new brics_3d::rsg::HDF5UpdateSerializer(wmUpdatesUbxPort);
//    	inf->wm->scene.attachUpdateObserver(wmUpdatesToHdf5Serializer);
    	inf->wm->scene.attachUpdateObserver(inf->frequency_filter);
    	inf->frequency_filter->attachUpdateObserver(wmUpdatesToHdf5Serializer);


    	/* Set error policy of RSG */
    	inf->wm->scene.setCallObserversEvenIfErrorsOccurred(false);

    	/* Initialize resender that resends the complete graph, if necessary */
    	inf->wm_resender = new brics_3d::rsg::SceneGraphToUpdatesTraverser(wmUpdatesToHdf5Serializer);

        return 0;
}

/* start */
int rsg_sender_start(ubx_block_t *b)
{
        /* struct rsg_sender_info *inf = (struct rsg_sender_info*) b->private_data; */
        int ret = 0;
        return ret;
}

/* stop */
void rsg_sender_stop(ubx_block_t *b)
{
        /* struct rsg_sender_info *inf = (struct rsg_sender_info*) b->private_data; */
}

/* cleanup */
void rsg_sender_cleanup(ubx_block_t *b)
{
        struct rsg_sender_info *inf = (struct rsg_sender_info*) b->private_data;
        if(inf->wm_printer) {
        	delete inf->wm_printer;
        	inf->wm_printer = 0;
        }
        if(inf->wm_resender) {
        	delete inf->wm_resender;
        	inf->wm_resender = 0;
        }
        free(b->private_data);
}

/* step */
void rsg_sender_step(ubx_block_t *b)
{

        struct rsg_sender_info *inf = (struct rsg_sender_info*) b->private_data;
        brics_3d::WorldModel* wm = inf->wm;

    	/* Add group nodes */
//    	std::vector<brics_3d::rsg::Attribute> attributes;
//    	attributes.clear();
//    	attributes.push_back(rsg::Attribute("taskType", "scene_objecs"));
//    	rsg::Id sceneObjectsId;
//    	wm->scene.addGroup(wm->getRootNodeId(), sceneObjectsId, attributes);
//    	wm->scene.addGroup(wm->getRootNodeId(), sceneObjectsId, attributes);

        /* Resend the complete scene graph */
        LOG(INFO) << "Resending the complete RSG now.";
        inf->wm_resender->reset();
        wm->scene.executeGraphTraverser(inf->wm_resender, wm->scene.getRootId());

}

