Multi Robot Example
===================

The multi robot example uses ZMQ PUB-SUB as communiation layer. 
The genreric setup is decribed first, followed by a section
on how to launch the example with three robots.  

ZMQ PUB-SUB Communiation layer 
------------------------------

Each SWM has a ZMQ publisher and a ZMQ subscriber, while the publisher binds/owns the port. 
I.e. the subscriber has to know where to connect to.
In order to connect two or three SWMs each subscriber of a SMW has to connect to the publisher of the other SWM(s).

This is used the default setup:

```
  SHERPA WM 1 with IP#1   SHERPA WM 2 with IP#2
 +-----------+           +-----------+
 | PUB(11411)| -- ZMQ -> | SUB       |
 | SUB       | <- ZMQ -- | PUB(11511)|
 +-----------+           +-----------+

```

Please note, that the ports ``11411`` and ``11511`` could be indentical as well. Still it is suggested to seperate them
because typically tests or deggugging sessions are carried out on a single machine. All ports and IPs can ce configured
either in the usc file or via the below environment variables.


Configuration of SHERPA WM 1
```
 export SWM_LOCAL_IP=localhost
 export SWM_LOCAL_OUT_PORT=11411
 export SWM_REMOTE_IP=IP#2
 export SWM_REMOTE_OUT_PORT=11511
```

Configuration of SHERPA WM 2
```
 export WM_LOCAL_IP=localhost
 export SWM_LOCAL_OUT_PORT=11511
 export SWM_REMOTE_IP=IP#1
 export SWM_REMOTE_OUT_PORT=11411
```

#### Example setup with 3 robots:

Here we consider a distributed deployment with three robots: *WASP*, SHERPA-*BOX* and *HMI*. They have the below IPs 
and publish update messages on port ``11411``:

| Robot |      IP       |
|-------|---------------|
| WASP  | 192.168.0.107 |
| BOX   | 192.168.0.110 |
| HMI   | 192.168.0.111 |

To ease debugging we also assign human readable names to each of the agents via the ``SWM_WMA_NAME`` environment variable. 
We set all ports to 11411. We further assume that there is no ROS installed on the WASP robot. 
This results in three individual configurations and launch commands for the robots:  

* WASP (192.168.0.107):
```
  export SWM_WMA_NAME=wasp
  export SWM_REMOTE_IP=192.168.0.110
  export SWM_REMOTE_OUT_PORT=11411
  export SWM_REMOTE_IP_SECONDARY=192.168.0.111
  export SWM_REMOTE_OUT_PORT_SECONDARY=11411
  
  ./run_run_sherpa_world_model --no-ros
  start_all()
```

* BOX (192.168.0.110):
```
  export SWM_WMA_NAME=sherpa_box
  export SWM_REMOTE_IP=192.168.0.107
  export SWM_REMOTE_OUT_PORT=11411
  export SWM_REMOTE_IP_SECONDARY=192.168.0.111
  export SWM_REMOTE_OUT_PORT_SECONDARY=11411
  
  roscore&
  ./run_run_sherpa_world_model
  start_all()
```

* HMI (192.168.0.111):
```
  export SWM_WMA_NAME=hmi
  export SWM_REMOTE_IP=192.168.0.107
  export SWM_REMOTE_OUT_PORT=11411
  export SWM_REMOTE_IP_SECONDARY=192.168.0.110
  export SWM_REMOTE_OUT_PORT_SECONDARY=11411
  
  roscore&
  ./run_run_sherpa_world_model
  start_all()
```

Note, it is also possible to flip ``SWM_REMOTE_IP`` and ``SWM_REMOTE_IP_SECONDARY``. They just need to configure the IPs of the other robots.


Launching the system
--------------------

1.) Configure network settings

C.f. ``robotX.sh`` to set the correct IPs and ports. Default setting is that all robots are simultated on one PC.

2.) ``roscore&``

3.) start 3 new terminal windows


Quickstart:

|         Robot 1 (UGV)   |       Robot 2 (UAV)  |        Robot 3 (HMI) |
|-------------------------|----------------------|----------------------|
| ``./robot_1.sh``        | ``./robot_2.sh``     | ``./robot_3.sh``     |
| ``start_all()``         | ``start_all()``      | ``start_all()``      |
| ``load_map()``          |                      |                      |

Manaual start:

|         Robot 1 (UGV)   |       Robot 2 (UAV)  |        Robot 3 (HMI) |
|-------------------------|----------------------|----------------------|
| ``./robot_1.sh``        | ``./robot_2.sh``     | ``./robot_3.sh``     |
| ``scene_setup()``       | ``scene_setup()``    | ``scene_setup()``    |
| ``start_wm()``          | ``start_wm()``       | ``start_wm()``       |
| ``dump_wm()``           | ``dump_wm()``        | ``dump_wm()``        |
| ``load_map()``          |                      |                      |
| ``dump_wm()``           | ``dump_wm()``        | ``dump_wm()``        |


4.) Make as many snapshaot as needed by typing ``dump_wm()`` into the individual consoles.

5.) Start the robot simulation script in the ``mission_scripts`` subfolder with `` python robot.py ``

6.) Display grapgh via  `` evince `(../../latest_rsg_dump_to_pdf.sh)` ``
