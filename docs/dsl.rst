include .special.rst


NUClear DSL
===========

On Statements
*************

On statements are used by :ref:`Reactors` wishing to make subscriptions to the :ref:`PowerPlant`.  Using this statement,
developers can set the conditions under which desired :ref:`Reactions` will run.

anatomy of an on statement
--------------------------
The on statement can be seen as containing three main parts.  The DSL Request, the Runtime Arguments, and the Callback.

.. raw:: html

    <font color="blue">On<...></font><font color="red">(Runtime, ... )</font><font color="green">.then(function);</font>
    <br /><br />
    <font color="blue">DSL Request</font><br />

This is :red:`red !` And :blue:`this part is blue`.

The DSL request can be fused together through any combination of DSL words.  The combination of these words will define
the kind of reaction which is being requested (for example, :ref:`Trigger` will define a reaction that should occur when
a required data type is emitted, while :ref:`Every` will define periodic reactions).

For reactions to occur, at least one Binding DSL word should be present in the DSL Request. From the provided DSL words,
those which are binding are: :ref:`Trigger`, :ref:`With`, :ref:`Every`, :ref:`Always`, :ref:`Startup`, :ref:`Shutdown`,
:ref:`TCP`, :ref:`UDP` and :ref:`Network`

.. raw:: html

    <font color="red">Runtime Arguments</font>

Some DSL words will provide the ability to make changes to the system during runtime.  This means that NUClear avoids
the need for a system restart should a configuration, port number, or file need to be changed while the system is
running.

From the provided DSL words, those which take runtime arguments are: :ref:`IO`, :ref:`TCP`, and :ref:`UDP`

.. raw:: html

    <font color="green">Callback</font>

Finally, the developer can define the callback which will execute when the reaction is triggered during runtime.  The
callback can be defined using a C++ lambda function.

During system runtime, the argument selection for the callback works on the principle of fission, in that the arguments
provided with the callback can be deduced as needed.  For example:

.. code-block:: C++

    on<<Trigger<A>, Optional<Trigger<B>>().then([](const A& a, const B& b) {

    });

In the above request, the Trigger on dataType B has been listed as optional, while the Trigger for dataType A is listed
as mandatory.  Yet the callback function lists arguments for both dataType A and dataType B.

Lets say that dataType A is emitted to the PowerPlant, but at this time, dataType B does not have any data associated
with it.

Since dataType B was listed as optional, the task associated with this reaction can be scheduled.  However, when
executing the callback for this reaction, NUClear will identify that dataType B is not present, and will remove
reference to this data type from the callback, so that the task is only run for dataType A.

Effectively, through the application of fission, the callback is restructured as per the following example.

.. code-block:: C++

    .then([](const A& a){

    });

.. todo::

    explain how fission works for argument selection and how the type of arguments can be deduced as needed (for const
    references vs shared_ptrs and udp autodeserialisation)

DSL WORDS
----------

The following words are available in the DSL.  Reactors can fuse together their instructions and requests to the
PowerPlant from any combination of these words.  Developers wishing to add their own DSL words to the system can do so
at any time.  Please see:  :ref:`Extension`

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

Watchdog
`````````
.. doxygenstruct:: NUClear::dsl::word::Watchdog


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

Emit statements are used by :ref:`Reactors` wishing to emit data to the :ref:`PowerPlant`. Using this statement,
developers can specify when data will be emitted to the system.

When using NUClear, data will most likely be emitted during a reaction.  However, where necessary, emissions can also
occur during reactor construction (where it is recommended to use :ref:`Scope::Initialise`), or in some cases from
within the PowerPlant itself (for example, when using a third party library which does not have a reactor).

Any data emitted to the PowerPlant will be sent with a unique pointer.  The PowerPlant will take ownership of this
pointer and run any necessary callbacks to trigger reactions (create tasks).

Note that data can be emitted under varying scopes:

Local Emitting
--------------

These emissions send data to the local instance of the NUClear powerplant.  There are a number of scopes under which
these emissions can take place:

.. todo::

    Trent - I need to decide and get consistent on what we will call the powerPlant.  Should it be PowerPlant or
    powerPlant - what will you prefer

Scope::LOCAL
````````````
.. doxygenstruct:: NUClear::dsl::word::emit::Local

Scope::DIRECT
`````````````
.. doxygenstruct:: NUClear::dsl::word::emit::Direct

Scope::Initialise
``````````````````
.. doxygenstruct:: NUClear::dsl::word::emit::Initialise

Scope::DELAY
`````````````
.. doxygenstruct:: NUClear::dsl::word::emit::Delay

Scope::WATCHDOG
``````````````
.. doxygenstruct:: NUClear::dsl::word::emit::Watchdog

Network Emitting
----------------

Network emissions can be used to send data through the network on which the current system is running.

Scope::UDP
``````````
.. doxygenstruct:: NUClear::dsl::word::emit::UDP

Scope::Network
``````````````
.. doxygenstruct:: NUClear::dsl::word::emit::Network
