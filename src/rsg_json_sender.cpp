#include "rsg_json_sender.hpp"

/* microblx type for the robot scene graph */
#include "types/rsg/types/rsg_types.h"

/* BRICS_3D includes */
#include <brics_3d/core/Logger.h>
#include <brics_3d/worldModel/WorldModel.h>
#include <brics_3d/worldModel/sceneGraph/DotVisualizer.h>
#include <brics_3d/worldModel/sceneGraph/JSONSerializer.h>
#include <brics_3d/worldModel/sceneGraph/SceneGraphToUpdatesTraverser.h>
#include <brics_3d/worldModel/sceneGraph/RootFinder.h>
#include <brics_3d/worldModel/sceneGraph/FrequencyAwareUpdateFilter.h>
#include <brics_3d/worldModel/sceneGraph/ISceneGraphUpdateObserver.h>


using namespace brics_3d;
using brics_3d::Logger;
using namespace brics_3d::rsg;

UBX_MODULE_LICENSE_SPDX(BSD-3-Clause)

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
		__port_write(port, &msg);

		return 0;
	};

private:
	ubx_port_t* port;
	ubx_type_t* type;
};

/**
 * Triggers block b whenever a addRemoteRootNode event is detected.
 */
class RemoteRootNodeAdditionTrigger : public brics_3d::rsg::ISceneGraphUpdateObserver {
public:

	/**
	 * Construrctor with block to be triggereg.
	 */
	RemoteRootNodeAdditionTrigger(SceneGraphFacade* observedScene, ubx_block_t *b) : observedScene(observedScene), b(b){};
	virtual ~RemoteRootNodeAdditionTrigger(){};

	/* implemetntations of observer interface */
	bool addNode(Id parentId, Id& assignedId, vector<Attribute> attributes, bool forcedId = false){return true;};
	bool addGroup(Id parentId, Id& assignedId, vector<Attribute> attributes, bool forcedId = false){return true;};
	bool addTransformNode(Id parentId, Id& assignedId, vector<Attribute> attributes, IHomogeneousMatrix44::IHomogeneousMatrix44Ptr transform, TimeStamp timeStamp, bool forcedId = false){return true;};
    bool addUncertainTransformNode(Id parentId, Id& assignedId, vector<Attribute> attributes, IHomogeneousMatrix44::IHomogeneousMatrix44Ptr transform, ITransformUncertainty::ITransformUncertaintyPtr uncertainty, TimeStamp timeStamp, bool forcedId = false){return true;};
	bool addGeometricNode(Id parentId, Id& assignedId, vector<Attribute> attributes, Shape::ShapePtr shape, TimeStamp timeStamp, bool forcedId = false){return true;};
	bool addRemoteRootNode(Id rootId, vector<Attribute> attributes){
		LOG(DEBUG) << "RemoteRootNodeAdditionTrigger: addRemoteRootNode detected";

		/*
		 * Only repond to _other_ World Model Agnets that advertise their root nodes.
		 * So we have to filter out the "own" root node. Other wise a loop is created,
		 * because part of the triggering could (and typicall will) inclide yet another
		 * addRemoteRootNode().
		 */
		if(rootId != observedScene->getRootId()) {
			LOG(DEBUG) << "RemoteRootNodeAdditionTrigger: tiggering now.";
			b->step(b); // A single step.
		} else {
			LOG(DEBUG) << "RemoteRootNodeAdditionTrigger: Skipping addRemoteRootNode from local graph.";
		}

		return true;
	};
	bool addConnection(Id parentId, Id& assignedId, vector<Attribute> attributes, vector<Id> sourceIds, vector<Id> targetIds, TimeStamp start, TimeStamp end, bool forcedId = false){return true;};
	bool setNodeAttributes(Id id, vector<Attribute> newAttributes, TimeStamp timeStamp = TimeStamp(0)){return true;};
	bool setTransform(Id id, IHomogeneousMatrix44::IHomogeneousMatrix44Ptr transform, TimeStamp timeStamp){return true;};
    bool setUncertainTransform(Id id, IHomogeneousMatrix44::IHomogeneousMatrix44Ptr transform, ITransformUncertainty::ITransformUncertaintyPtr uncertainty, TimeStamp timeStamp){return true;};
	bool deleteNode(Id id){return true;};
	bool addParent(Id id, Id parentId){return true;};
    bool removeParent(Id id, Id parentId){return true;};

private:

    // For potentaion queries to the graph
    SceneGraphFacade* observedScene;

    // Block that gets triggerd on an addRemoteRootNode event;
    ubx_block_t *b;
};

/* define a structure for holding the block local state. By assigning an
 * instance of this struct to the block private_data pointer (see init), this
 * information becomes accessible within the hook functions.
 */
struct rsg_json_sender_info
{
        /* add custom block local data here */
		brics_3d::WorldModel* wm;
		brics_3d::rsg::DotVisualizer* wm_printer;
		brics_3d::rsg::SceneGraphToUpdatesTraverser* wm_resender;
		brics_3d::rsg::FrequencyAwareUpdateFilter* frequency_filter;
		RemoteRootNodeAdditionTrigger* remote_root_trigger;

        /* this is to have fast access to ports for reading and writing, without
         * needing a hash table lookup */
        struct rsg_json_sender_port_cache ports;
};

/* init */
int rsg_json_sender_init(ubx_block_t *b)
{
        int ret = -1;
        struct rsg_json_sender_info *inf;

    	/* Configure the logger - default level won't tell us much */
    	brics_3d::Logger::setMinLoglevel(brics_3d::Logger::LOGDEBUG);

        /* allocate memory for the block local state */
        if ((inf = (struct rsg_json_sender_info*)calloc(1, sizeof(struct rsg_json_sender_info)))==NULL) {
                ERR("rsg_json_sender: failed to alloc memory");
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

    	/* Attach debug graph printer */
    	brics_3d::rsg::VisualizationConfiguration dotConfig;
    	dotConfig.abbreviateIds = false;
    	inf->wm_printer = new brics_3d::rsg::DotVisualizer(&inf->wm->scene);
    	inf->wm_printer->setConfig(dotConfig);
    	inf->wm_printer->setGenerateSvgFiles(false);
    	inf->wm->scene.attachUpdateObserver(inf->wm_printer);

    	int* store_dot_files =  ((int*) ubx_config_get_data_ptr(b, "store_dot_files", &clen));
    	if(clen == 0) {
    		LOG(INFO) << "rsg_json_sender: No store_dot_files configuation given. Turned off by default.";
    	} else {
    		if (*store_dot_files == 1) {
    			LOG(INFO) << "rsg_json_sender: store_dot_files turned on.";
    	    	inf->wm_printer->setIsActive(true); // toggle visualizer ON
    		} else {
    			LOG(INFO) << "rsg_json_sender: store_dot_files turned off.";
    	    	inf->wm_printer->setIsActive(false); // toggle visualizer OFF
    		}
    	}

    	int* store_history_as_dot_files =  ((int*) ubx_config_get_data_ptr(b, "store_history_as_dot_files", &clen));
    	if(clen == 0) {
    		LOG(INFO) << "rsg_json_sender: No store_history_as_dot_files configuation given. Turned off by default.";
    	} else {
    		if (*store_history_as_dot_files == 1) {
    			LOG(INFO) << "rsg_json_sender: store_history_as_dot_files turned on.";
    			inf->wm_printer->setKeepHistory(true);
    		} else {
    			LOG(INFO) << "rsg_json_sender: store_history_as_dot_files turned off.";
    			inf->wm_printer->setKeepHistory(false);
    		}


    		/* retrive optinal dot file prefix from config */
    		char* chrptr = (char*) ubx_config_get_data_ptr(b, "dot_name_prefix", &clen);
    		if(clen == 0) {
    			LOG(INFO) << "rsg_json_sender: No dot_name_prefix configuation given. Selecting a default name.";
    		} else {
				if(strcmp(chrptr, "")==0) {
					LOG(INFO) << "rsg_json_sender: dot_name_prefix is empty. Selecting a default name.";
				} else {
					std::string prefix(chrptr);
					LOG(INFO) << "rsg_json_sender: Using dot_name_prefix = " << prefix;
					inf->wm_printer->setFileName(prefix);
				}
    		}


    	}


    	float* max_freq =  ((float*) ubx_config_get_data_ptr(b, "max_freq", &clen));
    	if(clen == 0) {
    		*max_freq = 1.0;
    		LOG(WARNING) << "rsg_json_sender: No max_freq configuration given. Setting it to " << *max_freq;
    	} else {
    		if (*max_freq <= 0) {
    			*max_freq = 1.0;
    			LOG(WARNING) << "rsg_json_sender: max_freq <= 0. Resetting it to " << *max_freq;
    	    }
    	}
		LOG(INFO) << "rsg_json_sender: max_freq = " << *max_freq;

    	/* Attach filter */
    	inf->frequency_filter = new brics_3d::rsg::FrequencyAwareUpdateFilter();
    	inf->frequency_filter->setMaxGeometricNodeUpdateFrequency(0); // everything;
    	inf->frequency_filter->setMaxTransformUpdateFrequency(*max_freq); // not more then x Hz;

    	/* Attach the UBX port to the world model */
    	ubx_type_t* type =  ubx_type_get(b->ni, "unsigned char");
    	RsgToUbxPort* wmUpdatesUbxPort = new RsgToUbxPort(inf->ports.rsg_out, type);
    	brics_3d::rsg::JSONSerializer* wmUpdatesToJSONSerializer = new brics_3d::rsg::JSONSerializer(wmUpdatesUbxPort);
    	inf->wm->scene.attachUpdateObserver(inf->frequency_filter);
    	inf->frequency_filter->attachUpdateObserver(wmUpdatesToJSONSerializer);


    	/* Set error policy of RSG */
    	inf->wm->scene.setCallObserversEvenIfErrorsOccurred(false);

    	/* Initialize resender that resends the complete graph, if necessary */
    	inf->wm_resender = new brics_3d::rsg::SceneGraphToUpdatesTraverser(wmUpdatesToJSONSerializer);

    	/* Setup auto mount reply policy for incoming addRemoteNodes  */
    	inf->remote_root_trigger = new RemoteRootNodeAdditionTrigger(&inf->wm->scene, b);
    	inf->wm->scene.attachUpdateObserver(inf->remote_root_trigger);

        return 0;
}

/* start */
int rsg_json_sender_start(ubx_block_t *b)
{
        /* struct rsg_json_sender_info *inf = (struct rsg_json_sender_info*) b->private_data; */
        int ret = 0;

    	/* Set logger level */
    	unsigned int clen;
    	int* log_level =  ((int*) ubx_config_get_data_ptr(b, "log_level", &clen));
    	if(clen == 0) {
    		LOG(INFO) << "rsg_json_sender: No log_level configuation given.";
    	} else {
    		if (*log_level == 0) {
    			LOG(INFO) << "rsg_json_sender: log_level set to DEBUG level.";
    			brics_3d::Logger::setMinLoglevel(brics_3d::Logger::LOGDEBUG);
    		} else if (*log_level == 1) {
    			LOG(INFO) << "rsg_json_sender: log_level set to INFO level.";
    			brics_3d::Logger::setMinLoglevel(brics_3d::Logger::INFO);
    		} else if (*log_level == 2) {
    			LOG(INFO) << "rsg_json_sender: log_level set to WARNING level.";
    			brics_3d::Logger::setMinLoglevel(brics_3d::Logger::WARNING);
    		} else if (*log_level == 3) {
    			LOG(INFO) << "rsg_json_sender: log_level set to LOGERROR level.";
    			brics_3d::Logger::setMinLoglevel(brics_3d::Logger::LOGERROR);
    		} else if (*log_level == 4) {
    			LOG(INFO) << "rsg_json_sender: log_level set to FATAL level.";
    			brics_3d::Logger::setMinLoglevel(brics_3d::Logger::FATAL);
    		} else {
    			LOG(INFO) << "rsg_json_sender: unknown log_level = " << *log_level;		}
    	}

        return ret;
}

/* stop */
void rsg_json_sender_stop(ubx_block_t *b)
{
        /* struct rsg_json_sender_info *inf = (struct rsg_json_sender_info*) b->private_data; */
}

/* cleanup */
void rsg_json_sender_cleanup(ubx_block_t *b)
{
        struct rsg_json_sender_info *inf = (struct rsg_json_sender_info*) b->private_data;
        if(inf->wm_printer) {
        	delete inf->wm_printer;
        	inf->wm_printer = 0;
        }
        if(inf->wm_resender) {
        	delete inf->wm_resender;
        	inf->wm_resender = 0;
        }
        if(inf->frequency_filter){
        	delete inf->frequency_filter;
        	inf->frequency_filter = 0;
        }
        if(inf->remote_root_trigger){
        	delete inf->remote_root_trigger;
        	inf->remote_root_trigger = 0;
        }
        free(b->private_data);
}

/* step */
void rsg_json_sender_step(ubx_block_t *b)
{

        struct rsg_json_sender_info *inf = (struct rsg_json_sender_info*) b->private_data;
        brics_3d::WorldModel* wm = inf->wm;

        /* Resend the complete scene graph */
        LOG(INFO) << "rsg_json_sender: Resending the complete RSG now.";
        inf->wm->scene.advertiseRootNode(); // Make sure root node is always send; The graph traverser cannot handle this.
        inf->wm_resender->reset();
        Id localRootId = wm->scene.getRootId();
        /*
         * Warning a traversal that starts "above" the local root node is not guaranteed to
         * pass over the the complete structure. This has to be established beforehand.
         * It is actually beyond the context of a single agent. The application/system composition
         * has to be performed top-down and not vice versa.
         *
         * This is more an added robustness to send primitives further down the tree.
         * E.g. an agent updates mostly another containment like a commonly shared one.
         *
         *             global_root
         *                 |
         *    +------------+--------- +
         *    |            |          |
         *    r1           r2      local_root
         *
         */
        Id rootId = localRootId;
        bool useGlobalRootId = true;
        if(useGlobalRootId) { //use (potental) global id
        	Id globalRootId = 0;
    		brics_3d::rsg::RootFinder rootFinder;
    		wm->scene.executeGraphTraverser(&rootFinder, localRootId);
    		globalRootId = rootFinder.getRootNode();

    		if(!globalRootId.isNil()) {
    			rootId = globalRootId;
    		} else {
    			LOG(ERROR) << "rsg_json_sender: Cannot obtain a global root Id. Using the local one instead.";
    		}

    		LOG(DEBUG) << "rsg_json_sender: using rootId = " << rootId << ", while localRootId = " << localRootId;
        }

        wm->scene.executeGraphTraverser(inf->wm_resender, rootId); // Note: addRemoteRoot node is only forwarded once

}

