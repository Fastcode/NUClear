==========
Components
==========
The most significant components of a NUClear system are the `PowerPlant`_, `Reactors`_, `Reactions`_ and `Tasks`_.
It is the interaction between these components that ensures data and code are easily accessible throughout the system.

PowerPlant
**********
The PowerPlant can be thought of as the central message system through which :ref:`Reactors` communicate.

.. image:: images/power_plant.svg
    :width: 500px
    :align: center

Whenever a reactor emits data into the system, the PowerPlant takes ownership of the data and executes any :ref:`Reactions` which are subscribed to the message.

The PowerPlant is also responsible for assigning the threads on which the :ref:`Tasks` will run.  NUClear is a multithreaded environment, with all threading logic centralised and handled by the PowerPlant.  The transparent multithreading uses a threadpool with enough threads to saturate every CPU core in the system, allowing :ref:`Tasks` to execute on a different thread.

By centralising the threading logic, NUClear provides developers the freedom to concentrate on their :ref:`Reactors` instead of threading problems.  The NUClear DSL provides :ref:`Execution Modifiers` should developers wish to execute control over how their threads will run.

Reactors
********
A reactor can be thought of as a module.  In fact, all modules in the NUClear system will be an extension of NUClear::Reactor.

During the :ref:`Startup Process`, the reactors for the system will be installed into the :ref:`PowerPlant`.

From that point on, reactors are primarily responsible for two functions; defining the :ref:`Reactions` and conditions under which they will process, and in some cases, emitting data to the :ref:`PowerPlant` as required.

Reactions
*********
Reactions provide the definitions of the :ref:`Tasks` which need to run when the data and/or conditions required for the reaction become available/true.

To setup a reaction, a reactor can use the functionality provided by NUClear::Reactor to subscribe to the :ref:`PowerPlant` for any messages or conditions of interest.  Under the hood, these functions are bound by NUClear as callbacks, and it is the execution of these callbacks which will assign :ref:`Tasks` to a thread.

Subscriptions to the :ref:`PowerPlant` are made using DSL :ref:`On Statements`.  The conditions for the request are then further defined using the keywords :ref:`Trigger`, :ref:`With`, :ref:`Every` and :ref:`Always`.

Developers can execute further control over reactions in the system with the tools provided for :ref:`Managing
Reactions`.

Tasks
*****
A task is the current execution of a defined reaction within the system.  For debugging purposes, all tasks will track
the following information:

identifier

reaction_id

task_id

cause_reaction_id

cause_task_id

emitted()

started()

finished()

exception()


.. todo::

  Update the above list so that it is trigged by Doxygen and brings in the comments...
