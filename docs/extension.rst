=========
Extension
=========

Fusion Engine
*************

TODO what the fusion engine is and how it works

Extending the DSL
*****************

TODO What extending the dsl means and what bits there are

Bind
----

TODO What extension via bind means and how it works

Get
---

TODO what extension via get means and how it works
Is used to get data to run a function with
If the type can be dereferenced, then this is also an acceptable type in the argument list

Precondition
------------

TODO what extension via precondition means and how it works
Used to determine if a reaction should run
if any of the functions return false it doesn't run

Postcondition
-------------

TODO what extension via postcondition means and how it works
All of these always run after a reaction has finished running

Reschedule
----------

TODO what extension via reschedule means and how it works
'Steals' reactions so they can be run elsewhere
Can also be used to replace a reaction with a different reaction, or a modified one

Example Case
************

TODO take one of the parts (sync maybe?) and explain how it is built using extensions
