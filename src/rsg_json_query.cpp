#include "rsg_json_query.hpp"

/* microblx type for the robot scene graph */
#include "types/rsg/types/rsg_types.h"

/* BRICS_3D includes */
#include <brics_3d/core/Logger.h>
#include <brics_3d/worldModel/WorldModel.h>
#include <brics_3d/worldModel/sceneGraph/DotVisualizer.h>
#include <brics_3d/worldModel/sceneGraph/JSONQueryRunner.h>
#include <brics_3d/worldModel/sceneGraph/UpdatesToSceneGraphListener.h>
#include <brics_3d/worldModel/sceneGraph/GraphConstraintUpdateFilter.h>


using namespace brics_3d;
using brics_3d::Logger;


UBX_MODULE_LICENSE_SPDX(BSD-3-Clause)

#define DEFAULT_BUFFER_SIZE 20000

/* define a structure for holding the block local state. By assigning ano
 * instance of this struct to the block private_data pointer (see init), this
 * information becomes accessible within the hook functions.
 */
struct rsg_json_query_info
{
        /* add custom block local data here */
		brics_3d::WorldModel* wm;
		brics_3d::rsg::DotVisualizer* wm_printer;
		brics_3d::rsg::JSONQueryRunner* wm_query_runner;
		brics_3d::rsg::GraphConstraintUpdateFilter* constraint_filter; // optional
		brics_3d::rsg::UpdatesToSceneGraphListener* wm_updates_to_wm;  // for constraint_filter

        /* this is to have fast access to ports for reading and writing, without
         * needing a hash table lookup */
        struct rsg_json_query_port_cache ports;

        unsigned char* input_buffer;		/* A buffer to buffer somplete HDF5 messgage. */
        unsigned long input_buffer_size;	/* Buffer size in bytes.
        										 * This depends on the data used and transmitted
        										 * 5-20.000 bytes are sufficinet for transform updates, etc,
        										 * More should be reserved for point clouds or mesh data.
        										 * E.g a Kinect consumes around 10,000,000 bytes per
        										 * message.
         	 	 	 	 	 	 	 	 	 	 */

};

/* init */
int rsg_json_query_init(ubx_block_t *b)
{
        int ret = -1;
        struct rsg_json_query_info *inf;

    	/* Configure the logger - default level won't tell us much */
    	brics_3d::Logger::setMinLoglevel(brics_3d::Logger::LOGDEBUG);

        /* allocate memory for the block local state */
        if ((inf = (struct rsg_json_query_info*)calloc(1, sizeof(struct rsg_json_query_info)))==NULL) {
                ERR("rsg_json_query: failed to alloc memory");
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
    		LOG(ERROR) << "World model handle could not be optained via configuration parameter."
    					  "Creating a new world model instance instead (mostly for debugging purposes)."
    					  "Please check your system design if this is intended!";
    		inf->wm = new brics_3d::WorldModel();
        }


        /*
         * Work flow:
         *  Queries:
         *    IN port -> QueryRunner -> OUT port
         *  Update "Queries":
         *    IN port -> QueryRunner -> Deserializer -> constraint filter -> wm_updates_to_wm ->  wm -> OUT port
         */

        /* Setup graph constraint filter */
        inf->constraint_filter = new brics_3d::rsg::GraphConstraintUpdateFilter(inf->wm, brics_3d::rsg::GraphConstraintUpdateFilter::RECEIVER);
    	inf->wm_updates_to_wm = new brics_3d::rsg::UpdatesToSceneGraphListener();
    	inf->wm_updates_to_wm->attachSceneGraph(&inf->wm->scene);
    	inf->wm_updates_to_wm->setForcedIdPolicy(false);
    	inf->constraint_filter->attachUpdateObserver(inf->wm_updates_to_wm); // handle used for updates

        /* Setup query runner module  */
//        inf->wm_query_runner = new brics_3d::rsg::JSONQueryRunner(inf->wm); // without filter for updates
        inf->wm_query_runner = new brics_3d::rsg::JSONQueryRunner(inf->wm, inf->constraint_filter); // with filter for updates



        /* Setup input buffer for JSON messages */
        inf->input_buffer_size = *((uint32_t*) ubx_config_get_data_ptr(b, "buffer_len", &clen));
    	if((clen == 0) || (inf->input_buffer_size == 0)) {
            inf->input_buffer_size = DEFAULT_BUFFER_SIZE;
    		LOG(WARNING) << "Invalid or missing configuration for buffer_len. "
    	                    "Falling back to default value buffer_len = " << DEFAULT_BUFFER_SIZE;
    	}
    	LOG(DEBUG) << "Input buffer len set to " << inf->input_buffer_size;

        if((inf->input_buffer = (unsigned char *)malloc(inf->input_buffer_size)) == NULL) {
          ERR("failed to allocate input buffer");
          free(inf->input_buffer);
          return 0;
        }

        return 0;
}

/* start */
int rsg_json_query_start(ubx_block_t *b)
{
        /* struct rsg_json_query_info *inf = (struct rsg_json_query_info*) b->private_data; */
        int ret = 0;

    	/* Set logger level */
    	unsigned int clen;
    	int* log_level =  ((int*) ubx_config_get_data_ptr(b, "log_level", &clen));
    	if(clen == 0) {
    		LOG(INFO) << "rsg_json_query: No log_level configuation given.";
    	} else {
    		if (*log_level == 0) {
    			LOG(INFO) << "rsg_json_query: log_level set to DEBUG level.";
    			brics_3d::Logger::setMinLoglevel(brics_3d::Logger::LOGDEBUG);

    			std::stringstream tmpFileName;
    			time_t rawtime;
    			struct tm* timeinfo;
    			time(&rawtime);
    			timeinfo = localtime(&rawtime);
    			tmpFileName << "20" // This will have to be adjusted in year 2100 ;-)
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

    			std::string fileName = "rsg_json_query-" + tmpFileName.str() + ".log";
    			bool doAppend = false;
    			brics_3d::Logger::setLogfile(fileName, doAppend);

    		} else if (*log_level == 1) {
    			LOG(INFO) << "rsg_json_query: log_level set to INFO level.";
    			brics_3d::Logger::setMinLoglevel(brics_3d::Logger::INFO);
    		} else if (*log_level == 2) {
    			LOG(INFO) << "rsg_json_query: log_level set to WARNING level.";
    			brics_3d::Logger::setMinLoglevel(brics_3d::Logger::WARNING);
    		} else if (*log_level == 3) {
    			LOG(INFO) << "rsg_json_query: log_level set to LOGERROR level.";
    			brics_3d::Logger::setMinLoglevel(brics_3d::Logger::LOGERROR);
    		} else if (*log_level == 4) {
    			LOG(INFO) << "rsg_json_query: log_level set to FATAL level.";
    			brics_3d::Logger::setMinLoglevel(brics_3d::Logger::FATAL);
    		} else {
    			LOG(INFO) << "rsg_json_query: unknown log_level = " << *log_level;		}
    	}

        return ret;
}

/* stop */
void rsg_json_query_stop(ubx_block_t *b)
{
        /* struct rsg_json_query_info *inf = (struct rsg_json_query_info*) b->private_data; */
}

/* cleanup */
void rsg_json_query_cleanup(ubx_block_t *b)
{
        struct rsg_json_query_info *inf = (struct rsg_json_query_info*) b->private_data;
		if(inf->constraint_filter != 0) {
			delete inf->constraint_filter;
			inf->constraint_filter = 0;
		}
		if(inf->wm_updates_to_wm != 0){
			delete inf->wm_updates_to_wm;
			inf->wm_updates_to_wm = 0;
		}
        free(inf->input_buffer);
        free(b->private_data);
}

/* step */
void rsg_json_query_step(ubx_block_t *b)
{

        struct rsg_json_query_info *inf = (struct rsg_json_query_info*) b->private_data;
        //LOG(DEBUG) << "rsg_json_query: Processing an incoming update";

		/*
		 * read data
		 */
		ubx_port_t* port = inf->ports.rsq_query;
		assert(port != 0);

		ubx_data_t msg;
		checktype(port->block->ni, port->in_type, "unsigned char", port->name, 1);
		msg.type = port->in_type;
		msg.len = inf->input_buffer_size;
		msg.data = (void *)inf->input_buffer;
		int readBytes = __port_read(port, &msg);
//		LOG(DEBUG) << "rsg_json_query: Port returned " << readBytes <<
//                      " bytes, while data message length is " << msg.len <<
//                      " bytes. Resulting size = " << data_size(&msg);

		const char *dataBuffer = (char *)msg.data;
		if ((dataBuffer!=0) && (msg.len > 1) && (readBytes > 1)) {
			LOG(INFO) << "rsg_json_query: Port returned " << readBytes <<
	                      " bytes, while data message length is " << msg.len <<
	                      " bytes. Resulting size = " << data_size(&msg);

			std::string query(dataBuffer, readBytes);
			std::string result;
			LOG(INFO) << "rsg_json_query: Processing query = " << std::endl << query;

			/*
			 * process query
			 */
			inf->wm_query_runner->query(query, result);

			/*
			 * write data
			 */
			LOG(DEBUG) << "rsg_json_query: Reply is = " << std::endl << result;
			ubx_port_t* result_port = inf->ports.rsg_result;
			assert(result_port != 0);

			if(result.size() > inf->input_buffer_size) {
				LOG(ERROR) << "Result with = " << result.size() << " bytes is larger than max output buffer lenght = "
						<< inf->input_buffer_size;
				/*
				 * Warning: we actually don't have an output buffer size. Though is mostly has the same as the input one...
				 */
				result = "{}";
			}

			ubx_data_t msg_result;
			msg_result.data = (void *)result.c_str();
			msg_result.len = result.size();
			msg_result.type = result_port->out_type;

			LOG(DEBUG) << "Sending " << msg_result.len << " bytes: ";
			__port_write(result_port, &msg_result);

		} else if (dataBuffer == 0) {
			LOG(DEBUG) << "Pointer to data buffer is zero. Aborting this update.";
		} else {
			//LOG(DEBUG) << "Incoming update has not enough data to be processed. Aborting this update.";
		}



}

