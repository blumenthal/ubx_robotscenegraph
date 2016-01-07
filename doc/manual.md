# SHERPA World Model User Manual

The **SHERPA World Model** is a distributed envionment representation for
robots based on the Robot Scene Graph (RSG) data representation.  

This manual describes the following aspects of the world model:

 1. The [data model](#data-model)
 2. The [update](#updates) capapilities
 3. The [query](#queries) capapilities 

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

TBD

### Specialized Edges

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
 (which is not feature complete!), and [JSON](examples/json_api). 

The following table with pseudo code illustrates the update **operations** on the graph primitives:


| Operation            	| Pseudo Code                                                                      			| Description                                                                                                                                                                                                                                                                   	|
|----------------------	|-------------------------------------------------------------------------------------------|-------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------	|
| Create Node          	| ``addNode(id, parentId, attributes)``                                         			| ``id`` specifies the UUID. If not set the implementation will generate one. The ``parentId`` denfine the parent note. In cases of doubt use the root node id. ``attributes`` defines a set of initial Attributes. They might be updated in later steps and can be empty as well. 	|
| Create Group         	| ``addGroup(id, parentId, attributes)``                                        			| Same for id, parentId and attributes as for Create Node operation.                                                                                                                                                                                                                                           	|
| Create GeometricNode 	| ``addGroup(id, parentId, attributes, geometry, timeStamp)``                   			| Same for id, parentId and attributes as for Create Node operation, but the geometric shape like a box for instance has to be defined. The time stamp denotes the creation time. The geometric data is immutable. Though the GeometricNode might be delete as a whole.                                             |
| Create Connection    	| ``addConnection(id, parentId, attributes, sourceIds, targetIds, startTime, endTime)`` 	| Same for id, parentId and attributes as for Create Node operation. ``sourceIds`` and ``targetIds`` define sets of source/target nodes reeferred to by its IDs (ingoing/outgoing arcs) that will be set for the connection. Can be empty. ``startTime`` denots since when a connection is valid. Recommended is the time of creation. ``endTime`` denots until when a connection is valid. Recommended is an infitite time stamp. Both time stamps can be updated later. |
| Create Transform     	| ``addTransform(id, parentId, sourceId, targetId, attributes, transform, timeStamp)`` 		| Same for id, parentId and attributes as for Create Node operation but the initial transform has to be given with an accompanying time stamp. The data can be updated afterwards. The ``sourceId`` and ``targetId`` denote between which nodes the transform holds. Cf. Transform section.                    	|
| Create Parent-Child   | ``addParent(id, parentId)`` 																| Adds an **additional** parent-child relation beween ``id`` and ``parentId``. (There is alwasy at least one during creation of a node.)             	|
| Update Attributes 	| ``updateAttributes(id, newAttributes)`` 													| ``id`` defines the node to be updated. ``newAttributes`` defines the new set of attributes that replaces the old one.  	|
| Update Transform	 	| ``updateTransform(id, transform, timeStamp)`` 											| ``id`` defines the Transform to be updated. ``transform``	holds the transform data that will be inserted in to the temporal cache as the given time ``timeStamp``.   	|
| Delete Node		 	| ``deleteNode(id)``							 											| The ``id`` the defines the node to be deleted.	|
| Delete Parent		 	| ``deleteNode(id, parentID)``							 									| Remove an existing parent-child relation between two nodes specifies by ``id`` and ``parentId`. If the last perent is deleten, then it is treated as ``deleteNode(id)`` 	|



## Queries

An query is regarded as a **R**ead operation on the graph. Depending on the type of 
query this can involve a traversal of the primitives in the graph.

Examples for using the JSON API can be found for [here](examples/json_api)

TBD

