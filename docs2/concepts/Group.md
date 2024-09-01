# Group

The `Group` concept in NUClear is designed to ensure that across multiple threads, the concurrency of tasks flagged with a specific group is controlled.
This is particularly useful in scenarios where certain tasks should not run concurrently to avoid race conditions or to manage shared resources efficiently.

## How it Works

When a task is flagged with a `Group`, it is assigned to a specific group identified by a group type and a maximum concurrency level.
The Group concept ensures that no more than the specified number of tasks from the same group can run concurrently.
This is achieved through a combination of task queuing and locking mechanisms.

## Example: Sync<Group>

The `Sync<Group>` is a special case of the `Group` concept where the concurrency level is set to one.
This means that across multiple threads and pools, only a single reaction will be executing at the same time for the specified group.
This is particularly useful for ensuring that tasks that modify shared state do not run concurrently, thereby avoiding race conditions.

## Usage

To use the `Group` concept, you can specify it in the DSL of a reaction. For example:

```cpp
struct MyGroup {
    static constexpr int max_concurrency = 2;
};

on<Trigger<T, ...>, Group<MyGroup>>()
```

In this example, at most two tasks from the `MyGroup` group can run concurrently.

For the `Sync<MySyncGroup>` case:

```cpp
on<Trigger<T, ...>, Sync<MySyncGroup>>()
```

In this example, only one task from the MySyncGroup group can run at any given time.

## Implementation Details

The `Group` concept is implemented using a combination of descriptors and locks.
Each group has a descriptor that specifies its name and maximum concurrency level.
When a task is flagged with a group, it is added to a queue managed by the group.
The group uses a locking mechanism to ensure that no more than the specified number of tasks are running concurrently.

### Group Descriptor

The GroupDescriptor struct holds the name and maximum concurrency level of a group:

```cpp
struct GroupDescriptor {
    GroupDescriptor(std::string name, const int& thread_count)
        : name(std::move(name)), thread_count(thread_count) {}

    std::string name;
    int thread_count;
};
```
