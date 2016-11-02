RSG integration into Microblox (UBX) [![Build Status](https://travis-ci.org/blumenthal/ubx_robotscenegraph.svg?branch=master)](https://travis-ci.org/blumenthal/ubx_robotscenegraph)
==========================================================

## Table of Contents

* [Overview](#overview)
* [Installation](#installation)
* [Updates](#updates)
* [Usage](#usage)
* [User Manual](doc/manual.md)
 * The [data model](doc/manual.md#data-model)
 * The [update](doc/manual.md#updates) capabilities
 * The [query](doc/manual.md#queries) capabilities 
 * The [distribution](doc/manual.md#distribution) capabilities  
 * World model [debugging](doc/manual.md#debugging) techniques
* [Changelog](#changelog)
* [Licensing](#licensing)





Overview
--------

RSG integration into Microblox (UBX). Provides system composition
models for the **SHERPA World Model**.

In a nutshell, a **SHERPA World Model** as typically deployed on a robot has one 
so called *World Model Agent* plus a set of communication components like ZMQ or ROS.
The components are realized with the Microblox (UBX) framework that allows to represent
the SHERPA World Model in a single system composition model (.utc file). 

The World Model Agent is written in **C++** and can be 
[queried](http://www.best-of-robotics.org/brics_3d/classbrics__3d_1_1rsg_1_1ISceneGraphQuery.html) and 
[updated](http://www.best-of-robotics.org/brics_3d/classbrics__3d_1_1rsg_1_1ISceneGraphUpdate.html) 
with its respective API. In order to use that C++ API in your own program a dedicated 
*World Model Agent* has to be spawned, the communication infrastructure (ZMQ or ROS) 
has to be added manually and it has to be connected to the other World Model Agent wihtin **SHERPA World Model**.

As an alternative, the SHERPA World Model provides a **JSON API** that allows to send graph operations via Zyre or ZMQ. 
Examples for using the JSON API can be found for a [Task Specification Tree](examples/tst/README.md) 
and [here](examples/json_api)



Installation
------------

### Installation as Docker container


This software can be installed as a [Docker](https://www.docker.com/) container.
Further details can be found in the respective [docker section](docker/README.md).

### Installation with an install script

In the below example we assume that all modules will be installed in the **parent** folder, where
this repository (ubx_robotscenegraph) has been *cloned* to. Call the script with default parameters:

```
	./install.sh -i 
```
 OR:

```
 ./install.sh --help
```
to retrive the folllowing usage information: 
 
``` 
 Usage: ./install.sh -i [--no-sudo] [--no-ros] [--workspace-path=PATH] [--install-path=PATH] [-h|--help] [-j=VALUE] 
 E.g. : ./install.sh -i --workspace-path=../ 

    -i                     Mandatory! Perform actual installation.
    -h|--help              Display this help and exit
    --no-sudo              In case the system has no sudo command available. 
    --no-ros               In case the system has no ROS (Hydro/Indigo) installation.
    --workspace-path=PATH  Path to where libraries and bulild. Default is ../
    --install-path=PATH    Path to where libraries and modules are installed (make install) into.
                           (except for brics_3d). Default is /usr/local 
    -j=VALUE               used for make -jVAULE 
```

The script will automatically summarize which environment variable have to permanently saved. Please follow the instructions. E.g.
```
############################ATTENTION###############################
 ATTENTION: Please add the following environment variables:

export "HDF5_ROOT=/usr/local/" >> ~/.bashrc
export "UBX_ROOT=/workspace/microblx" >> ~/.bashrc
export "UBX_MODULES=/usr/local/lib/ubx" >> ~/.bashrc
export "BRICS_3D_DIR=/workspace/brics_3d" >> ~/.bashrc
export "FBX_MODULES=/workspace/brics_3d_function_blocks" >> ~/.bashrc
source ~/.bashrc .

####################################################################
```


### Manual installation

All detailed steps on how to install the dependencies can be found in the [install.sh](install.sh) script. 
The relevant environment variables for the installation procedure are listed in the [section](#environment-variables) below. 

#### Dependencies

 - brics_3d_function_blocks Installation instructions can be found here: https://github.com/blumenthal/brics_3d_function_blocks
 - BRICS_3D library. Installation instructions can be found here: http://www.best-of-robotics.org/brics_3d/installation.html
 - microblx library. See: https://github.com/UbxTeam/microblx

#### Compilation


Compile this project manually as follows:

```
 $ mkdir build
 $ cd build 
 $ cmake -DCMAKE_CXX_COMPILER=/usr/bin/clang++ -DEIGEN_INCLUDE_DIR=/usr/include/eigen3 -DHDF5_1_8_12_OR_HIGHER=true -DUSE_JSON=true ..
 $ make
 $ make install 
```

#### Environment Variables

Please make sure the following environment variables are set. (They should be since the install script is putting them into your `.bashrc`. However, you need to source your `.bashrc` after the installation.)

Default variables for Microblox:


| Name          | Description |
| ------------- | ----------- |
| UBX_ROOT      | Points to the installation folder of UBX.  Used within the CMake scripts to discover the UBX library. |
| UBX_MODULES   | Points to the the place where the UBX types, blocks etc. are installed. Used to load the types and modules at run-time |




Dependencies to BRICS_3D and HDF5:


| Name          | Description |
| ------------- | ----------- |
| BRICS_3D_DIR  | Points to the installation folder of BRICS_3D. Used within the CMake scripts to discover the BRICS_3D library. |
| FBX_MODULES   | Points to the the root folder of the BRICS_3D function blocks. Used to discover the rsg lua scripts.  |
| HDF5_ROOT     | Points to the installation folder of HDF5. Use it in case it is not into installed to the default folders. |



Updates
-------

### Manual updates

In order to update the system go to all relevant module folders. For each perform a ``git pull origin master`` and recompile with ``make clean`` followed by  ``sudo make install``

### Update script

For convenience please use the following [update.sh](update.sh) script:
```
	./update.sh
```

Similar to install script the following options are available:

```
	./update.sh --no-sudo
```
In case the system has no sudo command available. Usefull for Docker based installations. 

 OR
```
	./update.sh --no-git
```
In case no git pull should be invoked. 


Usage
-----

### Launching of a SHERPA World Model

You can start the SHERPA World Model by invoking:
```
  roscore&
  ./run_sherpa_world_model.sh
```


In case the ROS communication modules are not available use instead:
```
  ./run_sherpa_world_model.sh --no-ros
```


When the system is launched correcly the following promt appears:
```
JIT: ON CMOV SSE2 SSE3 SSE4.1 fold cse dce fwd dse narrow loop abc sink fuse
> 
```

Then enter the following command and hit enter:
```
 start_all()
```

The system state can be observed in a browser by entering http://localhost:8888/ as URL.
 

Other examples can be obtained from the [examples section](examples).

### Interaction with a SHERPA World Model

In general there are __three__ possibilities to interact with the *SHERPA World Model*:

1. By embedding a new *World Model Agent* in a process. It allows to [querie]
   (http://www.best-of-robotics.org/brics_3d/classbrics__3d_1_1rsg_1_1ISceneGraphQuery.html) and 
   [update](http://www.best-of-robotics.org/brics_3d/classbrics__3d_1_1rsg_1_1ISceneGraphUpdate.html) 
   all data via the C++ API. In order to use it, a dedicated *World Model Agent* has
   to be spawned, the communication infrastructure (ZMQ) has to be added manually 
   and it has to be connected to the other *World Model Agent* of the *SHERPA World Model*.

2. By using the [JSON API](examples/json_api/README.md). It allows to send graph operations 
   encoded as JSON messages via Zyre or ZMQ. The distribution of the [Task Specification Tree](examples/tst/README.md)
   makes use of this API.	 

3. By creation of a dedicated *bridge* as done for ROS [TF](https://github.com/blumenthal/sherpa_world_model_tf_bridge) 
   trees. Please note, that the implementation of the TF bridge embeds a *World 
   Model Agent* as stated my method 1 and serves as a good reference example.

Changelog
---------

### 0.3.0 (02.11.2016)

* Added [Zyre client example](./examples/zyre/README.md) in C and Python. 
  The existing Python examples can seamlessly used with the new Zyre back-end
* Added Zyre [with](./doc/manual.md#with-mediator-using-zyre-recommended) and optinally [without](./doc/manual.md#without-mediator-using-zyre) SHERPA Mediator as communication back-end between SWMs and removed ZMQ PUB-SUB sockets
* Added function block query API, mechanism and examples for [advanced queries](./doc/manual.md#queries) 
* Replaced semantic context and frequency filter by graph [constraint](./examples/sherpa/constraints.lua) mechanism
* Added simulated mission to be used to test the Knowrob-RSG bridge, including battery and ARTVA signals 
* Added optional OSG visualizer (as function block)
* Added Docker generation to Travis-CI
* Added handling of areas


### 0.2.0 (13.04.2016)

* Added support for automatic self synchronization
* Added reply status messages for updates and queries.
* Added one shot launch script
* Added semantic context and frequency filter 
* Added Champoluc semantic map and scripts
* Added further Python examples
* Revised install script


### 0.1.0 (01.03.2016) 

* Initial version

Licensing
---------

This software is published under a dual-license: GNU Lesser General Public
License LGPL 2.1 and Modified BSD license. The dual-license implies that
users of this code may choose which terms they prefer. Please see the files
called LGPL-2.1 and BSDlicense.

Acknowledgements
----------------

This work was supported by the European FP7 project SHERPA (FP7-600958).

Impressum
---------

Written by Sebastian Blumenthal (blumenthal@locomotec.com)
Last update: 02.11.2016
 


