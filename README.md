RSG integration into Microblox (UBX)
==========================================================

Overview
--------

RSG integration into Microblox (UBX). Provides system composition
models for the **SHERPA world model**.

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

The ``source`` command is important because it adds environment variables (cf. [section below](#environment-variables))
that are required for the subsequent installation steps.


### Manual installation

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

Please make sure the following environment variables are set. (The should be ) 

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
| HDF5_ROOT (optional)     | Points to the installation folder of HDF5. Use it in case it is not into installed to the default folders. |



Usage
-----


You can start the SHERPA World Model by invoking:
```
  ./run_sherpa_world_model.sh --no-ros
```

In case the ROS communication modules are used also start roscore:
```
  roscore&
  ./run_sherpa_world_model.sh
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
 

Ather examples can be obtained from the [examples section](examples).


Licensing
---------

This software is published under a dual-license: GNU Lesser General Public
License LGPL 2.1 and Modified BSD license. The dual-license implies that
users of this code may choose which terms they prefer. Please see the files
called LGPL-2.1 and BSDlicense.


Impressum
---------

Written by Sebastian Blumenthal (blumenthal@locomotec.com)
Last update: 30.10.2015
 


