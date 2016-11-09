Zyre client communication example
=================================

* A C library ``libswmzyre`` that can be used to realize Zyre based message passing. 
  It encapsulates the following use case:
  1. Take a valid JSON message for the SWM
  2. If necessary add a randomly generated ``queryId`` field to the message
  3. Add the appropriate envelope
  4. Send the message with envelope via a Zyre *shout* command to the SWM
  5. Receive Zyre messages until either a timeout occurs or a result message with the same ``queryId`` arrives
  6. If a reply was received than, remove the envelop and return the payload - a SWM JSON message -  as response
* A C [example program](swm_zyre.c) that accepts a JSON file as argument and will return the reply by the SWM. 
* A [Python wrapper](../json_api/zyre_add.py) for ``libswmzyre`` to be used as a drop in replacement for the existing [Python examples](../json_api).
  The [add_geo_pose.py](../json_api/add_geo_pose.py) example shows how seamlessly switch between Zyre based massaging and ZMQ REQ-REP that has been
  used in older examples. 


Installation 
------------

This example requires the [SHERPA World Model](../../README.md) and Zyre C libraries to be installed. 
The letter is a requirement of the SWM, thus the example should pretty much compile out of the box:

```
mkdir build
cd build
cmake ..
make
``` 

Usage
-----

### C example

Synopsis:

```
./swm_zyre <path_to_json_file> [<path_to_config_file>]
```

E.g.

```
./swm_zyre ../../json_api/new_node.json
```

Example output:

```
...
[swm_zyre_client] Sent msg: {"payload": {"@worldmodeltype": "RSGUpdate", "operation": "CREATE", "node": {"@graphtype": "Node", "attributes": [{"key": "name", "value": "myNode"}, {"key": "comment", "value": "Some human readable comment on the node."}], "id": "92cf7a8d-4529-4abd-b174-5fabbdd3068f"}, "parentId": "e379121f-06c6-4e21-ae9d-ae78ec1986a1", "queryId": "fda98006-5d63-b4c9-3246-6c1995c40330"}, "metamodel": "SHERPA", "model": "RSGQuery", "type": "RSGQuery"} 
...
[swm_zyre_client] received answer to query fda98006-5d63-b4c9-3246-6c1995c40330:
 {"queryId": "fda98006-5d63-b4c9-3246-6c1995c40330", "@worldmodeltype": "RSGUpdateResult", "updateSuccess": true}
...
``` 

### Python wrapper example

Got to the [JSON API folder](../json_api), then use the [zyre_add.py](../json_api/zyre_add.py) in the same way as the [add.py](../json_api/zyre_add.py): 

```
.../examples/json_api$ python zyre_add.py new_node.json
```

Example output

```
Sending message: {
  "@worldmodeltype": "RSGUpdate",
  "operation": "CREATE",
  "node": {
    "@graphtype": "Node",
    "id": "92cf7a8d-4529-4abd-b174-5fabbdd3068f", 
    "attributes": [
          {"key": "name", "value": "myNode"},
          {"key": "comment", "value": "Some human readable comment on the node."}
    ]
  },
  "parentId": "e379121f-06c6-4e21-ae9d-ae78ec1986a1"
}
... 
[swm_zyre_client] message : {"node": {"attributes": [{"key": "name", "value": "myNode"}, {"key": "comment", "value": "Some human readable comment on the node."}], "@graphtype": "Node", "id": "92cf7a8d-4529-4abd-b174-5fabbdd3068f"}, "@worldmodeltype": "RSGUpdate", "operation": "CREATE", "parentId": "e379121f-06c6-4e21-ae9d-ae78ec1986a1"}
send_json_message No queryId found, adding one.
[swm_zyre_client] send_json_message: message = {"metamodel": "SHERPA", "model": "RSGQuery", "type": "RSGQuery", "payload": {"queryId": "6ee90274-b3f6-3221-4452-e4528e682303", "node": {"attributes": [{"key": "name", "value": "myNode"}, {"key": "comment", "value": "Some human readable comment on the node."}], "@graphtype": "Node", "id": "92cf7a8d-4529-4abd-b174-5fabbdd3068f"}, "@worldmodeltype": "RSGUpdate", "operation": "CREATE", "parentId": "e379121f-06c6-4e21-ae9d-ae78ec1986a1"}}:
...
[swm_zyre_client] received answer to query 6ee90274-b3f6-3221-4452-e4528e682303:
 {"@worldmodeltype": "RSGUpdateResult", "queryId": "6ee90274-b3f6-3221-4452-e4528e682303", "updateSuccess": true}
 Received result: {"@worldmodeltype": "RSGUpdateResult", "queryId": "6ee90274-b3f6-3221-4452-e4528e682303", "updateSuccess": true}
...
```
