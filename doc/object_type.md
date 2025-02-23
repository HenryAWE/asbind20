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

## Notes
1. The type flags cannot be retrieved by `asGetTypeTraits<T>()` like `asOBJ_APP_CLASS_MORE_CONSTRUCTORS` or `asOBJ_APP_CLASS_ALLINTS` needs to be set manually.  
   **If you cannot provide the correct flags, you will probably get some runtime errors like stack corruption!**

2. If a wrapper function satisfies both `asCALL_CDECL_OBJFIRST` and `asCALL_CDECL_OBJLAST`, the binding generator will prefer the `asCALL_CDECL_OBJFIRST`, i.e., treating the first argument as `this`.  
  If you need `asCALL_CDECL_OBJLAST` for this kind of functions, you can specify the calling convention manually using a tag as the last argument.
```c++
// Declaration of C++ side
void do_something(my_value_class* other, my_value_class* this_);

// Registering
asbind20::value_class<my_value_class>(...)
    .method("void do_something(my_value_class& other)", &do_something, asbind20::call_conv<asCALL_CDECL_OBJLAST>);
```

3. If you want to register a templated value class, please read the following sections.

## Generating Wrappers
asbind20 has support for generating wrappers for common usage.

### Default Constructor and Copy Constructor
Registered by `constructor()` and `copy_constructor()`.

### Custom constructor
Registered by `constructor<Args...>("params")`.
The `params` string should only contains declarations of parameters such as `int val` or `float, float`.  
If you want to register an explicit constructor, you can use the `use_explicit` tag. For example, `constructor<Args...>("params", asbind20::use_explicit)`.

If you already have a wrapper for constructor, you can register it with `constructor_function()`. The supported calling conventions of this helper are `GENERIC`, `CDECL_OBJLAST`, and `CDECL_OBJFIRST`.

NOTE: When deducing calling convention for constructors, if the convention cannot be deduced by the pointer to class in parameters, asbind20 will fallback to treat `void*` as valid parameter that emulates `this`, because some libraries use `void*` directly for placement new.

**WARNING: Remember to set the `asOBJ_APP_CLASS_MORE_CONSTRUCTORS` flag when registering a custom constructor (constructor other than default/copy constructor), otherwise you may get strange runtime error.**

### Destructor
Registered by `destructor()`.  
It will generated a wrapper for calling the destructor (`~type()`) of the registered type.

### List Constructor
Registered by `list_constructor<ListElementType>("pattern")`. This helper expects the registered type is constructible with `ListElementType*`.
If your pattern is repeated or contains variable type (`?`) which cannot have a consistent element type, you can ignore this template argument. The `ListElementType` will be set to `void` by default.

If you have a constructor that satisfies some common C++ paradigm, you can use a policy type as the second template argument. For example, if your class accept an iterator range of `int`, whose declaration is `template <typename Iterator> type(Iterator start, Iterator stop)`, you can register it by `.list_constructor<int, asbind20::policies::as_iterators>("repeat int")`. You can check the `namespace asbind20::policies` for more list constructor policies.

If you already have a wrapper, you can register it with `list_constructor_function()`. The supported calling convention of this helper are `GENERIC`, `CDECL_OBJLAST`, and `CDECL_OBJFIRST`.

NOTE: When deducing calling convention for list constructors, asbind20 will treat `void*` as valid parameter that emulates `this`, because some libraries use `void*` directly for placement new.

### Operators
Given constant C++ references `a` and `b`, as well as a variable `val` of registered type `T`,

| AngelScript Declaration              | Equivalent C++ Code                           |
| :----------------------------------- | :-------------------------------------------- |
| `T& opAssign(const T&in)`            | `val = a`                                     |
| `bool opEquals(const T&in) const`    | `a == b`                                      |
| `int opCmp(const T&in) const`        | `translate_three_way(a <=> b)` *(See Note 2)* |
| `T& opAddAssign(const T&in)`         | `val += a`                                    |
| `T& opSub/Div/MulAssign(const T&in)` | Similar to above                              |
| `T opAdd(const T&in) const`          | `a + b`                                       |
| `T opSub/Div/Mul(const T&in) const`  | Similar to above                              |
| `T& opPreInc/Dec()`                  | `++val` / `--val`                             |
| `T opPostInc/Dec()`                  | `val++` / `val++`                             |
| `T opNeg() const`                    | `-a`                                          |

**Notes:**  
1. In native mode, the binding generator will try to use the member version at first, then fallback to a lambda for using friend operators. The generic mode will directly use lambda for all operators.
2. The wrapper requires `operator<=>` returns `std::weak_ordering` at least, i.e., **no** `std::partial_ordering` support.  
   The result of three way comparison will be translated to integral value recognized by AngelScript.
3. Returning by value using native calling convention of AngelScript needs you set the type flags carefully, otherwise you might get error when receiving value from these functions. Besides, some types cannot be passed/returned by value in native calling convention on some platforms. If you encounter such situation, you can force asbind20 to use generic calling convention for the specific generated wrappers using the tag `use_generic`. For example, `c.opPostInc(asbind20::use_generic)`.

If your operators are not included in the above list, you can register them by `method()` with a lambda for choosing the correct operator overload.

## Member Aliases
You can register a member `funcdef`.  
Here use a template class as example, but the same logic also applies to value class.
```c++
template_class<script_array>(engine, "array<T>", asOBJ_GC);
    .funcdef("bool erase_if_callback(const T&in if_handle_then_const)")
    .method("void erase_if(const erase_if_callback&in fn, uint idx=0, uint n=-1)", &script_array::script_erase_if);
```

NOTE: Since the AngelScript doesn't provide an API for registering a member `funcdef` directly (as for 2.37), the binding generator will insert `class-name::` into the function signature and then register it with underlying interface.

## Utilities
### Automatically Register Common Behaviours
The `behaviours_by_traits()` will use `asGetTypeTraits<T>()` to register default constructor, copy constructor, assignment operator (`operator=` / `opAssign`) and destructor accordingly.

**NOTE: DO NOT register those function manually again after using this helper! Otherwise, you will get an error of duplicated function.**

### Generic Wrapper for Ordinary Methods
Register with advanced API `c.method(use_generic, "method decl", fp<&val_class:member_fun>)`. This will create a generic wrapper for the `val_class::member_fun` and register it with `asCALL_GENERIC` convention.

# Registering a Reference Class
For non-template reference class, register it with `ref_class` helper. This is similar to registering a value class. But the constructor is replaced by factory function. Thus, you need to register special behaviours for AngelScript to handle lifetime of the registered object type, e.g., `addref()` and `release()`.

- Behaviors for handling lifetime:  

| Registering Helper | `asEBehaviours`        | Script Declaration |
| ------------------ | ---------------------- | ------------------ |
| `addref()`         | `asBEHAVE_ADDREF`      | `void f()`         |
| `release()`        | `asBEHAVE_RELEASE`     | `void f()`         |
| `get_refcount()`   | `asBEHAVE_GETREFCOUNT` | `int f()`          |
| `set_gc_flag()`    | `asBEHAVE_SETGCFLAG`   | `void f()`         |
| `get_gc_flag()`    | `asBEHAVE_GETGCFLAG`   | `bool f()`         |
| `enum_refs()`      | `asBEHAVE_ENUMREFS`    | `void f(int&in)`   |
| `release_refs()`   | `asBEHAVE_RELEASEREFS` | `void f(int&in)`   |

Besides, the generated operator wrappers don't support function that may return a reference class by value, e.g. `T opAdd(const T&in) const`.

# Templated Value/Reference Class
The template class is special value/reference class. It can be registered with `template_value_class`/`template_ref_class`. The binding generator will automatically handle the hidden type information (`asITypeInfo*`, passed by `int&in` in AngelScript) as the first argument when generating constructor/factory function from C++ constructors.

The template validation callback (`asBEHAVE_TEMPLATE_CALLBACK`) can be registered with `template_callback`. The C++ signature of the callback should be `bool(asITypeInfo*, bool&)`. You can read the [official document explaining template callback](https://www.angelcode.com/angelscript/sdk/docs/manual/doc_adv_template.html#doc_adv_template_4).

You can find detailed example in extension for registering script array.

## Notice for returning/receiving templated value class by value using generic wrapper
Usually, the copy constructor of templated value class is declared as `(asITypeInfo*, const Class& other)`. So the class may not have a ordinary copy/move constructor in C++, thus the generic wrapper won't know how to convert them. To deal with this situation, you [need to tell asbind20 how to convert those values for generic wrapper](./custom_conv_rule.md).

You can check the script optional in the extension (`ext/utility/vocabulary.hpp`) on how to return by value for generic wrapper.

# Registering an Interface
```c++
asIScriptEngine* engine = ...;
asbind20::interface(engine, "my_interface")
    // Declaration only
    .method("int get() const")
    .funcdef("int callback(int)")
    .method("int invoke(callback@ cb) const");
```
