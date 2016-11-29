Overview
========

NUClear is a C++ software framework designed to aid in the development of real time modular systems.
It is built from a set of template metapgrograms that control the flow of information through the system.
These metaprograms reduce the cost of routing messages between modules resulting in a much faster system.
It is highly extensible and provides several attachment points to develop new DSL keywords if needed.

NUClear is specifically designed for use in systems that require low latency between components and require easy access to data throughout the system.
It utilises a system called co-messaging to allow much simpler event callback functions through an expressive and extensible domain specific language (DSL).

NUClear has been successfully applied in several projects robotics and virtual reality.

If you're starting a new project using NUClear the `NUClear Roles system <https://github.com/Fastcode/NUClearRoles>`_ is highly recommended as it wraps much of the complexity of managing modules.

Components
**********

The most significant components of a NUClear system are the `PowerPlant`_, `Reactors`_, `Reactions`_ and `Tasks`_.
It is the interaction between these components that ensures data and code are easily accesible.

PowerPlant
----------

TODO what the powerplant is and what it does
TODO talk about installing reactors and managing threads and tasks

Reactors
--------

TODO what a reactor is and what it does
TODO talk about acting as a modular element, continas a bunch of reactions

Reactions
---------

TODO defined by a DSL statement in NUClear
TODO statement determines when

Tasks
-----
