include .special.rst

NUClear DSL
===========

On Statements
*************

The On statement is used by :ref:`Reactors` to make subscriptions to the :ref:`PowerPlant`.  Using this statement, developers can set the conditions under which desired :ref:`Reactions` will run.

anatomy of an on statement
--------------------------
.. image:: images/on_statement.svg

The on statement can be seen as containing three main parts.  The DSL Request, the Runtime Arguments, and the Callback.

.. raw:: html

    <font color="blue">DSL Request</font><br />

This defines the kind of reaction which is being requested. The keyword "on" makes the request, while the specified types will define the kind of reaction desired.  For more information please see: :ref:`Trigger`, :ref:`With`, :ref:`Every` and :ref:`Always`,

.. raw:: html

    <font color="red">runtime arguments</font>

TODO

.. raw:: html

    <font color="green">callback</font>

TODO

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

Scope::Initialise
`````````````````
.. doxygenstruct:: NUClear::dsl::word::emit::Initialise

Network Emitting
----------------

Scope::UDP
``````````
.. doxygenstruct:: NUClear::dsl::word::emit::UDP

Scope::Network
``````````````
.. doxygenstruct:: NUClear::dsl::word::emit::Network
