# SHERPA World Model User Manual

The **SHERPA World Model** (SWM) is a distributed envionment representation for
robots based on the Robot Scene Graph (RSG) data representation.  

This manual describes the following aspects of the world model:

 1. The [data model](#data-model)
 2. The [update](#updates) capapilities
 3. The [query](#queries) capapilities
 4. The [distribution](#distribution) capapilities  
 5. World model [debugging](#debugging) techiques
 
## Data model

The underlying data model is a graph. Nodes can represent physical and virtual 
objects in the environemnt. Their relations are expressed by edges. There are 
different types of nodes and edges. We denote such a graph as *Robot Scene Graph
(RSG)*. 

Nodes are destinguished between *inner nodes* 
and *leaf* nodes. An inner node is labelled as **Group** and a leaf as **Node**. 
Groups are used to model containment of objects. E.g. a room can *contain* chairs 
and tables. Another example is a serach area in a Search And Rescue (SAR) mission that
*contains* found victims. In that case the room and the search area would be represented
as **Groups**. **Nodes** are used to model enteties in the environment that do not
*contain* further objects like a way point on a path or a sensor measurement. 
   
All node types i.e. inner nodes an leaf nodes share common properties:

* Each node has a *Universally Unique ID*s (UUID) according to 
  [RFC 4122](https://www.ietf.org/rfc/rfc4122.txt). This is used as
  a primary key to adress all primitives in the graph. 
* Each node can have multiple *parent* nodes. The root node is a node that has no
  parents.
* Each node has attached [**Attributes**](#attributes). These attributes are stored as a list 
  of key value pairs. This mechanism allows to tag graph entities with e.g. 
  semantic attributes or debug information. However the exact usage and meaning
  depends on the actually application and requires a convention.  

For inner nodes (Groups) the following additional property holds:

* Each **Group** can have multiple child nodes. Parents and child nodes effectively form a
  Directed Cyclic Graph (DAG). This reflects a hierarchically modeling of containment
  relations. E.g. the following parent child relation ``building->room->chair`` means ``building``
  contains ``room`` and ``room`` contains ``chair``. The containment relation is *transitive*, thus
  the ``building`` contains the ``chair`` as well. 

There are also more specialized [nodes](#specialized-nodes) and 
[edges](#specialized-edges) es explained later. For instance the **Transform**
is used to represent rigid transforms between objects.

### Attributes

Each node i.e. all **Nodes** and **Groups** can have attached attributes.
A single **Attribute** is an information that can be attached, removed or altered
at any point in time. (In contrast to a *property* which is understood here as an
information that always has to be present). In the remainder of this manual
we use the folloing tuple notation for an **Attribute**: (``attribute``,``value``)
An example of an attribute is a human readable ``name`` for an object: (``name``,``robot_123``).  

The **Attributes** allow to start a search queries on all primitives in the Robot Scene Graph
and identify those nodes specified by the attributes. Further details on how to query the graph
can be found in section [Queries](#queries).

The **Attributes** concept can be seen as "semantic tagging" of the elements in a scene. 
These tags could contain generic information like a name or a node type or task specific tags.
In order to better structure the semanic meaning of attributes we define that 
**Attributes** belong can belong to a *Semantic Context*. 

A *Semantic Context* groups all possible attribute-value combinations that can be interpreted
within a certain context. e.g. tags for the domain of geometric shapes, rigid transforms or application
specific ones. A *Semantic Context* is formalized in a (JSON) Schema definition. Per convention 
a unique namespace intentifier is used as a prefix to the ``attribute``, seperated by a colon ``:`` 
to reference to a specific *Semantic Context*. 
An example is  (``tf:max_duration``,``10s``) for the *Transform* domain that uses ``tf`` as a prefix.

Attributes that do not belong to a *Semantic Context* are not supposed to by interpreted, though they
can be helpful to improve human readability or can be used for debugging purposes. 



### Specialized Nodes

#### GeometricNode

A GeometricNode to stores geometric shape data as a leaves node in the RSG. 
The geometric node is a rather general container for any kind of 3D data.
Possible data ranges from rather primitive shapes like a box or a cylinder to
unconstrained geometries like point clouds or meshes. A GeometricNode can
be used to store measurements from sensors.

#### Sherpa specific Nodes


* **ARTVA Signals**: A single ARTVA measurement is represented by a single ``Node``, 
which is tagged with a ``sherpa:artva_signal`` **Attribute** e.g. (``sherpa:artva_signal``,``77``).
It typically has a Connection to a GIS origin via a **Transform** to inticates its geo-pose. 
Examples to [add](../examples/json_api/add_artva_signal.py) and [query](../examples/json_api/get_artva_signals.py) ARTVA measurements can be found in the JOSN API section.

The complete list of Attributes used for a SHERPA mission can be found in the [codebook](https://github.com/blumenthal/sherpa_world_model_knowrob_bridge/blob/master/doc/codebook.md).

### Specialized Edges

So far nodes can be related via *contaiment* relations, which are modled as parent-child
edges. However, a robotic environemnt representaion exhibits much more relations
including topoligic relations (on, near) pose information and others. A generic 
relation between nodes is labelled as **Connection** while pose information
is modeled with **Transforms**.   

#### Connection

TBD

#### Transform

TBD

## Updates

An update is defined as a **C**reate, **U**pdate or **D**elete operation on the
primitive elements of the Robot Scene Graph. I.e. it is possible to add new
objects to the world model by creating a *Group* or *Node*, adding a *Transform*
primitive (to represent relative poses between nodes) and by attaching any 
additional [Attributes](#attributes) as needed. 

Different interfaces for interacting with the RSG exist for 
[C++](http://www.best-of-robotics.org/brics_3d/classbrics__3d_1_1rsg_1_1ISceneGraphUpdate.html), 
[Java](https://github.com/blumenthal/brics_3d_jni/blob/master/java/RobotSceneGraph/src/be/kuleuven/mech/rsg/Rsg.java)
 (which is not feature complete!), and [JSON](../examples/json_api). 

The following table with pseudo code illustrates the update **operations** on the graph primitives:


| Operation            	| Pseudo Code                                                                      			| Description                                                                                                                                                                                                                                                                   	|
|----------------------	|-------------------------------------------------------------------------------------------|-------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------	|
| Create Node          	| ``addNode(id, parentId, attributes)``                                         			| ``id`` specifies the UUID. If not set the implementation will generate one. The ``parentId`` denfines the parent note. In cases of doubt use the root node id. ``attributes`` defines a set of initial Attributes. They might be updated in later steps and can be empty as well. 	|
| Create Group         	| ``addGroup(id, parentId, attributes)``                                        			| Same for id, parentId and attributes as for Create Node operation.                                                                                                                                                                                                                                           	|
| Create GeometricNode 	| ``addGroup(id, parentId, attributes, geometry, timeStamp)``                   			| Same for id, parentId and attributes as for Create Node operation, but the geometric shape like a box for instance has to be defined. The time stamp denotes the creation time. The geometric data is immutable. Though the GeometricNode might be deleted as a whole.                                             |
| Create Connection    	| ``addConnection(id, parentId, attributes, sourceIds, targetIds, startTime, endTime)`` 	| Same for id, parentId and attributes as for Create Node operation. ``sourceIds`` and ``targetIds`` define sets of source/target nodes reeferred to by its IDs (ingoing/outgoing arcs) that will be set for the connection. Can be empty. ``startTime`` denots since when a connection is valid. Recommended is the time of creation. ``endTime`` denots until when a connection is valid. Recommended is an infitite time stamp. Both time stamps can be updated later. |
| Create Transform     	| ``addTransform(id, parentId, sourceId, targetId, attributes, transform, timeStamp)`` 		| Same for id, parentId and attributes as for Create Node operation but the initial transform has to be given with an accompanying time stamp. The data can be updated afterwards. The ``sourceId`` and ``targetId`` denote between which nodes the transform holds. Cf. [Transform](#transform) section.                    	|
| Create Parent-Child   | ``addParent(id, parentId)`` 																| Adds an **additional** parent-child relation beween ``id`` and ``parentId``. (There is always at least one during creation of a node.)             	|
| Update Attributes 	| ``updateAttributes(id, newAttributes)`` 													| ``id`` defines the node to be updated. ``newAttributes`` defines the new set of attributes that replaces the old one.  	|
| Update Transform	 	| ``updateTransform(id, transform, timeStamp)`` 											| ``id`` defines the Transform to be updated. ``transform``	holds the transform data that will be inserted in to the temporal cache at the given time ``timeStamp``.   	|
| Delete Node		 	| ``deleteNode(id)``							 											| The ``id`` the defines the node to be deleted.	|
| Delete Parent		 	| ``deleteParent(id, parentID)``							 								| Remove an existing parent-child relation between two nodes specified by ``id`` and ``parentId``. If the last perent is deleted, then it is treated as ``deleteNode(id)`` 	|

Examples for using the JSON API to update the graph can be found for [here](../examples/json_api)

## Queries

An query is regarded as a **R**ead operation on the graph. Depending on the type of 
query this can involve a traversal of the primitives in the graph.

The below table with pseudo code illustrates the **queries** on the graph **primitives**:


| Operation            	         | Pseudo Code                                                                      	     | Description                                                                                                                                                                                                                                                                   	|
|--------------------------------|-------------------------------------------------------------------------------------------|-------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------	|
| Search nodes by Attributes     | ``findNodes(attributes, resultIds)``                                         			 | Find all nodes that have at least the specified ``attributes``. Regular exressions (POSIX) are possible as well. Returns ``resultIds`` which is a set of ids. 	|
| Search transform between nodes | ``getTransformForNode (id, idReferenceNode, timeStamp, transform)``                       | Calculates a rigid transform between nodes specified by ``id` and ``referecnceId`` in the graph. ``referecnceId`` denotes the *origin* of the transform. Depending on how the graph looks like there is not always a solution. |
| Read Attributes                | ``getAttributes (id, attributes)``                                         		         | Get the attributes of node with id ``id``. Returns all Attributes in output parameter ``attributes``.	|
| Read Parents                   | ``getParents (id, perentIds)``                                         		             | Get the parenbts ids of node with id ``id``. Returns all parents in output parameter ``parentIds``.	|
| Read Children                  | ``getChildren (id, childIds)``                                         		             | Get the child ids of node with id ``id``. Returns all childs in output parameter ``childIds``.	|
| Read Transform                 | ``getTransform (id, timeStamp, transform)``                                         		 | Get the data of a Transform with id ``id`` at time ``time stamp``. Returns entry that best matches the to the given time stamp.	|
| Read Geometry                  | ``getGeometry (id, geometry, timeStamp)``                                         		 | Get the data of a GeometryNode with id ``id``. Returns the geometric shape and the accompanying time stamp.	|


Examples for using the JSON API to query the graph can be found for [here](../examples/json_api)

## Distribution

The distribution capabilities strive for all SWMs having a **local copy** of the 
(more or less) same Robot Scene Graph. Technically speaking, every update of
the SWM, that is the creation of new nodes or an update will be send to all
other listening SWMs. It can be seen as broadcasting diffs of the world model.
Here, the *Mediator* is in charge to properly broadcast the data to the other 
robots over a potentially unreliable network.

The SWM has a mechism that sends the full graph to all other SWMs. It is triggered
when a new SWM comes "up". An *advertisement* message is send when a new SWM joins.
It is  also possible to trigger it in case of missing data (but this is not yet implemented).
For debugging purposes it can be triggered manually as well via the ``sync()`` 
[terminal commnad](#terminal-commands).

In all distribution scenarios all World Model Agents must have UUIDs and a communication framework with or 
without a Mediator (recommeded) has to be selected.

### World Model Agent UUIDs

Before launching a distributed scenario it is strongly advised that every World Model Agent, thus every SWM 
has a unique UUID. It can set in the respective section in the ``sherpa_world_mode.usc`` file or as an
environment variable. If the agent IDs are the same, the *advertisment* message that automatically synchronizes agents on start up will be ignored. 


Set the environment variable ``SWM_WMA_ID`` to the UUID to be used. E.g.:

```
export SWM_WMA_ID=e379121f-06c6-4e21-ae9d-ae78ec1986a1
```


Or directly modify the default value in the usc file:

```
local worldModelAgentId = getEnvWithDefault("SWM_WMA_ID", "e379121f-06c6-4e21-ae9d-ae78ec1986a1") 
```

It is also possible to ommit that step and let the World Model Agent automatically generate an ID.
Simply leave the SWM_WMA_ID empty:


```
 export SWM_WMA_ID= 
```

Of course, all individual agents (with individual IDs) will have to be connected. The easiest way is to 
define a global application ID and enable the "auto mount" capabilities of the SWM. I.e. whenever
a new agent joins it is automatically added as child to that global application ID. In order
to activate this functionalyly simply let the World Model Agent automatically generate one as described above
and set the ``SWM_GLOBAL_ID`` to the global ID. E.g.   

```
 export SWM_WMA_ID=
 export SWM_GLOBAL_ID=e379121f-06c6-4e21-ae9d-ae78ec1986a1 
```

Note in this example the ``SWM_GLOBAL_ID`` uses the same Id as in the above exambles for a fixed WMA ids. 
This can be a helpful migration stategiy in cases some applications assume the existance of a particular (global) root Id. 

### Without Mediator

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

### With Mediator

TBD

## Debugging

This section presents methods to understand if the SWM is working properly.
It involves to qickly asses *what* is exactly stored and if some particular 
data is missing *why* is it not stored although it is expected to be there. 


### Dump of data model

Dump the SWM by using the ``dump_wm()`` command then call on another shell:
 ``./latest_rsg_dump_to_pdf.sh``. This will generate a pdf file that can be further
inspected. All Nodes, Attributes and Connection will be visualized. The ``Transforms``
only depict the latest translation value (to get a rough idea).  

<img src="rsg_dump_example.png" alt="Example of a World Model Dump."" width="700px">
 
Check if:
 * Nodes, Connections or Attributes are missing. If yes, definitely check the [status](#status-of-modules) 
   of the modules and the [log](#log-messages) messages. If you are expecting data
   from another World Model Agent check the connectivity of the network.
 * Parent-child relations are as expected. 
 * Connections relation are as expected (source and targed nodes could be swithed). 
 * Attributes contain typos, are missing or have a wrong namespace prefix. 

### Status of modules

Enter ``localhost:8888`` into a web browser and check if the relavant modules are active (green font). 
Please note, the status changes based on the used [terminal](#terminal-commands) commands. When all module 
are inactive - did you forgot to call ``start_all()``? 


### Terminal commands

The typical workflow is to start the SWM and then call ``start_all()`` by typing
it into the interactive console and hitting enter.
There are also other commands available that allow to go step by step. Here the 
``start_all()`` is a convenience function.

| Command        | Description                                                |
|----------------|------------------------------------------------------------|
| ``start_wm()`` | Starts all core and communication modules for the SWM.      |
| ``scene_setup()`` | Loads an RSG-JSON file as specified in the configuration. |
| ``load_map()`` | Loads an OpenSteedMap from a file as specified in the configuration. |
| ``sync()``     | Manually trigger to send the full RSG to all other World Model Agents. |
| ``start_all()``| Conveninece function that calls per default ``start_wm()`` and ``load_map()``. |


### Log messages
Look for ``[ERROR]`` and ``[WARNING]`` messages printed into the intercative terminal. 
``[DEBUG]`` messages can be mostly ignored. 


The debug levels can be configured in the ``sherpa_world_model.usc`` file for 
most modules by specifying the ``log_level`` variable e.g.

```
{ name="rsgjsonreciever", config =  { buffer_len=20000, wm_handle={wm = wm:getHandle().wm}, log_level = 2 } },
```

Only messages with his level or a higher level are displayed. The possible log levels are ``DEBUG`` = 0, 
``INFO`` = 1, ``WARNING`` = 2, ``LOGERROR`` = 3 and ``FATAL`` = 4. The default is ``INFO``. Be aware that 
the ``DEBUG`` level is very verbose and can cause significant load on a system (in particular on embedded
 systems like used the SHERPA Wasps).  


A rather common ``WARNING`` message is ``Forced ID`` *some_uuid* ``cannot be assigend``. It
means there is already a graph primitive with exactly that ID so this operation will be ignored. 

```
> zmq_receiver: data available.
D: 16-02-12 10:19:19 [310] {"node": {"attributes": [{"value": "myNode", "key": "name"}, {"value":...
[WARNING] Forced ID 92cf7a8d-4529-4abd-b174-5fabbdd3068f cannot be assigend. Probably another object with that ID exists already!
```

This warning is fine when data is received more than once form another World Model Agent 
or if a RSG-JSON or an OpenStreetMap file is (accidentaly) loaded multiple times.
However, if you are missing a primitive that should be loaded e.g. from a RSG-JSON
file it indicates that there is a mistake in the file. Check if it contains 
multiple entries with the same ``id``.

