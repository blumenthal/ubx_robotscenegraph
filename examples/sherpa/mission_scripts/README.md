Mission scripts
---------------

This folder contains auxiliary scripts for populating the DCM/SherpaWorldModel for a SHERPA mission.

Dependencies
------------

Tested with Python 2.7

Usage
-----

  *   ``python swm.py initialise/initialize`` initialise the DCM structure if it wasn't already defined by running the scene_setup() command;
  *   ``python swm.py add genius/donkey`` add a genius/donkey node and initialise the DCM structure if it isn't already available: only a genius/donkey node can be added on the tree so the function return an error when the user try to add a busy genius/donkey when another one was already added.
  *   ``python swm.py add wasp (NAME)`` add the node of the wasp named (NAME) and initialise the DCM structure if it isn't already available.
  *   ``python swm.py set genius/donkey geopose (LAT) (LON) (ALT) (Q0) (Q1) (Q2) (Q3)`` create/set the geopose node of the genius/donkey with the position (LAT) (LON) (ALT) and the quaternion (Q0) (Q1) (Q2) (Q3) and create the DCM structure and/or the genius/donkey node if it isn't already available.
  *   ``python swm.py set wasp (NAME) position (LAT) (LON) (ALT) (Q0) (Q1) (Q2) (Q3)`` create/set the geopose node of the wasp (NAME) with the position (LAT) (LON) (ALT) and the quaternion (Q0) (Q1) (Q2) (Q3) and create the DCM structure and/or the wasp node if it isn't already available.
  *   ``python swm.py set genius/donkey geopose (LAT) (LON) (ALT) (Q0) (Q1) (Q2) (Q3)`` create/set the geopose node of the genius/donkey with the position (LAT) (LON) (ALT) and the quaternion (Q0) (Q1) (Q2) (Q3) and create the DCM structure and/or the genius/donkey node if it isn't already available.
  *   ``python swm.py set wasp (NAME) geopose (LAT) (LON) (ALT) (Q0) (Q1) (Q2) (Q3)`` create/set the geopose node of the wasp (NAME) with the position (LAT) (LON) (ALT) and the quaternion (Q0) (Q1) (Q2) (Q3) and create the DCM structure and/or the wasp node if it isn't already available.

Authors
-------

* Marco Melega 
