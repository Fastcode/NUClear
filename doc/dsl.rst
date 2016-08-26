NUClear DSL
===========

TODO go through all the DSL words and explain what they do/used for and an example

On Statements
*************

TODO explain what an on statement is, and break it down into it's parts (the DSL request, the runtime arguments, and the callback)
TODO explain how fission works for argument selection and how the type of arguments can be deduced as needed (for const references vs shared_ptrs and udp autodeserialisation)

Data Gathering
--------------

Trigger
```````
.. doxygenstruct:: NUClear::dsl::word::Trigger

With
````
.. doxygenstruct:: NUClear::dsl::word::With

Data Modifiers
--------------
Last
````
.. doxygenstruct:: NUClear::dsl::word::Last

Optional
````````
.. doxygenstruct:: NUClear::dsl::word::Optional

Execution Modifiers
-------------------

Single
``````
.. doxygenstruct:: NUClear::dsl::word::Single

Buffer
``````
.. doxygenstruct:: NUClear::dsl::word::Buffer

Priority
````````
.. doxygenstruct:: NUClear::dsl::word::Priority

Sync
````
.. doxygenstruct:: NUClear::dsl::word::Sync

MainThread
``````````
.. doxygenstruct:: NUClear::dsl::word::MainThread

Timing Keywords
---------------

Every
`````
.. doxygenstruct:: NUClear::dsl::word::Every

Always
``````
.. doxygenstruct:: NUClear::dsl::word::Always

Event Keywords
--------------

Startup
```````
.. doxygenstruct:: NUClear::dsl::word::Startup

Shutdown
````````
.. doxygenstruct:: NUClear::dsl::word::Shutdown

IO Keywords
-----------

IO
``
.. doxygenstruct:: NUClear::dsl::word::IO

TCP
```
.. doxygenstruct:: NUClear::dsl::word::TCP

UDP
```
.. doxygenstruct:: NUClear::dsl::word::UDP

Network
```````
.. doxygenstruct:: NUClear::dsl::word::Network


Emit Statements
***************

Local Emitting
--------------
Scope::LOCAL
````````````
.. doxygenstruct:: NUClear::dsl::word::emit::Local

Scope::DIRECT
`````````````
.. doxygenstruct:: NUClear::dsl::word::emit::Direct

Scope::Initialize
`````````````````
.. doxygenstruct:: NUClear::dsl::word::emit::Initialize

Network Emitting
----------------

Scope::UDP
``````````
.. doxygenstruct:: NUClear::dsl::word::emit::UDP

Scope::Network
``````````````
.. doxygenstruct:: NUClear::dsl::word::emit::Network
