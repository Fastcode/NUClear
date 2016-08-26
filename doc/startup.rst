Startup Process
===============

Initialization Phase (single threaded)
**************************************
Reactor's constructors are executed
During this process some reactions may run instantly if using extensions
Initialise scope reactions are executed
Startup reactions are executed

Execution Phase (multithreaded)
*******************************
Reactions run as normal via the thread pool

Shutdown Phase (multithreaded)
******************************
Shutdown event is executed
Existing tasks will be finished
All non direct emits are silently dropped
