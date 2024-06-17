# Log Module 

## Stream Style Output

We can use `std::stringstream` to implement stream style output. Just need to output to `std::cout` in destructor.

```cpp
class StreamOutput {
public:
  ~StreamOutput() { std::cout << ss_.str(); }
  std::stringstream &GetStringStream() { return ss_; }

private:
  std::stringstream ss_;
};

#define STREAM_OUTPUT StreamOutput().GetStringStream()

int main(int argc, char *argv[], char *envp[]) {
  StreamOutput().GetStringStream() << "Hello Stream" << std::endl;
  STREAM_OUTPUT << "Hello Stream" << std::endl;
  return 0;
}
```

## Variadic Macro

A macro can be declear to accept to a variable number of arguments, as the follwoing shown:

```cpp
#define YY(l, ...) (l, __VA_ARGS__)

int main(int argc, char *argv[], char *envp[]) {
  YY(5, 10);      // -> (5, 10)
  YY(5, 10, 15);  // -> (5, 10, 15)
  return 0;
}
```

When the macro is invoked, the pary of comma will be replease to `__VA_ARGS__`. But it arise an issue that the part of comma cannot be empty. In this example, we must pass **at least two** argumeents to macro, we cannot pass only one argument to it.

To address this issue, we can add `##` in front of the `__VA_ARGS__`, which will delete the previous comma when we only pass one argument.

```cpp
#define XX(l, ...) (l, ##__VA_ARGS__)
#define YY(l, ...) (l, __VA_ARGS__)

int main(int argc, char *argv[], char *envp[]) {
  XX(5, 10);
  XX(5);
  YY(5, 10);
  // compile error in the next line
  // YY(5);
  return 0;
}
```

Additionally, we can name for comma, and this name can replease `__VA_ARGS__`, as the following shown:

```cpp
#define XX(l, ...) (l, ##__VA_ARGS__)
// `ZZ` equivalent to `XX` 
#define ZZ(l, args...) (l, ##args)
```

## Reference

- [Variadic Macros](https://gcc.gnu.org/onlinedocs/cpp/Variadic-Macros.html)