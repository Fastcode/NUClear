===============
Startup Process
===============

The runtime processes for NUClear are in 3 main phases, the first being the :ref:`Initialization Phase (single
threaded)`, where the system, reactors, and requests are loaded.  The next being the :ref:`Execution Phase
(multithreaded)` which can be thought of as the general runtime of the system, and the final being the :ref:`Shutdown
Phase (multithreaded)`, which is the process the system will run through once the shutdown command is executed.

Initialization Phase (single threaded)
**************************************
There are three main activities which need to occur during the system initialization.  These stages will setup the
system, before any runtime execution occurs.  It is important to note that the system will be running on a single
thread throughout this phase, and therefore, the order in which the developer lists specific activity is important.

Install the PowerPlant
----------------------
The Initialization Phase begins with the installation of the PowerPlant.  When installing the PowerPlant, we recommend
the `NUClear Roles system <https://github.com/Fastcode/NUClearRoles>`, which will allow the use of a configuration file
for this process.

.. code-block:: C++

    NUClear::PowerPlant::Configuration config;
    config.thread_count = 1;
    NUClear::PowerPlant plant(config);

Install Reactors
----------------
Once the plant is installed, the reactors can be loaded.

.. code-block:: C++
    plant.install<Reactor1, Reactor2, ... >();

This command will run the reactors constructors, and install them into the PowerPlant. In typical applications, it is
normal to define any binding DSL requests during the reactors construction.  Where necessary, a reactor might emit
initialization data to the system.  *When emitting initialization data to the system, there are a few points of interest
which a developer should consider:*

- As the system is still single threaded at this time, the order in which reactors are installed is significantly
  important.  **Any reactors which emit initialization data during construction should be installed AFTER any
  reactors/reactions which are triggered by that data.**
  *Issues can occur if data is emitted under a :ref:`Scope::LOCAL` **BEFORE** the subscribing reactors/reactions have
  been installed. Consider the case where Reactor1 emits initialization data, for which Reactor2 has an on<Trigger>
  request. The trigger cannot be bound for Reactor1's first emission because the callback associated with task creation
  did not exist at the time of data emission.  As such, an associated task for Reactor2's on<Trigger> request cannot be
  made at this point*
  - **So is there a better way?**   Absolutely!  If a reactor needs to emit data during this phase, it is recommended to
    use :ref:`Scope::Initialise`.  This will put a hold on the data emission, until the next step :ref:`Initialise
    Scope Reactions`, ensuring that any reactions subscribed to the emission will run.
  - **Anything else?**  Emissions during the construction of reactors using :ref:`Scope::DIRECT`, :ref:`Scope::UDP` and
    :ref:`Scope::Network` will trigger any reactions which have already been defined (before the data emission) and
    force any associated tasks to run inline.
  - **Feeling confused?** Its actually really simple.  Checkout the :ref:`Emissions Scope Table` for clarity.
- Most of the on<> statements which run during a reactor's construction will setup the binding requests under which
  Tasks will be created.  Any tasks created as a result of local data emission for these reactions will be queued, and
  will not run until the :ref:`Execution Phase (multithreaded)`.  Note that this is the standard behaviour, and
  recommended practice.  **However, there are exceptions to this behaviour which developers will find useful.**
  For example:
  - on<Configuration>:  This is part of the `NUClear Roles system <https://github.com/Fastcode/NUClearRoles>` and can
    be used during a reactors constructor.  This request will run immediately, as an in-line binding reaction.
  - on<
    **GET INFO FROM TRENT - I THINK IT WAS FILEWATCHER???**

Start the PowerPlant
--------------------
Once the reactors have been installed, and the binding reaction requests have been established, its time to start up
the system.

.. code-block:: C++

    plant.start();

This command will run two main actions before transitioning to the :ref:`Execution Phase (multithreaded)`.

Initialise Scope Tasks
``````````````````````
Any data emissions which were emitted under a the :ref:`Scope::Initialise` will run at this time.  The system is still
single threaded, so these emissions will run one by one, in the order they were installed.  As the emission run, the
associated Tasks will be bound.


DSL Startup Requests
````````````````````
Any tasks which were to run upon startup will be queued for execution now.  For example:
on<Trigger<T>, Startup>



Execution Phase (multithreaded)
*******************************
Reactions run as normal via the thread pool

Shutdown Phase (multithreaded)
******************************
Shutdown event is executed
Existing tasks will be finished
All non direct emits are silently dropped
