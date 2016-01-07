# SHERPA World Model User Manual

The **SHERPA World Model** is a distributed envionment representation for
robots based on the Robot Scene Graph (RSG) data representation.  

This manual describes the following aspects of the world model:

 1. Data model
 2. The query capapilities 

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

The **Attributes** allow to start a search query on all primitives in the Robot Scene Graph
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

### Updates

An update is defined as a **C**reate **R**ead **U**pdate or **D**elete *(CRUD)* 
operation on the primitive elements of the Robot Scene Graph. I.e. it is possible
 to add new objects to the world model by creating a *Group* or *Node*, adding a 
*Transform* primitive (to represent relative poses between nodes)
and by attaching any additional *Attributes* as needed. Geometric data is 



## Queries


Examples for using the JSON API can be found for a [Task Specification Tree](examples/tst/README.md) 
and [here](examples/json_api)

TBD

## Distribution

TBD