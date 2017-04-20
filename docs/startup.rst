===============
Startup Process
===============

The runtime process for NUClear has 3 main phases;  the :ref:`Initialization Phase (single threaded)`, where the
system, reactors, and requests are loaded, the :ref:`Execution Phase (multithreaded)` which can be thought of as the
general runtime of the system, and the :ref:`Shutdown Phase (multithreaded)`, which is the process the system will run
through once the shutdown command is executed.

Initialization Phase (single threaded)
**************************************
There are three main activities which need to occur during the system initialization.  These stages will setup the
system, before any standard runtime execution occurs.  It is important to note that the system will be running on a
single thread throughout this phase, and therefore, the order in which the developer lists specific activities is
important.

Install the PowerPlant
----------------------
This phase begins with the installation of the PowerPlant.  When installing the PowerPlant, it is recommended to use
the `NUClear Roles system <https://github.com/Fastcode/NUClearRoles>`_, as it allows the use of a configuration
file for the process.

.. code-block:: C++

    NUClear::PowerPlant::Configuration config;
    config.thread_count = 1;
    NUClear::PowerPlant plant(config);

.. todo::

    Requires a link to details about the Nuclear Roles config details.

Install Reactors
----------------
Once the plant is installed, the reactors can be loaded.

.. code-block:: C++

    plant.install<Reactor1, Reactor2, ... >();

This command will run the reactors constructors, and install them into the PowerPlant.  There are two main aspects
developers should consider when designing a reactor's construction.

On Statements
~~~~~~~~~~~~~

Most of the :ref:`On Statements` which run during a reactor's construction will setup the binding requests under which
:ref:`Tasks` will be created.  Any tasks created as a result of local data emission for these reactions will be queued,
and will not run until the :ref:`Execution Phase (multithreaded)`.  Note that this is the standard behaviour, however
**there are exceptions to this behaviour which developers will find useful.**  For example:

- on<Configuration>:  This is part of the `NUClear Roles system <https://github.com/Fastcode/NUClearRoles>`_ and can
  be used during a reactors constructor.  This request will run immediately, as an in-line binding reaction.
- on<fileWatcher>:  This is part of the `NUClear Roles system <https://github.com/Fastcode/NUClearRoles>`_ and can
  be used during a reactors constructor.  This request will run immediately, as an in-line binding reaction.

.. todo::

    Requires a link to details about the Nuclear Roles on config and on filewatcher stuff.

Data Emission Statements
~~~~~~~~~~~~~~~~~~~~~~~~

As the system is single threaded at this time, the order in which reactors are installed can be significantly important.
This is pertinent when dealing with any data emissions during reactor construction which are NOT emitted under
:ref:`Scope::Initialise`.  For example; data emission during the construction of a reactor using :ref:`Scope::DIRECT`,
:ref:`Scope::UDP`, or :ref:`Scope::Network` will trigger any necessary activity to run inline. Should any reactions be
defined to run as a result of the emission, the task will be generated and also run inline. It is here where the order
in which reactors are installed becomes important.  Suppose Reactor1 were to emit under :ref:`Scope::DIRECT`, and
Reactor2 had a reaction defined to run on the associated datatype. In this case, the reaction defined by Reactor2 would
not run, as it was not yet defined at the time of data emission.  However, should the roles be reserved, then the
reaction would run.

**General Rule of Thumb:**  If a reactor needs to emit data during this phase, it is recommended to use
:ref:`Scope::Initialise`.  This will put a hold on the data emission, until the next step in the process
:ref:`Initialise Scope Tasks`, ensuring that any reactions subscribed to the emission will run.  Checkout the
:ref:`Emissions Scope Table` for clarity.

.. todo::

    Old text:   Keeping it here as it might be useful at some point.
    In typical applications, it is normal to define any binding DSL requests within a reactor's constructor.  However,
    where  necessary, a reactor might emit initialization data to the system.  *When emitting initialization data to the
    system, there are a few points a developer should consider:*

    As the system is single threaded at this time, the order in which reactors are installed is significantly important.
    **Any reactors which emit initialization data during construction should be installed AFTER any reactors/reactions
    which are triggered by that data.**  Issues can occur if data is emitted under a :ref:`Scope::LOCAL` **BEFORE** the
    subscribing reactors/reactions have been installed. Consider the case where Reactor1 emits initialization data, for
    which Reactor2 has an on<Trigger> request. The trigger cannot be bound for Reactor1's first emission because the
    callback associated with task creation did not exist at the time of data emission.  As such, an associated task for
    Reactor2's on<Trigger> request cannot be made at this point.

    **So is there a better way?**   Absolutely!  If a reactor needs to emit data during this phase, it is recommended
    to use :ref:`Scope::Initialise`.  This will put a hold on the data emission, until the next step in the process
    :ref:`Initialise Scope Tasks`, ensuring that any reactions subscribed to the emission will run.

    Anything else?**  Emissions during the construction of reactors using :ref:`Scope::DIRECT`, :ref:`Scope::UDP` and
    :ref:`Scope::Network` will trigger any reactions (which have already been defined - before the data emission) and
    force any associated tasks to run inline.

    **Feeling confused?** Its actually really simple.  Checkout the :ref:`Emissions Scope Table` for clarity.


Start the PowerPlant
--------------------
Once the reactors have been installed, and the binding reaction requests have been established, its time to start up
the system.

.. code-block:: C++

    plant.start();

This command will run two main actions before transitioning to the :ref:`Execution Phase (multithreaded)`.

Initialise Scope Tasks
~~~~~~~~~~~~~~~~~~~~~~

Any data emissions which were emitted under a the :ref:`Scope::Initialise` will run at this time.  The system is still
single threaded, so these emissions will run one by one, in the order they were installed.  As the emission run, the
associated Tasks will be bound.  Tasks generated under these emissions will be queued, but will not start execution
until the :ref:`Execution Phase (multithreaded)`.  Tasks will be queued based on their priority level, then their
emission timestamp.

DSL Startup Requests
~~~~~~~~~~~~~~~~~~~~

Any requests using an on<Startup> definition will be generated and will run now. These requests will run one-by-one,
using the order in which they were installed.  Once these tasks have completed processing, the system will transition
to the next phase.

Execution Phase (multithreaded)
*******************************
This phase is generally referred to as the standard system runtime.  During this phase, the threadpool will be started.

Once started, any reactions requested with an on<:ref:`Always`> definition will start running.

The system will then process any reactions requested with an on<:ref:`MainThread`> definition.

From here, any other tasks already queued will be processed and the system will start ticking over as per the setup.

During this phase, the system will be responsive to any of the :ref:`Managing Reactions` commands, as well as any
changes to the run time arguments for reactions defined with :ref:`IO`, :ref:`TCP`, :ref:`UDP`, or any other applicable
:ref:`Extension` from your system.

The system will tick along, until the shutdown command is given, pushing it into the next phase:

.. code-block:: C++

    powerplant.shutdown();

Note that all reactors in the system are given a reference to the powerplant object, so that any reactor/reaction with a
callback access to the powerPlant. Call the shutdown() command under desired conditions.

.. todo::

    Trent -in Audio1 at (58:38)  you say anyone with the powerplant object can shut it down.  Apart from the
    reactors, who else has the powerplant object? -- -NUClear Roles//

Shutdown Phase (multithreaded)
******************************
Once the shutdown event is executed, any existing tasks which were already queued will run and finish as normal.
Any on<:ref:`Shutdown`>() reaction requests will then be queued (in the order in which they were installed) with
:ref:`Priority`::IDLE.

Note that during this phase, any other task which would normally be scheduled as a result of a non-direct emission will
be silently dropped, while any tasks which would occur as a result of a :ref:`Scope::DIRECT` emission will interrupt the
shutdown process and run as normal.

.. todo::

   Trent - did you decide to give on shutdown tasks low priority? i.e; idle

   table below - is not confirming to widths and needs to be updated.   Can generate table properly now though.


Emissions Scope Table
*********************

.. table::

   +--------------------------+---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------+----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------+---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------+
   |                          | :ref:`Initialization Phase (single threaded)`                                                                                                                                                                                                                                                                                               | :ref:`Execution Phase (multithreaded)`                                                                                                                                                                               | :ref:`Shutdown Phase (multithreaded)`                                                                                                                                                                               |
   +==========================+=============================================================================================================================================================================================================================================================================================================================================+======================================================================================================================================================================================================================+=====================================================================================================================================================================================================================+
   | :ref:`Scope::LOCAL`      | Schedules any tasks for reactions which are currently loaded and bound to the emission data.  Adds to the queue of tasks to start running when the system shifts to the :ref:`Execution Phase (multithreaded)`                                                                                                                              | Schedules any tasks for reactions which are bound to the emission data.  Adds to the queue of tasks based on the desired :ref:`Priority`  level                                                                      | Any emissions under this scope while the system is in the shutdown phase are ignored.                                                                                                                               |
   +--------------------------+---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------+----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------+---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------+
   | :ref:`Scope::DIRECT`     | Schedules any tasks for reactions which are currently loaded and bound to the emission data. Pauses the initialization phase, and runs the task in-line.  The initialization phase continues upon task completion.                                                                                                                          | Schedules any tasks for reactions which are currently loaded and bound to the emission data. Pauses the task currently executing and runs the new task in-line.  The execution phase continues upon task completion. | Schedules any tasks for reactions which are currently loaded and bound to the emission data. Pauses the task currently executing and runs the new task in-line.  The shutdown phase continues upon task completion. |
   +--------------------------+---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------+----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------+---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------+
   | :ref:`Scope::Initialise` | Data emitted under this scope during this phase will wait until all reactors have been installed into the powerPlant before triggering any reactions.  Any tasks generated as a result of this emission type are the first tasks to run when the powerPlant starts. This is the recommended emission type for this phase of system startup. | Any emissions under this scope while the system is in the execution phase are ignored.                                                                                                                               | Any emissions under this scope while the system is in the shutdown phase are ignored.                                                                                                                               |
   +--------------------------+---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------+----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------+---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------+
   | :ref:`Scope::DELAY`      | The delay countdown starts at the time of emission.  Once the delay time-frame has passed, any tasks for reactions which are currently loaded and bound to the emission data are scheduled. Adds to the queue of tasks to start running when the system shifts to the :ref:`Execution Phase (multithreaded)`                                | Waits for the associated delay timeframe, then schedules any tasks for reactions which are bound to the emission data. Adds to the queue of tasks based on the desired :ref:`Priority`  level                        | Any emissions under this scope while the system is in the shutdown phase are ignored.                                                                                                                               |
   +--------------------------+---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------+----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------+---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------+
   | :ref:`Scope::UDP`        | Emits the data over the UDP network.  Should any UDP reaction request be loaded in the system based on this data emission, schedules the task to run in-line. Pauses the initialization phase, and runs the task in-line.  The initialization phase continues upon task completion.                                                         | Emits the data over the UDP network.  Should any UDP reaction request be loaded in the system based on this data emission, schedules the task to run in-line.                                                        | Any emissions under this scope while the system is in the shutdown phase are ignored.                                                                                                                               |
   +--------------------------+---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------+----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------+---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------+
   | :ref:`Scope::Network`    | Emits the data over the NUClear network. Should any network reaction request be loaded in the system based on this data emission, schedules the task to run in-line. Pauses the initialization phase, and runs the task in-line.  The initialization phase continues upon task completion.                                                  | Emits the data over the NUClear network. Should any network reaction request be loaded in the system based on this data emission, schedules the task to run in-line.                                                 | Any emissions under this scope while the system is in the shutdown phase are ignored.                                                                                                                               |
   +--------------------------+---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------+----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------+---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------+
