Mission scripts
---------------

This folder contains auxiliary scripts for populating the DCM/SherpaWorldModel for a SHERPA mission.

Dependencies
------------

Tested with Python 2.7


Introduction & Setup
--------------------

*pySWM* is a python module designed to allow the creation/update of SHERPA 
scenario files through the addition of static objects and pictures taken from 
SHERPA agents and the addition or modification of SHERPA agents geopose nodes
 on the DCM database. It is based on JSON API included in the ubx_robotscenegraph
 software distribution developed by KUL in bosom of SHERPA project. 
  
 No installation is required for pySWM: simply copy the pySWM folder on a directory of
 the computer to have it ready to use. Before to use it remember to start the 
 SHERPA World Model through the command:  ``roscore&./run_sherpa_world_model.sh`` and 
 enter the command ``start_all()``. Once that the SWM is started it is then possible 
 to use *pySWM* in three ways:

* by issuing the commands through terminal;
* by executing the commands specified in a .dcm file;
* by running these commands from a python script or module that could also be part of a ROS package;

Usage
-----

The available commands are the following ones:

* initialise/initialize: initialise the DCM structure if it wasn't already defined by running the scene_setup() command of the ubx_robotscenegraph software distribution;
* add genius/donkey: add a genius/donkey node and initialise the DCM structure if it isn't already available: only a genius/donkey node can be added on the tree so the function return an error when the user try to add a busy genius/donkey when another one was already added.
* add wasp (TAG): add the node of the wasp identified by the tag (TAG), that shouldn't include any space character, and initialise the DCM structure if it isn't already available.
* add picture (AGENT) (URL) (LAT) (LON) (ALT) (Q0) (Q1) (Q2) (Q3): add the node of the picture taken by agent (AGENT) and stored at the URL address (URL) at the position (LAT) (LON) (ALT) and the quaternion (Q0) (Q1) (Q2) (Q3) and initialise the DCM structure if it isn't already available.
* add river/wood/house/mountain (TAG): add the node of the river/wood/house/mountain identified by the tag (TAG), that shouldn't include any space character, and initialise the DCM structure if it isn't already available.
* set genius/donkey geopose (LAT) (LON) (ALT) (Q0) (Q1) (Q2) (Q3): create/set the geopose node of the genius/donkey with the position (LAT) (LON) (ALT) and the quaternion (Q0) (Q1) (Q2) (Q3) and create the DCM structure and/or the genius/donkey node if it isn't already available.
* set wasp (TAG) geopose (LAT) (LON) (ALT) (Q0) (Q1) (Q2) (Q3): create/set the geopose node of the wasp (NAME) with the position (LAT) (LON) (ALT) and the quaternion (Q0) (Q1) (Q2) (Q3) and create the DCM structure and/or the wasp node if it isn't already available.
* set river/wood/house/mountain (NAME) point (TAG) (LAT) (LON) (ALT): create/set the point identified by the tag (TAG) of the object river/wood/house/mountain (NAME) with the position (LAT) (LON) (ALT) and create the DCM structure and/or the river/wood/house/mountain node if it isn't already available.
* set river/wood/house/mountain (NAME) connections: create the connection node of all the created points of the object river/wood/house/mountain (NAME).

These commands can be run thorugh terminal from the pySWM directory by running the line ``python swm.py <COMMAND>`` where ``<COMMAND>`` is one of the commands of the previous list (e.g. ``python swm.py add genius``).
Otherwise it is also possible to specify these commands in the lines of a file with 
.dcm extension, such as that included in this distribution and to run the following instrution from command line python ``swm.py load <FILE>`` where ``<FILE>`` is the file name with extension (e.g. ``python swm.py load cesena_lab.dcm``). If pySWM find any error in the file it returns a print of the command line specfying the error and the line number of the file. The "#" character can be used to comment lines in the .dcm file.
Finally, it is possible to use pySWM also in a Python script and in a ROS package. An example of ROS node using pySWM is the ``dcm_com`` package that was produced for the SHERPA donkey. Similarly to what is done in the script included in this package it is simply possible to call pySWM functions inside your Python scripts  by adding the following import lines at the beginning of the script:

```
import sys
sys.path.append('/home/path/to/pySWM')
import swm
```

where ``'/home/path/to/pySWM'`` is the path to the pySWM folder. The commands of pySWM can then be executed by running the command swm.run("string") where "string" is the command defined accordingly to the previous list. E.g. to add the donkey node to the SWM it is simply necessary to type the following command: ``swm.run ("add donkey")``.


Authors
-------

* Marco Melega 

Last update: 04.03.2016

