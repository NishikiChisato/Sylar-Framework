# IO

- [IO](#io)
  - [The design of Scheduler](#the-design-of-scheduler)
  - [Prerequisite](#prerequisite)
    - [Blocking and Non-Blocking for File Dexcriptor](#blocking-and-non-blocking-for-file-dexcriptor)
    - [Select \& Poll \& Epoll](#select--poll--epoll)


## The design of Scheduler

Out schedule module used to schedule task, which provide `ScheduleTask()` interface for user to execute function or coroutine, and execute task, which actually execute those coroutine and function.

The `Start()` interface used to initialize and allocate scheduler object. After that point, scheduler will execute task(if it has additional sub-thread). If we call `Stop()` method, we might block in this method since we must wait for all task has done.

Sub-threads created by scheduler all perform `Run()` method, which just simply get task from task queue and execute it. If there is no task in task queue, this thread will resume `Idel()` coroutine. This coroutine will be overwriten by IO Manager.

## Prerequisite

### Blocking and Non-Blocking for File Dexcriptor

We can use `fnctl` to modify the mode of file, in this section, we only consider blocking mode(default) and non-blocking(we can use `O_NONBLOCK` flag to set it).

In blocking mode, the action of **read function** is as follows:

- If data is available, the read function will read data and return immediately with tha number of bytes it read.
- If data is no avaliable, **the read function will block the execution until the data is available or error occurs.**

In non-blocking mode, the action of **read funciton** is as follows:

- If data is available, the read function will read data and return immediately with the number of bytes it read.
- If data is no available, **the read function will not block the execution and return immediately with a return value of -1, `errno` is set to `EAGAIN` or `EWOULDBLOCK`.**

In blocking mode, the action of **write function** is as follows:

- If space is available to write to file descriptor, the write function will write data to file descriptor and return immediately with the number of bytes it write.
- If space is no available to write to file descriptor, the write function will block the execution until the space is available to write or error occurs.

In non-blocking mode, the action of **write function** is as follows:

- If space is available to write to file descriptor, the write function will write data to file descriptor and return immediately with the number of bytes it write.
- If space is no available to write to file descriptor, **the write function will not block the execution and return immediately with a value of -1, `errno` is set to `EAGAIN` or `EWOULDBLOCK`.**

### Select & Poll & Epoll

In this section, we only focus on the disadvantage amoung them, the detail usage can consult for man manual(`man select/poll/epoll`).

If we use `select`:

- The number of file descriptor will be limited to 1024
- The set of monitoring event cannot reuse, we must reset it in each turn
- If we want to check whether one file descriptor has update, we must iterate all elements

If we use `poll`:

- This interface don't have the first and the second disadvantages
- If we want to check whether one file descriptor has update, we must iterate all elements

If we use `epoll`:

- This interface don't have above three disadvantages
