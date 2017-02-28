==================
Managing Reactions
==================

During system runtime, executing reactions can be managed via the reaction handle.  A reaction handle is provided for
any binding :ref:`On Statements`.  Once an on statement has been bound, the reaction handle will be enabled.
If necessary, reactions can toggle between enabled and disabled during runtime.

The reaction handle provides the following functions for managing a reaction:

enable()
`````````
Enables the reaction and allows it to run.

disable()
``````````
Disables the reaction.  When disabled, any associated tasks will not be created for disabled reactions.  All reaction
configuration is still available, so that the reaction can be enabled when desired.

Note than a reaction which has been bound by an on<Always> request should not be disabled.

enabled(bool set)
`````````````````
Sets the reaction handle to be that of the specified boolean.

enabled()
``````````
Query:  Returns true if the reaction is enabled.  False if otherwise.

unbind()
````````
Unbinds a task and removes it from the runtime environment.  This action is not reversible, once a reaction has been
unbound, it is no longer available for further use.  This is most commonly used for the unbinding of network
configuration before attempting to re-set during runtime.


.. todo::

  Keep LOOKING INTO!!!!???
  How is the reaction handle provided to the reactor to use?  Review and see if you can describe
  Note:  How do I get doxygen reaction handle comments into here?  have tried:
  .. doxygenstruct:: NUClear::threading::ReactionHandle
  and
  .. doxygenstruct:: NUClear::threading::ReactionHandle::enable
