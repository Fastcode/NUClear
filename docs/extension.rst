=========
Extension
=========

Fusion Engine
*************

The fusion engine combines a number of templated types into a single "fused" type. This allows it to be treated as a
single type. The fused type, most importantly, exposes the methods described below which it then calls in an iterative
manner for each individual type fused.

The parse type is used as the interface into the fusion engine, it handles calling the fused type with the correct
template and filling in undefined methods with no-operations.

Both the fused type and parsed type only has static methods defined, so it would be pointless to instantiate it.

This means calling `Parse<Trigger<int>, Sync, Startup>.bind()` will call `Trigger<int>.get<DSL>()`, then a no-operation,
then will call `Startup.get<DSL>()`, using `DSL = Parse<Trigger<int>, Sync, Startup>`.

Fusions can be nested, this can be useful for creating a DSL word that combines the functionality of multiple other
words.

Extending the DSL
*****************

The DSL words are types, adding a word is as simple as declaring a new type with at least one of the following static
template methods defined. Each undefined static template method will automatically behave as a no-operation.

The template of each of these static methods should be `template <typename DSL>`, this will have the parsed DSL words
passed in. It is important to note that the type will only be considered by NUClear in a static context. If any
attributes need to be stored in the DSL word type template it and use static variables, see `Sync`.

There are DSL words that are not meant to be used directly but as a part of other words, see `CacheGet` and `TypeBind`.
TODO explain what these two do.

If the type you want to become a DSL extension word is not defined within your control specialise `DSLProxy<>` with the
type. Provide the template methods to the specialisation of `DSLProxy<>` as if it were the type.

Bind
----

``` c++
template <typename DSL>
static inline void bind(const std::shared_ptr<threading::Reaction>& reaction, /*More arguments can be declared*/)
```

This function is called when the reaction is bound, it should be thought of as the constructor. It is used to setup
anything that is required by the DSL word.

A common use for extensions is to setup something that will generate tasks for this reaction. This can be done by
communicating to an extension reactor via a helper type that the extension reactor triggers on.

An unbinder, if needed, should be passed to the reaction's unbinders callback list from the bind call. This is used as a
destructor.
e.g. for the `IO` word we have
``` c++
reaction->unbinders.push_back([](const threading::Reaction& r) {
    r.reactor.emit<emit::Direct>(std::make_unique<operation::Unbind<IO>>(r.id));
});
```
which will tell the extension reactor that this reaction no longer exists.

The extra arguments are passed in, in order, from the `on` call.

Get
---

``` c++
template <typename DSL>
static inline T get(threading::Reaction&)
```

This is used to get the data for the callback. The returned value is passed to the callback.

If the return type can be dereferenced, then either the return type or the type returned by the dereference of the
return type can be used in the callback.

If data needs to be passed to a task when it is submitted to the `Powerplant` use `ThreadStore<T>`. The thread store is
a static variable that can be accessed from within the get method.

Precondition
------------

``` c++
template <typename DSL>
static inline bool precondition(threading::Reaction& reaction)
```

A precondition is used to test if the reaction should run. On a true return the reaction will run as normal. On a false
return the reaction will be dropped.

Postcondition
-------------

``` c++
template <typename DSL>
static void postcondition(threading::ReactionTask& task)
```

This will run after the callback for a reaction task has finished.

Reschedule
----------

```
template <typename DSL>
static inline std::unique_ptr<threading::ReactionTask> reschedule(std::unique_ptr<threading::ReactionTask>&& task)
```

The ownership of the reaction task is passed to the DSL word. The task returned will be run instead of the passed in
reaction task. If the returned task is the one passed in the task will be run normally.

If a null pointer is returned, no task is run.

When it is time to schedule the task either return it in another reschedule call or call
`task.parent.reactor.powerplant.submit(std::move(task));`. Both these will pass the ownership of the task on.

Example Case
************

TODO take one of the parts (sync maybe?) and explain how it is built using extensions
