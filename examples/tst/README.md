Task Specification Tree (TST) example
=====================================

This example demonstrates how to store and receive TST data. 
It uses the JSON API in order to create a new *Node* in the *Robot Scene Graph
[(RSG)](http://www.best-of-robotics.org/brics_3d/worldmodel.html)*.
Currently the TST is stored as a single *Attribute* in that *Node*. As *Attributes*
can be updated the update command is used to represent a TST update as well. 
The exact commands can be found in the respective example [script](rsg_json_test_publisher.py).  

Installation 
------------

This example requires the [SHERPA World Model](../../README.md)  and Python3 with the ZMQ package to be installed.


### Ubuntu 12.04:
```
	apt-get install python3 python3-setuptools
	sudo easy_install3 pip
	pip install pyzmq	
```


Usage
-----

1. Start the SHERPA World Model (in this example with ROS, though it could be also started without ROS)
```	
	rosscore &	
	./run_sherpa_world_model.sh
	start_all()
```

2. In a seperate terminal start the python script that crates a TST *node* and updates its attributes.
```
	python3 examples/tst/rsg_json_test_publisher.py 
```

3. In a third terminal start the subsciber that receives subsequent updates.
```
	python3 examples/tst/rsg_json_test_subscriber.py
```	

The output should look like the following:
```
b'{"@worldmodeltype": "RSGUpdate","node": {"@graphtype": "Node","attributes": [{"key": "name","value": "Task Specification Tree Update"},{"key": "tst:ennodeupdate","value": {"key": "tst:ennodeupdate","value": {"metamodel": "sherpa_msgs","model": "","payload": {"approved": 1,"exec_info": {"can_be_aborted": 1,"can_be_paused": 1},"exec_status": {"aborted": 0,"active": 1,"executing": 1,"finished": 0,"paused": 0,"succeeded": 0,"waiting": 0},"tree_id": "uav0-1","unique_node_id": 2},"type": "update-execution-tst-node"}}}],"id": "ff483c43-4a36-4197-be49-de829cdd66c8"},"operation": "UPDATE_ATTRIBUTES"}'
b'{"@worldmodeltype": "RSGUpdate","node": {"@graphtype": "Node","attributes": [{"key": "name","value": "Task Specification Tree Update"},{"key": "tst:ennodeupdate","value": {"key": "tst:ennodeupdate","value": {"metamodel": "sherpa_msgs","model": "","payload": {"approved": 1,"exec_info": {"can_be_aborted": 1,"can_be_paused": 1},"exec_status": {"aborted": 0,"active": 1,"executing": 1,"finished": 0,"paused": 0,"succeeded": 1,"waiting": 0},"tree_id": "uav0-1","unique_node_id": 2},"type": "update-execution-tst-node"}}}],"id": "ff483c43-4a36-4197-be49-de829cdd66c8"},"operation": "UPDATE_ATTRIBUTES"}'
```	
	
