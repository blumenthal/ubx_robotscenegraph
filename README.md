Function blocks for the BRICS_3D library
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
 


