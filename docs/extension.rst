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

This means calling `Parse<Trigger<int>, Sync, Startup>.bind()` will call `Trigger<int>.bind<DSL>()`, then a no-operation,
then will call `Startup.bind<DSL>()`, using `DSL = Parse<Trigger<int>, Sync, Startup>`.

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
`TypeBind` adds the reaction to the list of reactions to be run when a `Local` or `Inline` emit is called for the data
type. `CacheGet` gets the last value from a thread-local cache (see `ThreadSore` below) this cache is usually populated
in the last a `Local` or `Inline` emit call for the data type.

If the type you want to become a DSL extension word is not defined within your control specialise `DSLProxy<>` with the
type. Provide the template methods to the specialisation of `DSLProxy<>` as if it were the type.

Bind
----

.. codeblock:: c++
    template <typename DSL>
    static void bind(const std::shared_ptr<threading::Reaction>& reaction, /*More arguments can be declared*/)

This function is called when the reaction is bound, it should be thought of as the constructor. It is used to setup
anything that is required by the DSL word.

A common use for extensions is to setup something that will generate tasks for this reaction. This can be done by
communicating to an extension reactor via a helper type that the extension reactor triggers on.

An unbinder, if needed, should be passed to the reaction's unbinders callback list from the bind call. This is used as a
destructor.
e.g. for the `IO` word we have
.. codeblock:: c++
    reaction->unbinders.push_back([](const threading::Reaction& r) {
        r.reactor.emit<emit::Inline>(std::make_unique<operation::Unbind<IO>>(r.id));
    });

which will tell the extension reactor that this reaction no longer exists.

The extra arguments are passed in, in order, from the `on` call.

Get
---

.. codeblock:: c++
    template <typename DSL>
    static T get(threading::ReactionTask& task)

This is used to get the data for the callback. The returned value is passed to the callback.

If the return type can be dereferenced, then either the return type or the type returned by the dereference of the
return type can be used in the callback.

If data needs to be passed to a task when it is submitted to the `Powerplant` use `ThreadStore<T>`. The thread store is
a static variable that can be accessed from within the get method. Make sure to clear the `ThreadStore` after use to
ensure future invocations won't get stale data.

Precondition
------------

.. codeblock:: c++
    template <typename DSL>
    static bool precondition(threading::ReactionTask& task)

A precondition is used to test if the reaction should run. On a true return the reaction will run as normal. On a false
return the reaction will be dropped.

Postcondition
-------------

.. codeblock:: c++
    template <typename DSL>
    static void postcondition(threading::ReactionTask& task)

This will run after the callback for a reaction task has run and finished.

Reschedule
----------

.. codeblock:: c++
    template <typename DSL>
    static std::unique_ptr<threading::ReactionTask> reschedule(std::unique_ptr<threading::ReactionTask>&& task)

The ownership of the reaction task is passed to the DSL word. The task returned will be run instead of the passed in
reaction task. If the returned task is the one passed in the task will be run normally.

If a null pointer is returned, no task is run.

When it is time to schedule the task either return it in another reschedule call or call
`task.parent.reactor.powerplant.submit(std::move(task));`. Both these will pass the ownership of the task on.

Transient
---------

.. codeblock:: c++
    template <>
    struct is_transient<word::IO::Event> : std::true_type {};

When the data returned from a `get` is falsy and its type is marked transient the latest truthy data from the `get`
return is instead used. If the data is falsy and is either not marked transient or nothing truthy has yet been returned
then the reaction is cancelled.

Custom Emit Handler
*******************

.. codeblock:: c++
    template <typename DataType>
    struct EmitType {
        static void emit(PowerPlant& powerplant, ...)
    };

Emit can be extended by creating a template struct that has at least one method called `emit`. This is then called from
a Reactor with `emit<EmitType>` and the arguments will be passed through.

.. codeblock:: c++
    static void emit(PowerPlant& powerplant, std::shared_ptr<DataType> data, ...)

If the second parameter is a shared pointer to the templated type when calling emit a unique pointer will be
automatically converted to a shared pointer.

Example Case
************

Sync
----

Here, we have an ordinary C++ class. In this case we start by defining the attributes we need in a static context.
The template is used to have multiple static contexts.
.. codeblock:: c++
    template <typename SyncGroup>
    struct Sync {

        using task_ptr = std::unique_ptr<threading::ReactionTask>;

        /// Our queue which sorts tasks by priority
        static std::priority_queue<task_ptr> queue;
        /// How many tasks are currently running
        static volatile bool running;
        /// A mutex to ensure data consistency
        static std::mutex mutex;

Now we define the `reschedule` to interrupt any new tasks if we are currently running. Recall that NUClear is
multithreaded so a mutex is needed when accessing the static members.
.. codeblock:: c++
        template <typename DSL>
        static std::unique_ptr<threading::ReactionTask> reschedule(
            std::unique_ptr<threading::ReactionTask>&& task) {

            // Lock our mutex
            std::lock_guard<std::mutex> lock(mutex);

            // If we are already running then queue, otherwise return and set running
            if (running) {
                queue.push(std::move(task));
                return std::unique_ptr<threading::ReactionTask>(nullptr);
            }
            else {
                running = true;
                return std::move(task);
            }
        }

To run any queued tasks after the current one is done we define `postcondition`. When there is a task in the queue we
resubmit it to the PowerPlant to be run.
.. codeblock:: c++
        template <typename DSL>
        static void postcondition(threading::ReactionTask& task) {

            // Lock our mutex
            std::lock_guard<std::mutex> lock(mutex);

            // We are finished running
            running = false;

            // If we have another task, add it
            if (!queue.empty()) {
                std::unique_ptr<threading::ReactionTask> next_task(
                    std::move(const_cast<std::unique_ptr<threading::ReactionTask>&>(queue.top())));
                queue.pop();

                // Resubmit this task to the reaction queue
                task.parent.reactor.powerplant.submit(std::move(next_task));
            }
        }

We need to instantiate our static members outside the class definition.
.. codeblock:: c++
    };
    template <typename SyncGroup>
    std::priority_queue<typename Sync<SyncGroup>::task_ptr> Sync<SyncGroup>::queue;

    template <typename SyncGroup>
    volatile bool Sync<SyncGroup>::running = false;

    template <typename SyncGroup>
    std::mutex Sync<SyncGroup>::mutex;
