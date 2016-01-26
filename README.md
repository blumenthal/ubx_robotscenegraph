RSG integration into Microblox (UBX)
==========================================================

## Table of Contents

* [Overview](#overview)
* [Installation](#installation)
* [Updates](#updates)
* [Usage](#usage)
* [User Manual](doc/manual.md)
* [Licensing](#licensing)





Overview
--------

RSG integration into Microblox (UBX). Provides system composition
models for the **SHERPA World Model**.

In a nutshell, a **SHERPA World Model** as typically deployed on a robot has one 
so called *World Model Agent* plus a set of communication components like ZMQ or ROS.
The components are realized with the Microblox (UBX) framework that allows to represent
the SHERPA World Model in a single system coposition model (.utc file). 

The World Model Agent is written in **C++** and can be 
[queried](http://www.best-of-robotics.org/brics_3d/classbrics__3d_1_1rsg_1_1ISceneGraphQuery.html) and 
[updated](http://www.best-of-robotics.org/brics_3d/classbrics__3d_1_1rsg_1_1ISceneGraphUpdate.html) 
with its respective API. In order to use that C++ API in your own program a dedicated 
*World Model Agent* has to be spawned, the communication infrastructure (ZMQ or ROS) 
has to be added manually and it has to be connected to the other World Model Agent wihtin **SHERPA World Model**.   

As an alternative, the SHERPA World Model provides a **JSON API** that allows to send graph operatins via ZMQ. 
Examples for using the JSON API can be found for a [Task Specification Tree](examples/tst/README.md) 
and [here](examples/json_api)



Installation
------------

### Installation as Docker container


This software can be installed as a [Docker](https://www.docker.com/) container.
Further details can be found in the respective [docker section](docker/README.md).

### Installation with an install script

__NOTE:__ The software modules are installed __relative__ to the invocation of [install.sh](install.sh) script.
In the below example we assume that all modules shall be installed in the parent folder, where
this repository (ubx_robotscenegraph) has been *cloned* to.
 
Call this script wihtout any parameters:

```
	source ./ubx_robotscenegraph/install.sh
```
 OR

```
	source ./ubx_robotscenegraph/install.sh --no-sudo
```
In case the system has no sudo command available. 
 
 OR
```
 source ./ubx_robotscenegraph/install.sh --no-ros
``` 
In case the system has no ROS (Hydro) installtion. 

The ``source`` command is important because it adds environment variables (cf. [section](#environment-variables) below)
that are required for the subsequent installation steps.


### Manual installation

All detailed steps on how to install the dependencies can be cound in the [install.sh](install.sh) script. 
The relevant environment variables for the installation procedured are listed in the [section](#environment-variables) below. 

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

Similar to install script the fowllowing options are available:

```
	./update.sh --no-sudo
```
In case the system has no sudo command available. Usefull for Docker based installations. 

 OR
```
	./update.sh --no-git
```
In case no git pull sould be invoked. 


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
   encoded as JSON messages via ZMQ. The distribution of the [Task Specification Tree](examples/tst/README.md)
   makes use of this API.	 

3. By creation of a dedicated *bridge* as done for ROS [TF](https://github.com/blumenthal/sherpa_world_model_tf_bridge) 
   trees. Please note, that the implementation of the TF bridge embedds a *World 
   Model Agent* as stated my method 1 and serves as a good reference example.


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
Last update: 25.01.2015
 


