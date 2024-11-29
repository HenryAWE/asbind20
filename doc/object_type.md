# Registering a Value Class
Given a C++ class,
```c++
class val_class
{
    val_class();
    val_class(const val_class&);

    val_class(const std::string& str);

    val_class& operator=(const val_class&);

    ~val_class();

    int method();

    std::string value;

    bool operator==(const val_class&) const;
    std::strong_ordering operator<=>(const val_class&) const;
};
```

Register the interface with asbind20
```c++
using namespace asbind20;

value_class<val_class>(
    engine,
    "val_class",
    asOBJ_APP_CLASS_MORE_CONSTRUCTORS
) c;

c
    .behaviours_by_traits()
    // Register std::string before register this class
    .constructor<const std::string&>("const string&in")
    .destructor()
    .method("int method()", &val_class:method)
    .property("string value", &val_class::value)
    .opEquals()
    .opCmp();
```

You can find detailed example in extension for registering the `std::string`.

## Generating Wrappers
asbind20 has support for generating wrappers for common usage.

### Default Constructor and Copy Constructor
Registered by `constructor()` and `copy_constructor()`.

### Custom constructor
Registered by `constructor<Args...>("params")`.
The `params` string should only contains declarations of parameters such as `int val` or `float, float`.

If you already have a wrapper for constructor, you can register it with `constructor_function()`. The supported calling convention of this helper are `GENERIC`, `CDECL_OBJLAST`, and `CDECL_OBJFIRST`.

### Destructor
Registered by `destructor()`

### List Constructor
Registered by `list_constructor<ListElementType>("pattern")`. This helper expects the registered type is constructible with `ListElementType*`.
If your pattern is repeated or contains variable type (`?`) which cannot have a consistent element type, you can ignore this template argument. The `ListElementType` will be set to `void` by default.

If you already have a wrapper, you can register it with `list_constructor_function()`. The supported calling convention of this helper are `GENERIC`, `CDECL_OBJLAST`, and `CDECL_OBJFIRST`.

NOTE: This wrapper doesn't support repeated pattern.

### Operators
Given constant C++ references `a` and `b`, as well as a variable `val` of registered type `T`,

| AngelScript Declaration              | Equivalent C++ Code                           |
| :----------------------------------- | :-------------------------------------------- |
| `T& opAssign(const T&in)`            | `val = a`                                     |
| `bool opEquals(const T&in) const`    | `a == b`                                      |
| `int opCmp(const T&in) const`        | `translate_three_way(a <=> b)` *(See Note 2)* |
| `T& opAddAssign(const T&in)`         | `val += a`                                    |
| `T& opSub/Div/MulAssign(const T&in)` | Same as above                                 |
| `T opAdd(const T&in) const`          | `a + b`                                       |
| `T opSub/Div/Mul(const T&in) const`  | Same as above                                 |
| `T& opPreInc/Dec()`                  | `++val` / `--val`                             |
| `T opPostInc/Dec()`                  | `val++` / `val++`                             |
| `T opNeg() const`                    | `-a`                                          |

**Notes:**  
1. In native mode, the binding generator will try to use the member version at first, then fallback to a lambda for using friend operators. The generic mode will directly use lambda for all operators.
2. The wrapper requires `operator<=>` returns `std::weak_ordering` at least. The result of three way comparison will be translate to integral value recognized by AngelScript.
3. Returning by value using native calling convention of AngelScript needs you set the type flags carefully, otherwise you might get error when receiving value from these functions. If you encounter such situation, you can force asbind20 to use generic calling convention for generated wrappers using the tag `use_generic`. For example, `c.opPostInc(asbind20::use_generic)`.

## Utilities
### Automatically Register Common Behaviours
The `behaviours_by_traits()` will use `asGetTypeTraits<T>()` to register default constructor, copy constructor, assignment operator (`operator=` / `opAssign`) and destructor accordingly.

**NOTE: DO NOT register those function manually again after using this helper! Otherwise, you will get an error of duplicated function.**

### Generic Wrapper for Ordinary Methods
Register with advanced API `c.method<&val_class:member_fun>(use_generic, "method decl")`. This will create a generic wrapper for the `val_class::member_fun` and register it with `asCALL_GENERIC` convention.

# Registering a Reference Class
For non-template reference class, register it with `ref_class` helper. This is similar to registering a value class. But the constructor is replaced by factory function. Thus, you need to register special behaviours for AngelScript handling lifetime of the registered object type, e.g. `addref()` and `release()`.

Besides, the generated operator wrappers don't support function that may return a reference class by value.

The template class, registered with `template_class`, will automatically handle the hidden type information (passed by `int&in` in AngelScript) as the first argument when generating factory function from constructors.

You can find detailed example in extension for registering script array.
