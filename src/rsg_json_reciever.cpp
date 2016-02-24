#include "rsg_json_reciever.hpp"

/* microblx type for the robot scene graph */
#include "types/rsg/types/rsg_types.h"

/* BRICS_3D includes */
#include <brics_3d/core/Logger.h>
#include <brics_3d/worldModel/WorldModel.h>
#include <brics_3d/worldModel/sceneGraph/DotVisualizer.h>
#include <brics_3d/worldModel/sceneGraph/HDF5UpdateDeserializer.h>
#include <brics_3d/worldModel/sceneGraph/JSONDeserializer.h>
#include <brics_3d/worldModel/sceneGraph/SemanticContextUpdateFilter.h>
#include <brics_3d/worldModel/sceneGraph/UpdatesToSceneGraphListener.h>
#include <brics_3d/worldModel/sceneGraph/RemoteRootNodeAutoMounter.h>

using namespace brics_3d;
using brics_3d::Logger;


UBX_MODULE_LICENSE_SPDX(BSD-3-Clause)

#define DEFAULT_HDF5_BUFFER_SIZE 20000

/* define a structure for holding the block local state. By assigning ano
 * instance of this struct to the block private_data pointer (see init), this
 * information becomes accessible within the hook functions.
 */
struct rsg_json_reciever_info
{
        /* add custom block local data here */
		brics_3d::WorldModel* wm;
		brics_3d::rsg::DotVisualizer* wm_printer;
		brics_3d::rsg::JSONDeserializer* wm_deserializer;
		brics_3d::rsg::SemanticContextUpdateFilter* wm_input_filter; // optional
		brics_3d::rsg::UpdatesToSceneGraphListener* wm_updates_to_wm; // optional
		brics_3d::rsg::RemoteRootNodeAutoMounter* wm_auto_mounter;

        /* this is to have fast access to ports for reading and writing, without
         * needing a hash table lookup */
        struct rsg_json_reciever_port_cache ports;

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
int rsg_json_reciever_init(ubx_block_t *b)
{
        int ret = -1;
        struct rsg_json_reciever_info *inf;

    	/* Configure the logger - default level won't tell us much */
    	brics_3d::Logger::setMinLoglevel(brics_3d::Logger::LOGDEBUG);

        /* allocate memory for the block local state */
        if ((inf = (struct rsg_json_reciever_info*)calloc(1, sizeof(struct rsg_json_reciever_info)))==NULL) {
                ERR("rsg_json_reciever: failed to alloc memory");
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

        bool inputFilterIsEnabled = false;
        int* enable_input_filter =  ((int*) ubx_config_get_data_ptr(b, "enable_input_filter", &clen));
        if(clen == 0) {
        	LOG(INFO) << "rsg_json_reciever: No enable_input_filter configuation given. Turned off by default.";
        } else {
        	if (*enable_input_filter == 1) {
        		LOG(INFO) << "rsg_json_reciever: enable_input_filter turned on.";
        		inputFilterIsEnabled = true;
        	} else {
        		LOG(INFO) << "rsg_json_reciever: enable_input_filter turned off.";
        		inputFilterIsEnabled = false;
        	}
        }
        LOG(INFO) << "rsg_json_reciever: enable_input_filter = " << inputFilterIsEnabled;

		/* retrive optional input_filter_pattern from config */
		std::string semanticContextIdentifier = "osm";
		char* chrptr = (char*) ubx_config_get_data_ptr(b, "input_filter_pattern", &clen);
		if(clen == 0) {
			LOG(INFO) << "rsg_json_reciever:  No input_filter_pattern configuation given. Selecting a default name.";
			semanticContextIdentifier = "osm";
		} else {
			if(strcmp(chrptr, "")==0) {
				LOG(INFO) << "rsg_json_reciever:  input_filter_pattern is empty. Selecting a default name.";
				semanticContextIdentifier = "osm";
			} else {
				std::string pattern(chrptr);
				LOG(INFO) << "rsg_json_reciever: Using input_filter_pattern = " << pattern;
				semanticContextIdentifier = pattern;

			}
		}
		LOG(INFO) << "rsg_json_reciever:  input_filter_pattern = " << semanticContextIdentifier;

        /* Attach debug graph printer */
        inf->wm_printer = new brics_3d::rsg::DotVisualizer(&inf->wm->scene);
        inf->wm_printer->setFileName("ubx_current_replica_graph");

        /* Use auto mount policy */
    	inf->wm_auto_mounter = new brics_3d::rsg::RemoteRootNodeAutoMounter(&inf->wm->scene, inf->wm->getRootNodeId()); //mount everything relative to root node
    	inf->wm->scene.attachUpdateObserver(inf->wm_auto_mounter);

        /* Attach deserializer (invoked at step function) */
    	if(inputFilterIsEnabled) {
    		inf->wm_input_filter = new brics_3d::rsg::SemanticContextUpdateFilter (&inf->wm->scene); // handle use for queries
    		inf->wm_updates_to_wm = new brics_3d::rsg::UpdatesToSceneGraphListener();
    		inf->wm_updates_to_wm->attachSceneGraph(&inf->wm->scene);
    		inf->wm_input_filter->attachUpdateObserver(inf->wm_updates_to_wm); // handle used for updates
    		inf->wm_input_filter->setNameSpaceIdentifier(semanticContextIdentifier);
    		LOG(INFO) << "rsg_json_reciever: filter enabled for semantic context identifier = " << semanticContextIdentifier;
            inf->wm_deserializer = new brics_3d::rsg::JSONDeserializer(inf->wm, inf->wm_input_filter); // Make the deserializer to call update on the filter
    	} else {
    		inf->wm_deserializer = new brics_3d::rsg::JSONDeserializer(inf->wm);
    		inf->wm_input_filter = 0;
    		inf->wm_updates_to_wm = 0;
    	}

        /* Setup input buffer for JSON messages */
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
int rsg_json_reciever_start(ubx_block_t *b)
{
        /* struct rsg_json_reciever_info *inf = (struct rsg_json_reciever_info*) b->private_data; */
        int ret = 0;

    	/* Set logger level */
    	unsigned int clen;
    	int* log_level =  ((int*) ubx_config_get_data_ptr(b, "log_level", &clen));
    	if(clen == 0) {
    		LOG(INFO) << "rsg_json_reciever: No log_level configuation given.";
    	} else {
    		if (*log_level == 0) {
    			LOG(INFO) << "rsg_json_reciever: log_level set to DEBUG level.";
    			brics_3d::Logger::setMinLoglevel(brics_3d::Logger::LOGDEBUG);
    		} else if (*log_level == 1) {
    			LOG(INFO) << "rsg_json_reciever: log_level set to INFO level.";
    			brics_3d::Logger::setMinLoglevel(brics_3d::Logger::INFO);
    		} else if (*log_level == 2) {
    			LOG(INFO) << "rsg_json_reciever: log_level set to WARNING level.";
    			brics_3d::Logger::setMinLoglevel(brics_3d::Logger::WARNING);
    		} else if (*log_level == 3) {
    			LOG(INFO) << "rsg_json_reciever: log_level set to LOGERROR level.";
    			brics_3d::Logger::setMinLoglevel(brics_3d::Logger::LOGERROR);
    		} else if (*log_level == 4) {
    			LOG(INFO) << "rsg_json_reciever: log_level set to FATAL level.";
    			brics_3d::Logger::setMinLoglevel(brics_3d::Logger::FATAL);
    		} else {
    			LOG(INFO) << "rsg_json_reciever: unknown log_level = " << *log_level;		}
    	}

        return ret;
}

/* stop */
void rsg_json_reciever_stop(ubx_block_t *b)
{
        /* struct rsg_json_reciever_info *inf = (struct rsg_json_reciever_info*) b->private_data; */
}

/* cleanup */
void rsg_json_reciever_cleanup(ubx_block_t *b)
{
		struct rsg_json_reciever_info *inf = (struct rsg_json_reciever_info*) b->private_data;
		if(inf->wm_input_filter != 0) {
			delete inf->wm_input_filter;
			inf->wm_input_filter = 0;
		}
		if(inf->wm_updates_to_wm != 0){
			delete inf->wm_updates_to_wm;
			inf->wm_updates_to_wm = 0;
		}
        free(inf->hdf_5_input_buffer);
        free(b->private_data);
}

/* step */
void rsg_json_reciever_step(ubx_block_t *b)
{

        struct rsg_json_reciever_info *inf = (struct rsg_json_reciever_info*) b->private_data;
        LOG(DEBUG) << "rsg_json_reciever: Processing an incoming update";

		/* read data */
		ubx_port_t* port = inf->ports.rsg_in;
		assert(port != 0);

		ubx_data_t msg;
		checktype(port->block->ni, port->in_type, "unsigned char", port->name, 1);
		msg.type = port->in_type;
		msg.len = inf->hdf_5_input_buffer_size;
		msg.data = (void *)inf->hdf_5_input_buffer;
		int readBytes = __port_read(port, &msg);
		LOG(DEBUG) << "rsg_json_reciever: Port returned " << readBytes <<
                      " bytes, while data message length is " << msg.len <<
                      " bytes. Resulting size = " << data_size(&msg);

		const char *dataBuffer = (char *)msg.data;
		int transferred_bytes;
		if ((dataBuffer!=0) && (msg.len > 1) && (readBytes > 1)) {
			inf->wm_deserializer->write(dataBuffer, readBytes, transferred_bytes);
			LOG(INFO) << "rsg_json_reciever: \t transferred_bytes = " << transferred_bytes;
		} else if (dataBuffer == 0) {
			LOG(ERROR) << "rsg_json_reciever: Pointer to data buffer is zero. Aborting this update.";
		} else {
			LOG(DEBUG) << "rsg_json_reciever: Incoming updare has not enough data to be processed. Aborting this update.";
		}

}

