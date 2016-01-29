Mission scripts
---------------

This folder contains auxiliary scripts for populating the DCM/SherpaWorldModel for a SHERPA mission.

Dependencies
------------

Tested with Python 2.7

Usage
-----

* python swm.py initialise/initialize: initialise the DCM structure;
* python swm.py add bg/donkey: add a busy genius/donkey node and initialise the DCM structure if the user haven't yet done it: only a busy genius/donkey node can be added on the tree so the function return an error when the user try to add a busy genius/donkey when another one was already added.
* python swm.py add wasp (NAME): add the node of the wasp named (NAME) and initialise the DCM structure if the user haven't yet done it.
* python swm.py set bg/donkey position (LAT) (LON) (ALT): create/set the geopose node of the busy genius/donkey with the position (LAT) (LON) (ALT) initialising to 0 the fields of the quaternion and create the DCM structure and/or the busy genius/donkey node if the user haven't yet done it.
* python swm.py set wasp (NAME) position (LAT) (LON) (ALT): create/set the geopose node of the wasp (NAME) with the position (LAT) (LON) (ALT) initialising to 0 the fields of the quaternion and create the DCM structure and/or the wasp node if the user haven't yet done it.
* python swm.py set bg/donkey geopose (LAT) (LON) (ALT) (Q0) (Q1) (Q2) (Q3): create/set the geopose node of the busy genius/donkey with the position (LAT) (LON) (ALT) and the quaternion (Q0) (Q1) (Q2) (Q3) and create the DCM structure and/or the busy genius/donkey node if the user haven't yed done it.
* python swm.py set wasp (NAME) geopose (LAT) (LON) (ALT) (Q0) (Q1) (Q2) (Q3): create/set the geopose node of the wasp (NAME) with the position (LAT) (LON) (ALT) and the quaternion (Q0) (Q1) (Q2) (Q3) and create the DCM structure and/or the wasp node if the user haven't yed done it.


Authors
-------

* Marco Melega 
