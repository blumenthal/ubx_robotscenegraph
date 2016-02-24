Multi Robot Example
===================

Launching the system
--------------------

1.) ``roscore&``

2.) start 3 new terminal windows


|         Robot 1 (UGV)   |       Robot 2 (UAV)  |        Robot 3 (HMI) |
|-------------------------|----------------------|----------------------|
| ``./robot_1.sh``        | ``./robot_2.sh``     | ``./robot_3.sh``     |
| ``start_wm()``          | ``start_wm()``       | ``start_wm()``       |
| ``scene_setup()``       | ``scene_setup()``    | ``scene_setup()``    |
| ``sync()``              |                      |                      |
| ``load_map()``          |                      |                      |


3.) Make as many snapshaot as needed by typing ``dump_wm()`` into the individual consoles.