Multi Robot Example
===================

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
