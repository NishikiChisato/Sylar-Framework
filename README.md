# Sylar-Framework

- [Sylar-Framework](#sylar-framework)
  - [Build \& Testing](#build--testing)
  - [Module explanation](#module-explanation)
    - [Log Module](#log-module)
      - [Basic](#basic)
      - [TODO](#todo)
    - [Config Module](#config-module)
      - [Basic](#basic-1)
    - [Mutex Module \& \[Deprecate\]Thread Module \& Util Module](#mutex-module--deprecatethread-module--util-module)
    - [Coroutine Module](#coroutine-module)


## Build & Testing

```bash
mkdir build
cd build
cmake ..
# if you want to build for debug, run the following command
cmake .. -DCMAKE_BUILD_TYPE=Debug
make
```

## Module explanation

### Log Module

We implement log format style output and stream style output, supporting customing log formatter, setting log level and logger separation(each logger can output different content). 

#### Basic

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

#### TODO

In `LoggerMgr`, `root_` should be `std::shared_ptr<const Logger>` instead of `std::shared_ptr<Logger>`. We must force `root_` cannot be modified.

### Config Module

Use [Convection vore Configuration](https://en.wikipedia.org/wiki/Convention_over_configuration), config avriable has default value and user can change this value by loading yaml file. We implement type cast from built-in type(int/double... and STL container) to std::string, vice versa. 

#### Basic

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

Second, in the plate that you want to read config variable from yaml file, you can get this config variable.

```cpp
void LoadConfig() {
  Sylar::ConfigVar<std::string>::ptr p3 = 
        Sylar::Config::Lookup("tcp.timeout.connection", std::string(), "");
  // or you can directly use g_tcp_timeout_connection
} 
```

Additionally, if you want to define custon type, you must specialize your type, detail usage shown in [`config_test`](./tests/config_test.cc): `Config.CustonType`.

### Mutex Module & [Deprecate]Thread Module & Util Module 

Mutex module wraps and implements some sync and async class.

- Wrapper
  - `Semaphore`, which wraps `sem_wait` and `sem_post`. 
  - `Mutex`, which wraps `pthread_mutex_t`.
  - `Spinlock`, which wraps `pthread_spinlock_t`.
  - `CASLock`, which wraps `std::atomic_flag`.
  - `RWMutex`, which wraps `pthread_rwlock_t`.
- Implementation(lock acquire and release bese on RAII)
  - `ScopeLockImpl`, which similars to `std::unique_lock`, can automatically lock and unlock.
  - `ReadScopeLockImpl` and `WriteScopeLockImpl`, which similars to `std::shared_lock`, the former can automatically lock and unlock read lock, the latter can automacally lock and unlock write lock.

Thread module can not satify our demand in coroutine module and schedule module, so we deprecate it.

Util module implement some tools, including get process id, get thread id and so on.

### Coroutine Module

