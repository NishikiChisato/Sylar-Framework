# Sylar-Framework

## Log Module

The framework in log module is as follow, and the starred class is key point.

```
LogEvent                  the information about current log event
Logger                    output a log event
  [has] LogAppender       the destination of log output

LogAppender               
  [has] LogFormatter      define a pattern string to output    

LogFormatter
  [has] FormatterItem*     the outcome of parsing pattern

LogEventWrapper*           provide stream style output and format style output
  [has] Logger
  [has] LogEvent

LoggerManager             singleton class to manage loggers             
  [has] Logger
```

Additionally, `LogAppender` and `FormatterItem` are base class, the former has two derived class and the latter has a group of derived class.

The typical usage process is as follows:

```cpp

```