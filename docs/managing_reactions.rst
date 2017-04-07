==================
Managing Reactions
==================

During system runtime, executing reactions can be managed via the reaction handle.  A reaction handle is provided for
any binding :ref:`On Statements`.  Once an on statement has been bound, the reaction handle will be enabled.
If necessary, reactions can toggle between enabled and disabled during runtime.

The reaction handle provides the following functions for managing a reaction:

disable
```````
.. doxygenfunction:: NUClear::threading::ReactionHandle::disable

enable
``````
.. doxygenfunction:: NUClear::threading::ReactionHandle::enable

enable(bool set)
`````````````````
.. doxygenfunction:: NUClear::threading::ReactionHandle::enable(bool)

enabled
````````
.. doxygenfunction:: NUClear::threading::ReactionHandle::enabled

unbind
``````
.. doxygenfunction:: NUClear::threading::ReactionHandle::unbind
