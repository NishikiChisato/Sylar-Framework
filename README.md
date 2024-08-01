# Sylar-Framework

- [Sylar-Framework](#sylar-framework)
  - [Build \& Testing](#build--testing)
  - [Main Module explanation](#main-module-explanation)
    - [Log Module](#log-module)
    - [Config Module](#config-module)
    - [Coroutine \& Epoll \& Timewheel Module](#coroutine--epoll--timewheel-module)


## Build & Testing

```bash
mkdir build
cd build
cmake ..
# if you want to build for debug, run the following command
cmake .. -DCMAKE_BUILD_TYPE=Debug
make
```

You can run `make test` to execute all unit tests. Additionally, I have built an echo server using the socket module, with the source code available in [`socket_client.cc`](./tests/socket_client.cc) and [`socket_server.cc`](./tests/socket_server.cc). You will need to open two terminal windows: one for the client and one for server.

I have also built an HTTP server, with the source code available in [`httpserver.cc`](./tests/httpserver.cc). You can configure it to use coroutine with multiple threads or just multiple threads. The server achieves a QPS of approximately `15k` in my computer.

## Main Module explanation

### Log Module

We implement log format style output and stream style output, supporting customing log formatter, setting log level and logger separation(each logger can output different content). 

```
LogEvent                  the information about current log event
Logger                    output a log event
  [has] LogAppender       the destination of log output

LogAppender               
  [has] LogFormatter      define a pattern string and output    

LogFormatter
  [has] FormatterItem[Base class]     the outcome of parsing pattern
    - MessageFormatterItem[Derived class]           output message
    - LoggerNameFormatterItem[Derived class]        output logger name 
    - FileNameFormatterItem[Derived class]          output file name
    - LevelNameFormatterItem[Derived class]         output log level
    - LineNameFormatterItem[Derived class]          output line
    - EpleaseNameFormatterItem[Derived class]       output eplease from process start
    - ThreadIdNameFormatterItem[Derived class]      output thread id
    - CoroutineIdNameFormatterItem[Derived class]   output coroutine id
    - TimestampNameFormatterItem[Derived class]     output time of specificed format

LogEventWrapper           provide stream style output and format style output
  [has] Logger
  [has] LogEvent

LoggerManager             singleton class to manage loggers             
  [has] Logger

```

Additionally, `LogAppender` and `FormatterItem` are base class, the former has two derived class and the latter has a group of derived class.

The typical usage process is writen in [`log_test.cc`](./tests/log_test.cc): `LogModule.TypicalUsage1`, `LogModule.TypicalUsage2`, `LogModule.TypicalUsage3`.

### Config Module

Use [Convection vore Configuration](https://en.wikipedia.org/wiki/Convention_over_configuration), config avriable has default value and user can change this value by loading yaml file. We implement type cast from built-in type(int/double... and STL container) to std::string, vice versa. 

```
ConfigVarBase[Base class]                   the abstract class of the common variable in config vairable

ConfigVar[Derived class & Template class]   due to every variable has different type, this class should be template
  Template arguement: 
    - T:        config variable type(e.g. string/vector/list/set/unordered_set/map/unordered_map... or custom type)
    - ToStr:    default is LexicalCast, used to cast variable type to std::string
    - FromStr:  default is LexicalCast, used to cast std::string to variable type

LexicalCast<F, T>                 used to cast from F to T, default type can be process by boost::lexical_cast
  - LexicalCast<T, std::string>   the partical specialiation for LexicalCast
  - LexicalCast<std::string, T>   

Config                                      used to manage config variable
```

The typical usage process is as follows:

First, you must define config variable in **global namespace**.

```cpp
Sylar::ConfigVar<std::string>::ptr g_tcp_timeout_connection = 
        Sylar::Config::Lookup("tcp.timeout.connection", std::string(), "");
```

Second, in the place that you want to read config variable from yaml file, you can get this config variable.

```cpp
void LoadConfig() {
  Sylar::ConfigVar<std::string>::ptr p3 = 
        Sylar::Config::Lookup("tcp.timeout.connection", std::string(), "");
  // or you can directly use g_tcp_timeout_connection
} 
```

Additionally, if you want to define custon type, you must specialize your type, detail usage shown in [`config_test`](./tests/config_test.cc): `Config.CustonType`.

### Coroutine & Epoll & Timewheel Module

The implementation of coroutine module is inspired by [lobco](https://github.com/Tencent/libco). It supports both shared stack and independent stack operations, with a default stack size of `64 * 1024` bytes. Coroutines can invoke other coroutines within their execution function with no limitation on the depth of invocation.

Each thread manages its coroutines using a `Schedule` module, which has a variable declared as `static thread_local`. This allows each thread to launch multiple coroutines(executing asynchronously) but **coroutine cannot be dispatched across threads.** 

The epoll module, which works alongside the coroutine module, mainly implements the core logic of event-driven. It provides a static method called `EventLoop`, which repeatedly performs the following steps:

- Calls `epoll_wait` to monitor READ and WRITE events on file descriptors that have beed registered with `epoll_ctl`.
- If one or more file descriptors are ready for reading or writing, the corresponding callback function or coroutine will execute.
- Checks for time events and executes the corresponding callback function or coroutine if any exist.

The Timewheel module must be used in conjunction with the epoll module. It allows for customize timewheel granularity, with a default value of `1ms`, and a total duration(all ticks), which a default value of `1s`.

The usage of coroutine module and epoll module are shown in [`coroutine_test.cc`](./tests/coroutine_test.cc) and [`epoll_test.cc`](./tests/epoll_test.cc).

The structure of coroutine is shown as follows:

```
MemoryAlloc[auxiliary class]    allocate a space managed by smart pointer by calling malloc       
StackMem                        the independent executing stack of coroutine
SharedMem                       the shared executing stack of coroutine
CoroutineAttr                   the attribute of coroutine
Schedule                        the manager of coroutine
Coroutine                       the definition of coroutine
```