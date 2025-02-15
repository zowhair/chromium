# Reactive Android Java programming

[TOC]

## Introduction

**Ever notice how Android methods often come in pairs?** For every `onCreate()`,
there is an `onDestroy()`, for every `onStart()`, there is an `onStop()`. The
Android SDK commonly asks clients to register callbacks or extend base classes
that override pairs of methods that correspond to *reversible changes in state*.

*State* is often expressed in Java code as *mutable variables*. A state changes
when you assign a new value to the variable. If a variable can be one value at
some points and another value at other points, that means there are two states
that the variable can have. Everything that interacts with that variable needs
to work correctly for each state the variable can be in. For example, if an
instance variable is `null` in a class's constructor, and set to a value by some
method in that class, then *every* method that tries to call a method on that
variable needs to check whether the value of the variable is null before
handling it, because there is no guarantee which state the variable is in. You
will see a lot of code that looks like this when using this pattern for
representing state:

```java
if (mFoo != null) {
    mFoo.doSomething();
}
```

Additionally, mutator methods may need to check the state at runtime. For
example, lazy initialization often looks like this:

```java
if (mFoo == null) {
    mFoo = new Foo(...);
}
```

This is not bad in and of itself, if the states are well-defined and it's easy
to reason about the set of possible states by looking at the code. However, it
very, very quickly becomes difficult to reason about states when there are any
of the following:

*   **Multiple methods that can mutate state**. For example, a hypothetical
    `Connection` class that reads and writes data over a socket might disconnect
    on a socket error from any `read()` or `write()` call. That means that
    before any `read()` or `write()` call, the state must be checked. (Real Java
    objects will often use `Exception`s to short-circuit code blocks that enter
    an exceptional state).
*   **Methods that throw a runtime error** or have **undefined behavior when in
    a certain state**. For example, a class with an `initialize()` method may
    have methods that should only be called after `initialize()`, but the
    compiler will not be able to check whether `initialize()` has been called.
    This includes every method that has an `assert` statement on a mutable
    instance variable.
*   Multiple **states that interact with each other**. The number of states that
    independently-mutable variables can take is the *product* of the number of
    states of each of the variables. Often, variables are not strictly
    independent (e.g. the only method that mutates a certain variable also
    mutates another), so some states might be unreachable. However, it's **not
    possible for the compiler to tell you which states are reachable** when
    you're using mutable instance variables, so you have to figure that out
    yourself! This makes it hard to exhaustively come up with unittest cases.

## Motivating Example

Consider this seemingly-simple task: you have two variables, `mA` and `mB`, each
of which could be either `null` or some real value of types `A` and `B`,
respectively. Furthermore, you want to initialize a new variable, `mC` of type
`C`, when the values of `mA` and `mB` are non-null, perhaps because the `C`
constructor takes an `A` and a `B`. Finally, if `mA` or `mB` becomes null again
after creating `mC`, reset `mC` to `null`. Also, you need to invoke a `close()`
method on `mC` whenever `mC` is reset. And if `mA` or `mB` changes while `mC`
exists, you need to call `mC.close()` and re-create `mC` with the new `mA` and
`mB`.

```java
class MyClass {
    private A mA = null;
    private B mB = null;
    private C mC = null;

    public void setA(A a) {
        mA = a;
        recalculateC();
    }

    public void setB(B b) {
        mB = b;
        recalculateC();
    }

    private void recalculateC() {
        // This method is always called when A or B changes, so if C exists, it
        // must first be reset.
        if (mC != null) {
            mC.close();
            mC = null;
        }
        if (mA != null && mB != null) {
            mC = new C(mA, mB);
        }
    }
}
```

This may be fine on its own. But chances are, you will want to do something with
`mC` outside these methods. Every read will have to null-check, there's an
**undocumented but critical requirement** that every write to `mA` and `mB` is
done through `setA()` and `setB()`, and that `recalculateC()` is only called
when `mA` or `mB` is mutating, or else it will implicitly close.

These undocumented dependencies can only be protected against regression by
testing. The compiler will not tell you if you made a mistake, so there must be
unittests covering every possible state change. And in this case, we have two
variables, each with two states that each have two possible state transitions,
so **8 test cases are needed to cover everything**. And this is the **simplest**
case of composing **two** independent nullable mutable variables.

Meanwhile, if you use the `Observable`s framework:

```java

class MyClass {
    private final Controller<A> mA = new Controller<>();
    private final Controller<B> mb = new Controller<>();

    {
        mA.and(mB).watch(ScopeFactories.both(C::new));
    }

    public void setA(A a) {
        mA.set(a);
    }

    public void setB(B b) {
        mB.set(b);
    }
}
```

In the instance initializer, we set up a simple state machine with two
`Controller`s, which correspond to the mutable instance variables from the
previous example, and an event that observes the state of the `Controller`s and
invokes some logic on certain state changes.

The `and()` call composes the `mA` and `mB`, returning a new `Observable` that
is only activated when *both* sources are activated, and then deactivated if
*either* source is deactivated. `mA` and `mB` are activated or deactivated by
`set()` and `reset()` calls, respectively. The `set()` method deactivates the
state if the argument is `null` (the `reset()` method can also be used to
deactivate the state).

The `watch()` call makes it so that when the composed `Observable` formed by
`mA.and(mB)` is activated, a new `C` object will be created. When deactivated,
that `C` object's `close()` method will be called.

This really does cover all the cases we need. If multiple `set*()` calls are
made, an implicit `reset()` call will be made to the relevant `Controller` and
the `C` object associated with the first scope will be `close()`d.

What's better about this? First, notice that we **don't need to have any mutable
variables**. Both `Controller` objects are `final`, and are never `null`. We
don't at any point need to know what state the object is in inside any method
implementations; the `Controller`s and the pipeline set up by the `and()` and
`watch()` calls handle that for you.

Second, notice how the concerns of mutating and reacting to state are **cleanly
separated**. The mutator methods `setA()` and `setB()` are concerned only with
their respective `Controller`s, and the lifetime of the `C` object is managed in
one place in the instance initializer.

Finally, the relationship between the `A`, `B`, and `C` objects is
**self-documenting**. In the first approach with mutable variables, to
understand that the lifetime of `C` is associated with the intersection of the
lifetimes of `A` and `B`, one has to examine both setters and trace through
`recalculateC()` from the perspective of both of its call sites. In the second
approach, using `Controller`s, the relationship between `A`, `B`, and `C` is
expressed holistically in one line.

## Observables and Scopes

Think of an `Observable` as an encapsulation of a nullable, mutable variable,
with one very important feature: the ability to register observers that will be
notified when changes take place. The `Observable` API alone does not expose any
state mutators, but it provides ways to register events that will be invoked
when state changes.

An `Observable` has two states: *activated* and *deactivated*. There are two
transitions between states: *activation* occurs when transitioning from
*deactivated* to *activated*, and *deactivation* occurs when transitioning from
*activated* to *deactivated*.

An activation is similar to setting a nullable, mutable variable that was `null`
to a non-`null` value. A deactivation, likewise, is similar to setting a
nullable, mutable variable that had a non-`null` value to `null`. The non-`null`
value that an activation is associated with is called **activation data**.

To register events that should be invoked on these state transitions, we utilize
**scopes**. When an `Observable` is activated, an observing scope is created,
and when the `Observable` is deactivated, that scope is `close()`d.

The only requirement of a scope is that it **implements
`java.lang.AutoCloseable`**. The side-effects of activation are in the scope's
constructor, and the side-effects of deactivation are in the scope's `close()`
method. This pairing of constructors with a `close()` is inspired by
[RAII](https://en.wikipedia.org/wiki/Resource_acquisition_is_initialization) in
C++, and allows the activation data injected into the scope to be expressed as
an **immutable** variable.

### Registering scopes with Observables

To register scopes to track the state of an `Observable`, we call `watch()` on
the `Observable`. The `watch()` method takes a single argument, a
`ScopeFactory`, which has a `create()` method that returns an `AutoCloseable`.
The `ScopeFactory`'s `create()` method is called when the `Observable`
activates, and the resulting `AutoCloseable`'s `close()` method is called when
the `Observable` deactivates.

Lambda syntax can be used to easily construct `ScopeFactory` objects without
much boilerplate. For instance, if we want to simply log the transitions of an
`Observable`, we might do it like this:

```java
void logStateTransitions(Observable<?> observable) {
    observable.watch(() -> {
        Log.d(TAG, "activated");
        return () -> Log.d(TAG, "deactivated");
    });
};
```

This is equivalent to the following, much more verbose version:

```java
void logStateTransitions(Observable<?> observable) {
    observable.watch(new VoidScopeFactory() {
        @Override
        public AutoCloseable create() {
            Log.d(TAG, "activated");
            return new AutoCloseable() {
                @Override
                public void close() {
                    Log.d(TAG, "deactivated");
                }
            };
        }
    });
}
```

As you can see, the version that uses lambdas is much more readable, as long as
you understand what a `ScopeFactory` is.

Either way, when `logStateTransitions()` is called on an `Observable`,
`"activated"` will be printed to the log when that `Observable` is activated,
and `"deactivated"` will be printed to the log when that `Observable` is
deactivated.

There are actually two types of `ScopeFactory`. `VoidScopeFactory`, as seen
above, does not care about the activation data, and so its `create()` method
takes no arguments. A normal `ScopeFactory` actually takes a single parameter to
its `create()` method, so that the behavior of the scope can depend on what data
the `Observable` is activated with.

Say we have an `Observable<String>` and we want to log the data it is activated
with:

```java
void logStateTransitionsWithData(Observable<String> observable) {
    observable.watch((String s) -> {
        Log.d(TAG, "activated with data: " + s);
        return () -> Log.d(TAG, "deactivated");
    });
}
```

### Mutating state with Controllers

The `Observable` interface does not provide any way to directly change the state
of the `Observable`. However, the `Controller` object, which implements
`Observable`, exposes two methods to do just that: `set()` and `reset()`.

Remember that `Controller`s are basically nullable, mutable variables that let
you register callbacks, through the `Observable` interface, that are run when
the variable changes.

With this in mind, the `set()` method on `Controller` is like setting a mutable
variable to a value. The `reset()` method is like setting the mutable variable
to `null`. **Any nullable mutable variable can be replaced by a `Controller`**.

Here are some guarantees that `Controller`s provide:

*   Start out in the *deactivated* state.
*   If the parameter of `set()` is non-`null`, it enters the *activated* state.
*   If the parameter of `set()` is `null`, it enters the *deactivated* state.
*   If in the activated state, `reset()` and `set(null)` enter the *deactivated*
    state.
*   If already in the deactivated state, `reset()` and `set(null)` do nothing.
*   If in the activated state, `set()` implicitly deactivates and reactivates
    with the new data.

As corollaries, any registered `ScopeFactory` objects will:

*   have their `create()` methods invoked *exactly once* for each non-`null`
    `set()` call
*   have their resulting `AutoCloseable` scopes `close()`d *exactly once* when
    `reset()` or `set()` to `null`
*   always clean up scopes from previous activations when new activations occur

This means this:

```java
void helloGoodbye(Controller<String> message) {
    message.set("hello");
    message.set("goodbye");
    message.set(null);
}
```

has the same behavior as this:

```java
void helloGoodbye(Controller<String> message) {
    message.set("hello");
    message.reset();
    message.set("goodbye");
    message.reset();
}
```

Essentially, a `Controller` adapts two states with two possible actions each:

*   `deactivated`: `set`, `reset`
*   `activated`: `set`, `reset`

into a well-defined state machine with two states and two transitions:

*   `deactivated`: `set`
*   `activated`: `reset`

... by dropping redundant `reset()` calls and inserting implicit `reset()` calls
between contiguous `set()` calls. This cuts the number of state transitions you
need to worry about in half!

Since `Controller`s implement `Observable`, you can register `ScopeFactory`
objects with `watch()` the same way as in the previous section, or inject a
`Controller` into any method that takes an `Observable` of the same parametric
type.

### Observables without data

The state of a `Controller<T>` is isomorphic to that of a nullable `T` variable
for all types `T`. But there are many cases where what we really want is a
representation of a boolean state: on or off, active or inactive, and don't need
any activation data.

For these cases, the `org.chromium.chromecast.base.Unit` class is used to denote
the fact that there is no data associated with the controller. The `Unit` type
is inspired by the type of the same name in many functional programming
languages, and represents a type with only one possible instance (aka
`Singleton`).

To make a controller without data, you can therefore use `Controller<Unit>`.
Since `Unit` means "no data," and there's only one way to get a `Unit` instance
(through the `Unit.unit()` method), this maps correctly to the concept of a
mutable boolean value.

Example:

```java
{
    Controller<Unit> onOrOff = new Controller<>();
    onOrOff.watch(() -> {
        Log.d(TAG, "on");
        return () -> Log.d(TAG, "off");
    });
    onOrOff.set(Unit.unit()); // Turns on.
    onOrOff.reset(); // Turns off.
}
```

### Composing Observables with and()

In the motivating example, we wanted to invoke a callback once *two* independent
states have been activated.

If there are two independent states, there are four possible *combinations* of
states. Given two independent states, `A` and `B`, there are four states in the
time-independent product state space:

*   `neither`
*   `just A`
*   `just B`
*   `A and B`

...and eight transitions:

*   `neither` **<->** `just A`
*   `neither` **<->** `just B`
*   `just A` **<->** `A and B`
*   `just B` **<->** `A and B`

The `Observable` interface gives us a convenient way to get the `(A and B)`
state with a simple call:

```java
public void logWhenBoth(Observable<A> observableA, Observable<B> observableB) {
    observableA.and(observableB).watch(...);
}
```

The `and()` method takes the calling `Observable` and the given other
`Observable` and returns a new `Observable` that is only activated when *both*
input `Observable`s are activated.

One way to think about it is that the `and()` call collapses the three states
`(neither)`, `(just A)`, and `(just B)` into one *deactivated* state, and
treats the state `both` as *activated*. For observers of the `and()`-composition
of states, one needs only worry about the two states, *deactivated* and
*activated*, same as with any other observer.

So how do we get the data in the `watch()` call? Let's say we want to log when
both `Observable`s are activated:

```java
public void logWhenBoth(Observable<A> observableA, Observable<B> observableB) {
    observableA.and(observableB).watch((Both<A, B> data) -> {
        A a = data.first;
        B b = data.second;
        Log.d(TAG, "both activated: a=" + a + ", b=" + b);
        return () -> Log.d(TAG, "deactivated");
    });
}
```

The type of the activation data for an `and()`-composed `Observable` is `Both`.
The `Both` type has two generic parameters, and `first` and `second` public
fields to access the data it encapsulates. It is essentially a trick to box
multiple values into a single object, so we only ever need `ScopeFactory`
interfaces that take a single argument.

Since the `and()` method returns an `Observable`, the result can itself call
`and()`, whose result can itself call `and()`, ad infinitum:

```java
    observableA.and(observableB).and(observableC).and(observableD)...
```

But beware, as the associated type of the `Observable` gets uglier and uglier:

```java
    a.and(b).and(c).and(d).watch((Both<Both<Both<A, B>, C>, D> data) -> {
        A aData = data.first.first.first;
        B bData = data.first.first.second;
        C cData = data.first.second;
        D dData = data.second;
        Log.d(TAG, "a=%s, b=%s, c=%s, d=%s", aData, bData, cData, dData);
        return () -> Log.d(TAG, "exit");
    });
```

One one hand, it's kind of neat that you can do that at all. But it does come at
a cost to readability. The compiler can catch you if you mess up the
`first.first.second` chains if the types are different, but it is regrettable
that this much work is required to read the compound data. Some methods for
alleviating this are described below.

### Imposing order dependency

Every composition of states up to this point has been *time-independent*. For
example, `stateA.and(stateB)` doesn't care if `stateA` or `stateB` was activated
first, so it can be activated by either activating `stateA` and then `stateB`,
or by activating `stateB` and then `stateA`.

This means the state `(A and B)`, extracted by the `and()` method on
`Observable`, is too ambiguous for knowing the order of activation. We must
*partition* the state `(A and B)` into `(A and then B)` and `(B and then A)`.
The time-dependent state space for two `Observable`s looks like this, with five
states:

*   `neither`
*   `just A`
*   `just B`
*   `A and then B`
*   `B and then A`

...and ten transitions:

*   `neither` **<->** `just A`
*   `neither` **<->** `just B`
*   `just A` **<-->** `A and then B`
*   `just B` **<-->** `B and then A`
*   `A and then B` **-->** `just B`
*   `B and then A` **-->** `just A`

Calling `stateA.andThen(stateB)` returns an `Observable` representing the
`(A and then B)` state from above. The resulting `Observable` will only activate
on the transition between `(just A)` and `(A and then B)`, and will not activate
on the transition between `(just B)` and `(B and then A)`.

### Increase readability for ScopeFactories with wrapper methods

The `ScopeFactories` class contains several helper methods to increase the
fluency and readability of common cases that `ScopeFactory` objects might be
used for.

#### Use onEnter() and onExit() to observe only one kind of transition

Every `ScopeFactory` returns an `AutoCloseable`, but sometimes clients do not
care about when the state deactivates, only when it activates. It's possible to
create a `ScopeFactory` with lambda syntax to do the job like this:

```java
{
    observable.watch((String data) -> {
        Log.d(TAG, "activated: data=" + data);
        return () -> {};
    });
}
```

The `return () -> {};` statement in the lambda corresponds to having no
side-effects to handle the destructor, but this is not very readable.

To make intentions clearer, the `onEnter()` method can wrap any `Runnable` or
`Consumer` of the activation data's type:

```java
{
    // Without data.
    observable.watch(ScopeFactories.onEnter(() -> Log.d(TAG, "activated")));
    // With data.
    observable.watch(ScopeFactories.onEnter((String data) -> {
        Log.d(TAG, "activated: data=" + data);
    }));
}
```

Likewise, `onExit()` is used the same way to transform any `Runnable` or
`Consumer` of the activation data's type into a `ScopeFactory` that only has
side effects when the state is deactivated.

#### Deconstructing Both objects

When you use the `and()` method on `Observable` to create an `Observable<Both>`,
recall that the `ScopeFactory` passed to `watch()` must look like this:

```java
{
    observableA.and(observableB).watch((Both<A, B> data) -> {
        A a = data.first;
        B b = data.second;
        Log.d(TAG, "on enter: a = " + a + "; b = " + b);
        return () -> Log.d(TAG, "on exit: a = " + a + "; b = " + b);
    });
}
```

`ScopeFactories` provides a helper method to turn any function that takes two
arguments and returns an `AutoCloseable` into a `ScopeFactory<Both>`, which
deconstructs the `Both` object for you and passes the constituent parts into
the function.

Using `ScopeFactories.both()`, we can rewrite the above like this:

```java
{
    observableA.and(observableB).watch(ScopeFactories.both((A a, B b) -> {
        Log.d(TAG, "on enter: a = " + a + "; b = " + b);
        return () -> Log.d(TAG, "on exit: a = " + a + "; b = " + b);
    }));
}
```

The `onEnter()` and `onExit()` helpers can also take a consumer of two arguments
and return an appropriate `ScopeFactory<Both>`:

```java
{
    observableA.and(observableB)
            .watch(ScopeFactories.onEnter((A a, B b) -> {
                Log.d(TAG, "on enter: a = " + a + "; b = " + b);
            }))
            .watch(ScopeFactories.onExit((A a, B b) -> {
                Log.d(TAG, "on exit: a = " + a + "; b = " + b);
            }));
}
```

## Data flow

There are numerous instances where one may want to take the activation data of
some `Observable` and use it to set the state of a `Controller`, and reset that
`Controller` when the `Observable` is deactivated. A shortcut to doing this
without having to instantiate any `Controller` is provided with the
`transform()` method in the `Observable` interface.

For example, we might have an `Activity` that overrides `onNewIntent()`, and
extracts some data from the `Intent` it receives. We might want to register
observers on the extracted data rather than the `Intent` itself, as some work
needs to be done to unparcel the data we care about from the `Intent`.

```java
public class MyActivity extends Activity {
    private final Controller<Intent> mIntentState = new Controller<>();

    {
        Observable<Uri> uriState = mIntentState.transform(Intent::getData);
        Observable<String> instanceIdState = uriState.transform(Uri::getPath);
        ...
    }

    public void onCreate() {
        super.onCreate();
        mIntentState.set(getIntent());
    }

    public void onNewIntent(Intent intent) {
        super.onNewIntent(intent);
        mIntentState.set(intent);
    }
}
```

The `transform()` method takes any function on the `Observable`'s activation
data and creates a new `Observable` of the result of that function applied to
the original `Observable`'s activation data. So the activation lifetime of
`uriState` and `instanceIdState` are the same as `mIntentState` in this
example.

The instance initializer can then call `watch()` on `uriState` or
`instanceIdState` to register callbacks for when we get a new URI or instance
ID, and the process of extracting the URI from the `Intent` and the instance ID
from the `Uri` is delegated to methods with no side-effects.

### Handling null

If a function provided to a `transform()` method returns `null`, then the
resulting `Observable` will be put in a deactivated state, even if the source
`Observable` is activated. This can be used to **filter** invalid data from
`Observable`s in the pipeline:

```java
{
    mIntentState.transform(Intent::getExtras)
            .transform((Bundle bundle) -> bundle.getString(INTENT_EXTRA_FOO))
            .watch((String foo) -> ...);
}
```

The `bundle.getString()` call might return `null` if the source `Intent` does
not have the correct extra data field set. When this happens, the resulting
`Observable` simply does not activate, so the `ScopeFactory` registered in the
`watch()` call does not need to worry that `foo` might be `null`.

## Tips and best practices

### Construct the pipeline before modifying it

Consider this code:

```java
    Controller<String> c = new Controller<>();
    c.set("hi");
    c.reset();
    c.watch(ScopeFactories.onEnter(s -> Log.d(TAG, s)));
```

Will the callback registered in the `watch()` call get fired? It turns out that
it will not, since `c` is deactivated when `watch()` is made. But if the
`watch()` call is made before the `set()` call, then the callback is fired.

Sometimes this is what you want, but it's best to avoid any ambiguity like this.
Generally, `Observable` methods like `watch()` should be called before any
`Controller` methods. A couple of things that one can do to help with this:

*   Instantiate `Controller` objects in field initializers, not the constructor.
*   Set up the pipeline (`watch()`, `and()`, `transform()`, etc.) in an instance
    initializer. This is run before anything else when creating an instance,
    including the constructor, and is the same regardless of which constructor
    is being used. This also removes the potential of accidentally depending on
    constructor parameters or mutable instance variables in the pipeline, which
    can be dangerous compared to adapting them to `Observable`s.
*   In the instance initializer, `Observable`s composed from other `Observable`s
    can usually be local variables rather than instance variables. This prevents
    code outside the initializer from `watch()`ing these `Observable`s after the
    instance has been initialized.
*   Do not call `Controller` mutator methods (`set()` or `reset()`) inside the
    instance initializer. They may be called in the constructor or any instance
    methods.
*   Alternatively, the concerns of creating the pipeline and adapting function
    calls to state changes of `Controller`s can be separated by using a factory
    function.

### Manipulating state inside observers

What happens here?

```java
    Controller<Object> c = new Controller<>();
    c.watch(() -> {
        Log.d(TAG, "enter");
        c.reset();
        return () -> Log.d(TAG, "exit");
    });
    c.set("ding");
```

Here, we `reset()` the same `Controller` in an activation observer for that very
`Controller`!

This is in fact safe, though there should be few places you need to do something
like this. Currently, `Controller`s notify all observers synchronously on the
thread that `set()` or `reset()` was called in (so they are not thread safe),
but if an observer calls `set()` or `reset()` again while observers are still
being notified, the `set()` or `reset()` call gets queued and handled only after
all observers have been notified. This allows a deterministic and unastonishing
order of execution for the above example: the log will show "enter", followed
immediately by "exit".

Note that if you `set()` a controller with a value that is never `null` inside
an activation handler, **you will get an infinite loop**.

```java
    Controller<Integer> c = new Controller<>();
    c.watch(ScopeFactories.onEnter(x -> c.set(x + 1))); // Danger!
    c.set(0); // Infinite loop!
```

Whenever the `Controller` is set with a value, the observing scope immediately
sets it with a new value, recurring infinitely.

It's possible to still be safe if you can guarantee that `set()` isn't called or
`set(null)` is called in some base case for all recursive stacks of activation
handlers, but if you do that, it's *your* job to solve the halting problem.

It is good practice to avoid calling `set()` or `reset()` on `Controller`s
inside `ScopeFactory` event handlers altogether, but there are many safe ways
that are useful. `and()`-composed `Observable`s, for example, use `Controller`s
under the hood to know when to notify.

## When to use Observables

`Observable`s and `Controller`s are intended to succinctly adapt common Android
SDK method pairs, whether they're callbacks for entering and exiting a state, or
mutators to perform state changes, into a common pattern that better separates
concerns and is composable.

### Replace mutable, nullable variables

Every mutable, nullable variable is a variable that you constantly have to
null-check before using. A `Controller` can be used to refactor these variables
into a `final` `Controller`.

The important insight is that you tend to only read a variable when state
changes, either after the variable itself is known to change, or when some other
state changes.

First, let's consider an Activity that registers a `BroadcastReceiver` in
`onStart()` and unregisters the `BroadcastReceiver` in `onStop()`.

We will ignore for now that Android tries to guarantee that pathological call
sequences like multiple `onStart()`s in a row or an `onStop()` before the first
`onStart()` will not occur, because these guarantees are not known to the Java
compiler and similar guarantees can't be relied on for all events.

```java
class MyActivity extends Activity {
    private BroadcastReceiver mReceiver = null;

    @Override
    public void onStart() {
        super.onStart();
        if (mReceiver != null)
            unregisterReceiver(mReceiver);
        mReceiver = new BroadcastReceiver(...);
        registerReceiver(mReceiver);
    }

    @Override
    public void onStop() {
        if (mReceiver != null)
            unregisterReceiver(mReceiver);
        mReceiver = null;
        super.onStop();
    }
}
```

Without the assumption that Android will call `onStart()` and `onStop()` in
reasonable orders, we need to check the state of the `mReceiver` variable each
time before it is used. And making that assumption is prone to backfiring, as
it's a recipe for `NullPointerException`s, `IllegalStateException`s, and other
horrors in general practice.

Here's the refactored version that uses a `Controller`:

```java
class MyActivity extends Activity {
    private final Controller<Unit> mStartedState = new Controller<>();

    {
        mStartedState.watch(() -> {
            final BroadcastReceiver receiver = new BroadcastReceiver(...);
            registerReceiver(receiver);
            return () -> unregisterReceiver(receiver);
        });
    }

    @Override
    public void onStart() {
        super.onStart();
        mStartedState.set(Unit.unit());
    }

    @Override
    public void onStop() {
        mStartedState.reset();
        super.onStop();
    }
}
```

The refactored version better separates concerns. `BroadcastReceiver`
registration and unregistration is handled in a small area of the code, rather
than spread throughout the class, and the `BroadcastReceiver` doesn't need to be
stored in a mutable variable. *No code outside the scope in which the
`BroadcastReceiver` object is relevant can touch it*, and the `onStart()` and
`onStop()` methods have no logic except toggling the `Controller` that
represents whether the `Activity` is started. Best of all, there are no `null`
checks, and no need for any.

### Asynchronous initialization

Some methods run asynchronously and take a callback that is run when the work is
complete. We can `set()` `Controller`s in such callbacks to adapt this pattern
to `Observable`s, which can be used to create asynchronous initialization
pipelines.

This example shows how one can link up the outputs of multiple asynchronous
functions that use the callback-passing style using `Controller`s, and
encapsulating the complicated setup into a single function that returns an
`Observable`.

```java
public class AsyncExample {
    private static final String TAG = "AsyncExample";

    // Adapts the callback-style asynchronous Baz function to an Observable.
    // Shows how a
    public static Observable<Baz> createBazDefault() {
        // Begin constructing a Foo.
        Controller<Foo> fooState = new Controller<>();
        // Set the fooState controller when created, reset on errors.
        Foo.createAsync(fooState::set, fooState::reset);
        // Bar requires Foo to initialize.
        Controller<Bar> barState = new Controller<>();
        fooState.watch((Foo foo) -> {
            Bar.createAsync(foo, barState::set);
            // If fooState is reset, then barState is also reset.
            return barState::reset;
        });
        // Baz requires Foo and Bar to initialize.
        Controller<Baz> bazState = new Controller<>();
        fooState.and(barState).watch(ScopeFactories.both((Foo foo, Bar bar) -> {
            Baz.createAsync(foo, bar, bazState::set);
            // If fooState or barState is reset, then bazState is also reset.
            return bazState::reset;
        }));
        return bazState;
    }

    public static void demo() {
        Observable<Baz> bazState = createBazDefault();
        bazState.watch(ScopeFactories.onEnter((Baz baz) -> {
            // This runs when the full initialization pipeline is complete.
            Log.d(TAG, "Baz created!");
        }));
    }

    public static class Foo {
        static void createAsync(Consumer<Foo> callback, Runnable onError) {...}
    }

    public static class Bar {
        static void createAsync(Foo foo, Consumer<Bar> callback) {...}
    }

    public static class Baz {
        static void createAsync(Foo foo, Bar bar, Consumer<Baz> callback) {...}
    }
}
```

This way, `Observable`s can be used similarly to `Promise`s, where a callback
handling the underlying value can be registered before the underlying value is
available. But unlike `Promise`s, `Observable`s provide a way to also handle
teardowns, and to transitively tear down everything down stream when something
is torn down.

### Protocols

When two components need to talk to each other over some unsafe serialization
protocol, you can use a neat trick of using `Controller`s for output channels
and `Observable`s for input channels, and using `andThen()` calls to ensure that
incoming messages arrive in a certain order.

As an illustrative example, let's consider the TCP three-way handshake. In this
simple protocol, the client sends a `SYN` message, the server replies with a
`SYN-ACK` message, and then the client replies with an `ACK` message.

When implementing this protocol, one needs to consider that the medium may be
untrustworthy, and that the other end of the protocol may not conform to your
expectations. How does a server handle malicious or outdated clients? How does
a client handle malicious or outdated servers? What if the server sends a
`SYN-ACK` before the client sent a `SYN`, or what if the client sends an `ACK`
without first sending a `SYN` or the server neglects to send a `SYN-ACK`? Even
with such a simple protocol as a three-way handshake, there are a lot of edge
cases!

This is even further complicated by considerations of blocking and non-blocking
sending and receiving of messages.

Fortunately, we can express this beautifully with `Observable`s if we represent
each message type as an `Observable`, with `Controller`s for outputs and
`Observable`s for inputs:

```java
// Client: sends SYN, receives SYN-ACK, sends ACK, then done.
public static Observable<Unti> tcpHandshakeClient(
        Controller<Unit> syn, Observable<Unit> synAck, Controller<Unit> ack) {
    Controller<Unit> done = new Controller<>();
    syn.andThen(synAck)
            .watch(() -> ack.set(Unit.unit()))
            .watch(() -> done.set(Unit.unit()));
    syn.set(Unit.unit());
    return done;
}

// Server: receives SYN, sends SYN-ACK, receives ACK, then done.
public static Observable<Unit> tcpHandshakeServer(
        Observable<Unit> syn, Controller<Unit> synAck, Observable<Unit> ack) {
    Controller<Unit> done = new Controller<>();
    syn.watch(synAck::set).andThen(ack).watch(() -> done.set(Unit.unit()));
    return done;
}

public static void simulateHandshake() {
    Controller<Unit> syn = new Controller<>();
    Controller<Unit> synAck = new Controller<>();
    Controller<Unit> ack = new Controller<>();
    Observable<Unit> client = tcpHandshakeClient(syn, synAck, ack);
    Observable<Unit> server = tcpHandshakeServer(syn, synAck, ack);
    client.and(server).watch(ScopeFactories.onEnter(() -> Log.d(TAG, "done!")));
}
```

This gives several benefits:

*   The protocol is expressed purely independent of the medium through which
    messages are exchanged (e.g. a socket connection or IPC mechanism). This
    allows you to test the protocol without worrying about the medium, just by
    creating `Controller`s for each message type, as seen in
    `simulateHandshake()`.
*   The separation of the protocol from the medium also allows writing adapters
    from `Observable`s and `Controller`s to the medium without regard to the
    details of the protocol. This way, it's trivial to change the details of how
    the messages are serialized and delivered (`Intent`s? `IBinder`? Protocol
    buffers?) by writing thin adapters of `Controller` and `Observable` events
    and actions to the desired medium, each easy to test, and each independent
    of the protocol itself besides the messages that need to be sent.
*   The `andThen()` calls impose a constraint that allows automatically ignoring
    non-conforming messages. If the client gets a `SYN-ACK` before it sent its
    `SYN`, the client will ignore it until it gets another `SYN-ACK` that
    arrives *after* its `SYN` was sent.
