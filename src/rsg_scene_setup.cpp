#include "rsg_scene_setup.hpp"

/* microblx type for the robot scene graph */
#include "types/rsg/types/rsg_types.h"

/* BRICS_3D includes */
#include <brics_3d/core/Logger.h>
#include <brics_3d/core/HomogeneousMatrix44.h>
#include <brics_3d/worldModel/WorldModel.h>
#include <brics_3d/worldModel/sceneGraph/DotVisualizer.h>
#include <brics_3d/worldModel/sceneGraph/JSONDeserializer.h>

//#define GENERATED_SCENE_SETUP

#ifdef GENERATED_SCENE_SETUP
#include "/opt/src/eclipse_workspaces/eclipse_kepler_xtext_runtime/wm_example/src-gen/SceneSetup.h"
#endif /* GENERATED_SCENE_SETUP */

using namespace brics_3d;
using brics_3d::Logger;


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
struct rsg_scene_setup_info
{
        /* add custom block local data here */
		brics_3d::WorldModel* wm;
		brics_3d::rsg::DotVisualizer* wm_printer;

        /* this is to have fast access to ports for reading and writing, without
         * needing a hash table lookup */
        struct rsg_scene_setup_port_cache ports;
};

/* init */
int rsg_scene_setup_init(ubx_block_t *b)
{
        int ret = -1;
        struct rsg_scene_setup_info *inf;

    	/* Configure the logger - default level won't tell us much */
    	brics_3d::Logger::setMinLoglevel(brics_3d::Logger::LOGDEBUG);

        /* allocate memory for the block local state */
        if ((inf = (struct rsg_scene_setup_info*)calloc(1, sizeof(struct rsg_scene_setup_info)))==NULL) {
                ERR("rsg_scene_setup: failed to alloc memory");
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
    		LOG(FATAL) << " World model handle could not be initialized.";
    		return -1;
    	}

    	/* Attach debug graph printe */
    	inf->wm_printer = new brics_3d::rsg::DotVisualizer(&inf->wm->scene);
    	inf->wm_printer->setFileName("scene_setup_graph");
    	inf->wm_printer->setKeepHistory(false);
//    	inf->wm->scene.attachUpdateObserver(inf->wm_printer);

    	inf->wm->scene.setCallObserversEvenIfErrorsOccurred(false);
        return 0;
}

/* start */
int rsg_scene_setup_start(ubx_block_t *b)
{
        /* struct rsg_scene_setup_info *inf = (struct rsg_scene_setup_info*) b->private_data; */
        int ret = 0;

    	/* Set logger level */
    	unsigned int clen;
    	int* log_level =  ((int*) ubx_config_get_data_ptr(b, "log_level", &clen));
    	if(clen == 0) {
    		LOG(INFO) << "rsg_scene_setup: No log_level configuation given.";
    	} else {
    		if (*log_level == 0) {
    			LOG(INFO) << "rsg_scene_setup: log_level set to DEBUG level.";
    			brics_3d::Logger::setMinLoglevel(brics_3d::Logger::LOGDEBUG);
    		} else if (*log_level == 1) {
    			LOG(INFO) << "rsg_scene_setup: log_level set to INFO level.";
    			brics_3d::Logger::setMinLoglevel(brics_3d::Logger::INFO);
    		} else if (*log_level == 2) {
    			LOG(INFO) << "rsg_scene_setup: log_level set to WARNING level.";
    			brics_3d::Logger::setMinLoglevel(brics_3d::Logger::WARNING);
    		} else if (*log_level == 3) {
    			LOG(INFO) << "rsg_scene_setup: log_level set to LOGERROR level.";
    			brics_3d::Logger::setMinLoglevel(brics_3d::Logger::LOGERROR);
    		} else if (*log_level == 4) {
    			LOG(INFO) << "rsg_scene_setup: log_level set to FATAL level.";
    			brics_3d::Logger::setMinLoglevel(brics_3d::Logger::FATAL);
    		} else {
    			LOG(INFO) << "rsg_scene_setup: unknown log_level = " << *log_level;		}
    	}

        return ret;
}

/* stop */
void rsg_scene_setup_stop(ubx_block_t *b)
{
        /* struct rsg_scene_setup_info *inf = (struct rsg_scene_setup_info*) b->private_data; */
}

/* cleanup */
void rsg_scene_setup_cleanup(ubx_block_t *b)
{
        free(b->private_data);
}

/* step */
void rsg_scene_setup_step(ubx_block_t *b)
{

        struct rsg_scene_setup_info *inf = (struct rsg_scene_setup_info*) b->private_data;
        brics_3d::WorldModel* wm = inf->wm;

        /*
         * Load scene based on JSON file.
         */
    	/* retrive optional dot file prefix from config */
    	string* fileName = new std::string("");
    	unsigned int clen;
    	char* chrptr = (char*) ubx_config_get_data_ptr(b, "rsg_file", &clen);
    	if(clen == 0) {
    		LOG(INFO) << "rsg_scene_setup: No rsg_file configuation given. Selecting a default.";
    	} else {
    		if(strcmp(chrptr, "")==0) {
    			LOG(INFO) << "rsg_scene_setup: rsg_file is empty. Selecting a default.";
    		} else {
    			std::string map_file(chrptr);
    			*fileName = map_file;
    		}
    	}
    	LOG(INFO) << "rsg_scene_setup: file name = " << *fileName;

    	if(fileName->compare("") != 0) {
    		LOG(INFO) << "rsg_scene_setup: Loading JSON model.";

    		/* Read file */
    		std::ifstream inputFile;
    		inputFile.open (fileName->c_str(), std::ifstream::in);
    		std::stringstream serializedModel;
    		serializedModel << inputFile.rdbuf();

    		/* Do the actual JSON parsing. */
    		LOG(INFO) << serializedModel.str();
    		brics_3d::rsg::JSONDeserializer deserializer(wm);
    		deserializer.setMapUnknownParentIdsToRootId(true);
    		deserializer.write(serializedModel.str());

    		return;

    	}

        /*
         * Hard coded CPP version below as fall back:
         */

    	/* Add group nodes */
//    	std::vector<brics_3d::rsg::Attribute> attributes;
//    	attributes.clear();
//    	attributes.push_back(rsg::Attribute("taskType", "scene_objecs_scene_setup"));
//    	rsg::Id sceneObjectsId;
//    	wm->scene.addGroup(wm->getRootNodeId(), sceneObjectsId, attributes);
////    	wm->scene.addGroup(wm->getRootNodeId(), sceneObjectsId, attributes);

    	/* Add group nodes */
    	std::vector<brics_3d::rsg::Attribute> attributes;
    	attributes.clear();
    	attributes.push_back(rsg::Attribute("taskType", "scene_objecs"));
    	pid_t pid = getpid();
    	std::stringstream pidAsSteam("");
    	pidAsSteam << pid;
    	attributes.push_back(rsg::Attribute("PID", pidAsSteam.str()));
    	rsg::Id sceneObjectsId;
    	wm->scene.addGroup(wm->getRootNodeId(), sceneObjectsId, attributes);

    	/* Box for "virtual fence" */
    	attributes.clear();
    	attributes.push_back(rsg::Attribute("name", "virtual_fence")); // this name serves as a conventions here
    	attributes.push_back(rsg::Attribute("taskType", "sceneObject"));
    	rsg::Id boxTfId;
    	brics_3d::IHomogeneousMatrix44::IHomogeneousMatrix44Ptr transform120(new brics_3d::HomogeneousMatrix44(1,0,0,  	// Rotation coefficients
    			0,1,0,
    			0,0,1,
    			1,2,0)); 						// Translation coefficients
    	wm->scene.addTransformNode(sceneObjectsId, boxTfId, attributes, transform120, wm->now());

    	attributes.clear();
    	attributes.push_back(rsg::Attribute("shape", "Box"));
    	rsg::Box::BoxPtr box( new rsg::Box(1,2,0));
    	rsg::Id boxId;
    	wm->scene.addGeometricNode(boxTfId, boxId, attributes, box, wm->now());


#ifdef GENERATED_SCENE_SETUP
    	brics_3d::rsg::IFunctionBlock* scene_setup = new SceneSetup(wm);
    	scene_setup->execute();
    	delete scene_setup;
#endif

}

