RSG integration into Microblox (UBX)
==========================================================

Overview
--------

RSG integration into Microblox (UBX)

Dependencies
------------

 - brics_3d_function_blocks Installation instructions can be found here: https://github.com/blumenthal/brics_3d_function_blocks
 - BRICS_3D library. Installation instructions can be found here: http://www.best-of-robotics.org/brics_3d/installation.html
 - microblx library. See: https://github.com/kmarkus/microblx

Compilation
-----------

```
 $ mkdir build
 $ cd build 
 $ cmake .. -DCMAKE_CXX_COMPILER=/usr/bin/clang++
 $ make 
```

Environment Variables
---------------------

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

To start the example do the following:

```
sh start_test2.sh 
```

The system will be _initialized_. To actually _start_ it go to the web interface at 
localhost:8888 and click on all dark green _start_ buttons.


Licensing
---------

This software is published under a dual-license: GNU Lesser General Public
License LGPL 2.1 and Modified BSD license. The dual-license implies that
users of this code may choose which terms they prefer. Please see the files
called LGPL-2.1 and BSDlicense.


Impressum
---------

Written by Sebastian Blumenthal (blumenthal@locomotec.com)
Last update: 09.10.2014
 


