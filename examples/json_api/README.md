JSON API examples
==================

These examples demonstrate how to store and retrieve data from 
the SHERPA World Model via usage of JSON messages.

Installation 
------------

This example requires the [SHERPA World Model](../../README.md)  and Python3 with the ZMQ package to be installed.


### Ubuntu 12.04:
```
	sudo apt-get install python3 python3-setuptools python3-dev
	sudo easy_install3 pip
	pip3 install cffi pyzmq	
```


Usage
-----

Add data to the world model:

```
  python3 add_node.py 
  python3 add_pose.py 
```

Retreive data from the world wodel:

```
  python3 get.py node_attributes_query.json 
  
  python3 get.py nodes_query.json 
  
  python3 get.py root_node_query.json 
  
  python3 get.py transform_query.json  

  python3 get.py geometry_query.json 
```

Complete example output from a query:

```
python3 get.py transform_query.json  

Sending query: {
"@worldmodeltype": "RSGQuery",
  "query": "GET_TRANSFORM",
  "id": "3304e4a0-44d4-4fc8-8834-b0b03b418d5b",
  "idReferenceNode": "e379121f-06c6-4e21-ae9d-ae78ec1986a1",
  "timeStamp": {
    "@stamptype": "TimeStampDate",
    "stamp": "2015-11-12T16:16:44Z",
  } 
}
 
Received result: b'{"@worldmodeltype": "RSGQueryResult","query": "GET_TRANSFORM","querySuccess": true,"transform": {"matrix": [[1.0000000000000000,0.0000000000000000,0.0000000000000000,1.6000000000000001],[0.0000000000000000,1.0000000000000000,0.0000000000000000,1.5000000000000000],[0.0000000000000000,0.0000000000000000,1.0000000000000000,0.0000000000000000],[0.0000000000000000,0.0000000000000000,0.0000000000000000,1.0000000000000000]],"type": "HomogeneousMatrix44","unit": "m"}}' 
```  
