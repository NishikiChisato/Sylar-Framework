# Coroutine Module

- [Coroutine Module](#coroutine-module)
  - [Basic](#basic)


## Basic

Explanation of this main four function(excerpt from man manual):

```cpp
  int getcontext(ucontext_t *ucp);
  int setcontext(const ucontext_t *ucp);
```

In a System V-like environment, one has the two types mcontext_t and ucontext_t defined in `<ucontext.h>` and the four functions `getcontext(), setcontext(), makecontext(3), and swapcontext(3)` that allow user-level context switching between multiple threads of control within a process.

The mcontext_t type is machine-dependent and opaque.  The ucontext_t type is a structure that has at least the following fields:

```cpp
typedef struct ucontext_t {
    struct ucontext_t *uc_link;
    sigset_t          uc_sigmask;
    stack_t           uc_stack;
    mcontext_t        uc_mcontext;
    ...
} ucontext_t;
```

with `sigset_t` and `stack_t` defined in `<signal.h>`.  Here `uc_link` points to the context that will be resumed when the current context termi‐nates (in case the current context was created using `makecontext(3)`), `uc_sigmask` is the set of signals blocked in this context (see `sigprocmask(2)`), `uc_stack` is the stack used by this context (see `sigaltstack(2)`), and `uc_mcontext` is the machine-specific representation of the saved context, that includes the calling thread's machine registers.

The function `getcontext()` initializes the structure pointed at by `ucp` to the currently active context.

The function `setcontext()` restores the user context pointed at by `ucp`.  A successful call does not return.  The context should have been obtained by a call of `getcontext()`, or `makecontext(3)`, or passed as third argument to a signal handler.

If the context was obtained by a call of `getcontext()`, program execution continues as if this call just returned.

If the context was obtained by a call of `makecontext(3)`, program execution continues by a call to the function func specified as the second argument of that call to `makecontext(3)`.  When the function func returns, we continue with the `uc_link` member of the structure `ucp` specified as the first argument of that call to `makecontext(3)`.  When this member is NULL, the thread exits.

Return:

When successful, `getcontext()` returns 0 and `setcontext()` does not
return. On error, both return -1 and set errno appropriately.

---

```cpp
void makecontext(ucontext_t *ucp, void (*func)(), int argc, ...);
int swapcontext(ucontext_t *restrict oucp, const ucontext_t *restrict ucp);
```

In a System V-like environment, one has the type `ucontext_t` (defined in `<ucontext.h>` and described in `getcontext(3)`) and the four functions `getcontext(3), setcontext(3), makecontext(), and swapcontext()` that allow user-level context switching between multiple threads of control within a process.

The `makecontext()` function modifies the context pointed to by `ucp` (which was obtained from a call to `getcontext(3)`).  Before invoking `makecontext()`, the caller must allocate a new stack for this context and assign its address to `ucp->uc_stack`, and define a successor context and assign its address to `ucp->uc_link`.

When this context is later activated (using `setcontext(3) or swapcontext()`) the function `func` is called, and passed the series of integer (int) arguments that follow argc; the caller must specify the number of these arguments in `argc`.  When this function returns, the successor context is activated. If the successor context pointer is NULL, the thread exits.

The `swapcontext()` function saves the current context in the structure pointed to by `oucp`, and then activates the context pointed to by `ucp`. 

Return:

When successful, `swapcontext()` does not return.  (But we may return later, in case `oucp` is activated, in which case it looks like `swapcontext()` returns 0.)  On error, swapcontext() returns -1 and sets errno to indicate the error.

Notes:

The interpretation of `ucp->uc_stack` is just as in `sigaltstack(2)`, namely, this struct contains the start and length of a memory area to be used as the stack, regardless of the direction of growth of the stack.  Thus, it is not necessary for the user program to worry about this direction.
