#include "rsg_dump.hpp"

/* microblx type for the robot scene graph */
#include "types/rsg/types/rsg_types.h"

/* BRICS_3D includes */
#include <brics_3d/core/Logger.h>
#include <brics_3d/core/HomogeneousMatrix44.h>
#include <brics_3d/worldModel/WorldModel.h>
#include <brics_3d/worldModel/sceneGraph/DotGraphGenerator.h>
#include <brics_3d/worldModel/sceneGraph/HDF5UpdateSerializer.h>

#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip> 	//for setw and setfill
#include <ctime>

using namespace brics_3d;
using brics_3d::Logger;


UBX_MODULE_LICENSE_SPDX(BSD-3-Clause)



/* define a structure for holding the block local state. By assigning an
 * instance of this struct to the block private_data pointer (see init), this
 * information becomes accessible within the hook functions.
 */
struct rsg_dump_info
{
        /* add custom block local data here */
		brics_3d::WorldModel* wm;
		brics_3d::rsg::DotGraphGenerator* wm_printer;

		std::ofstream* output;
		std::string* fileNamePrefix;
		std::string* directoryName;
		int counter;

        /* this is to have fast access to ports for reading and writing, without
         * needing a hash table lookup */
        struct rsg_dump_port_cache ports;
};

/* init */
int rsg_dump_init(ubx_block_t *b)
{
        int ret = -1;
        struct rsg_dump_info *inf;

    	/* Configure the logger - default level won't tell us much */
    	brics_3d::Logger::setMinLoglevel(brics_3d::Logger::LOGDEBUG);

        /* allocate memory for the block local state */
        if ((inf = (struct rsg_dump_info*)calloc(1, sizeof(struct rsg_dump_info)))==NULL) {
                ERR("rsg_dump: failed to alloc memory");
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

    	inf->output = new std::ofstream();
    	inf->fileNamePrefix = new std::string("rsg_dump");

		/* retrive optional dot file prefix from config */
		char* chrptr = (char*) ubx_config_get_data_ptr(b, "dot_name_prefix", &clen);
		if(clen == 0) {
			LOG(INFO) << "rsg_dump: No dot_name_prefix configuation given. Selecting a default name.";
			*inf->fileNamePrefix = "rsg_dump";
		} else {
			if(strcmp(chrptr, "")==0) {
				LOG(INFO) << "rsg_dump: dot_name_prefix is empty. Selecting a default name.";
				*inf->fileNamePrefix = "rsg_dump";
			} else {
				std::string prefix(chrptr);
				LOG(INFO) << "rsg_dump: Using dot_name_prefix = " << prefix;
				*inf->fileNamePrefix = prefix;

			}
		}
		LOG(DEBUG) << "rsg_dump: dot_name_prefix = " << *inf->fileNamePrefix;

    	inf->directoryName = new std::string("./");

    	/* Create graph printer */
    	inf->wm_printer = new brics_3d::rsg::DotGraphGenerator();
    	brics_3d::rsg::VisualizationConfiguration config;
    	config.abbreviateIds = false;
    	inf->wm_printer->setConfig(config);

    	inf->wm->scene.setCallObserversEvenIfErrorsOccurred(false);

    	inf->counter = 0;

        return 0;
}

/* start */
int rsg_dump_start(ubx_block_t *b)
{
        /* struct rsg_dump_info *inf = (struct rsg_dump_info*) b->private_data; */
        int ret = 0;
        return ret;
}

/* stop */
void rsg_dump_stop(ubx_block_t *b)
{
        /* struct rsg_dump_info *inf = (struct rsg_dump_info*) b->private_data; */
}

/* cleanup */
void rsg_dump_cleanup(ubx_block_t *b)
{
		struct rsg_dump_info *inf = (struct rsg_dump_info*) b->private_data;
		if(inf->wm_printer) {
			delete inf->wm_printer;
			inf->wm_printer = 0;
		}
		if(inf->fileNamePrefix) {
			delete inf->fileNamePrefix;
			inf->fileNamePrefix = 0;
		}
		if(inf->directoryName) {
			delete inf->directoryName;
			inf->directoryName = 0;
		}
		if(inf->output) {
			delete inf->output;
			inf->output = 0;
		}
		free(b->private_data);
}

/* step */
void rsg_dump_step(ubx_block_t *b)
{

        struct rsg_dump_info *inf = (struct rsg_dump_info*) b->private_data;
        brics_3d::WorldModel* wm = inf->wm;
        std::string fileName;

		std::stringstream tmpFileName;
		time_t rawtime;
		struct tm* timeinfo;
		time(&rawtime);
		timeinfo = localtime(&rawtime);


		tmpFileName << "20" // This will have to be adjusted in year 2100
				<< (timeinfo->tm_year)-100 << "-"
				<< std::setw(2) << std::setfill('0')
				<<	(timeinfo->tm_mon)+1 << "-"
				<< std::setw(2) << std::setfill('0')
				<<	timeinfo->tm_mday << "_"
				<< std::setw(2) << std::setfill('0')
				<<	timeinfo->tm_hour << "-"
				<< std::setw(2) << std::setfill('0')
				<<	timeinfo->tm_min << "-"
				<< std::setw(2) << std::setfill('0')
				<<	timeinfo->tm_sec;


		fileName = *inf->directoryName + *inf->fileNamePrefix + "_" + tmpFileName.str();
		LOG(INFO) << "rsg_dump: Printing graph to file " << fileName;

		/* Save a complete snapshopt relative to the root node */
		wm->scene.executeGraphTraverser(inf->wm_printer, wm->scene.getRootId());
		bool printRemoteRootNodes = true;
		if(printRemoteRootNodes) {
			vector<brics_3d::rsg::Id> remoteRootNodeIds;
			wm->scene.getRemoteRootNodes(remoteRootNodeIds);
			for(vector<brics_3d::rsg::Id>::const_iterator it = remoteRootNodeIds.begin(); it != remoteRootNodeIds.end(); ++it) {
				wm->scene.executeGraphTraverser(inf->wm_printer, *it);
			}
		}

		inf->output->open((fileName + ".gv").c_str(), std::ios::trunc);
		if (!inf->output->fail()) {
			*inf->output << inf->wm_printer->getDotGraph();
		} else {
			LOG(ERROR) << "DotVisualizer: Cannot write to file " << fileName << ".gv";
		}

		inf->output->flush();
		inf->output->close();
		inf->wm_printer->reset();
		inf->counter++;

		LOG(INFO) << "rsg_dump: Done.";
}

