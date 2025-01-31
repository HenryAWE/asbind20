# asbind20
[![Build](https://github.com/HenryAWE/asbind20/actions/workflows/build.yml/badge.svg)](https://github.com/HenryAWE/asbind20/actions/workflows/build.yml)

C++20 AngelScript binding library powered by templates. It also provides tools for easy interoperability between the AngelScript and C++.

The name and API design are inspired by the famous [pybind11 library](https://github.com/pybind/pybind11).

# Brief Examples
## 1. Binding Application Interfaces
```c++
class my_value_class
{
public:
    my_value_class() = default;
    my_value_class(const my_value_class&) = default;

    explicit my_value_class(int val)
        : value(val) {}

    ~my_value_class() = default;

    my_value_class& operator=(const my_value_class&) = default;

    friend bool operator==(const my_value_class& lhs, const my_value_class& rhs)
    {
        return lhs.value == rhs.value;
    }

    friend std::strong_ordering operator<=>(const my_value_class& lhs, const my_value_class& rhs)
    {
        return lhs.value <=> rhs.value;
    }

    int get_val() const
    {
        return value;
    }

    void set_val(int new_val)
    {
        value = new_val;
    }

    int value = 0;
    int another_value = 0;
};

void add_obj_last(int val, my_value_class* this_)
{
    this_->value += val;
}

void mul_obj_first(my_value_class* this_, int val)
{
    this_->value *= val;
}

void set_val_gen(asIScriptGeneric* gen)
{
    my_value_class* this_ = asbind20::get_generic_object<my_value_class*>(gen);
    int val = asbind20::get_generic_arg<int>(gen, 0);
    this_->set_val(val);
}
```
Binding code
```c++
asIScriptEngine* engine = /* Get a script engine */;
asbind20::value_class<my_value_class>(
    engine,
    "my_value_class",
    // Other flags will be automatically set
    asOBJ_APP_CLASS_ALLINTS | asOBJ_APP_CLASS_MORE_CONSTRUCTORS
)
    // Generate & register the default constructor, copy constructor, destructor,
    // and assignment operator (operator=/opAssign) based on type traits
    .behaviours_by_traits()
    // The constructor `my_value_class::my_value_class(int val)`
    // The tag `use_explicit` indicates an explicit constructor.
    .constructor<int>("int val", asbind20::use_explicit)
    // Generate opEquals for AngelScript using operator== in C++
    .opEquals()
    // Generate opCmp for AngelScript using operator<=> in C++,
    // translating comparison result from the C++ enum to int value for AS.
    .opCmp()
    // Ordinary member functions
    .method("int get_val() const", &my_value_class::get_val)
    // Automatically deducing calling conventions for wrapper functions.
    .method("void add(int val)", &add_obj_last)  // asCALL_CDECL_OBJLAST
    .method("void mul(int val)", &mul_obj_first) // asCALL_CDECL_OBJFIRST
    .method("void set_val(int)", &set_val_gen)   // asCALL_GENERIC
    // Register property by member pointer.
    .property("int value", &my_value_class::value)
    // Register property by offset.
    .property("int another_value", offsetof(my_value_class, another_value));
```
The binding helpers also support registering a reference type, an interface, or global functions, etc. to the AngelScript engine.  
You can find more examples in `ext/container/src/array.cpp` and `ext/container/src/stdstring.cpp`.

## 2. Invoking a Script Function
The library can automatically convert arguments in C++ for invoking an AngelScript function. Besides, the library provides RAII classes for easily managing lifetime of AngelScript object like `asIScriptContext`.

- AngelScript side
```angelscript
string test(int a, int&out b)
{
    b = a + 1;
    return "test";
}
```
- C++ side
```c++
asIScriptEngine* engine = /* Get a script engine */;
asIScriptModule* m = /* Build the above script */;
asIScriptFunction* func = m->GetFunctionByName("test");
if(!func)
    /* Error handling */

// Manage script context using RAII
asbind20::request_context ctx(engine);

int val = 0;
auto result = asbind20::script_invoke<std::string>(
    ctx, func, 1, std::ref(val)
);

assert(result.value() == "test");
assert(val == 2);
```
You can find more examples in `test/test_invoke.cpp`.

## 3. Using a Script Class
The library provides tools for instantiating a script class. The `script_invoke` also supports invoking a method, a.k.a., member function.

Script class in AngelScript
```angelscript
class my_class
{
    int m_val;
    void set_val(int new_val) { m_val = new_val; }
    int get_val() const { return m_val; }
    int& get_val_ref() { return m_val; }
};
```
C++ code
```c++
asIScriptEngine* engine = /* Get a script engine */;
asIScriptModule* m = /* Build the above script */;
asITypeInfo* my_class_t = m->GetTypeInfoByName("my_class");

asbind20::request_context ctx(engine);

auto my_class = asbind20::instantiate_class(ctx, my_class_t);

asIScriptFunction* set_val = my_class_t->GetMethodByDecl("void set_val(int)");
asbind20::script_invoke<void>(ctx, my_class, set_val, 182375);

asIScriptFunction* get_val = my_class_t->GetMethodByDecl("int get_val() const");
auto val = asbind20::script_invoke<int>(ctx, my_class, get_val);

assert(val.value() == 182375);

asIScriptFunction* get_val_ref = my_class_t->GetMethodByDecl("int& get_val_ref()");
auto val_ref = asbind20::script_invoke<int&>(ctx, my_class, get_val_ref);

assert(val_ref.value() == 182375);

*val_ref = 182376;

val = asbind20::script_invoke<int>(ctx, my_class, get_val);
assert(val.value() == 182376);
```

# Advanced Features
The asbind20 library also provides tools for advanced users. You can find detailed examples in extensions and unit tests.

## 1. Generating Generic Wrapper at Compile-Time
With the power of non-type template parameter (NTTP), asbind20 can generate generic wrapper by macro-free utilities. This can be useful for binding interface on platform without native calling convention support, e.g., Emscripten.

```c++
// Helpers
using asbind20::fp;
using asbind20::call_conv;
// Usage: to_asGENFUNC_t(fp<MyFunction>, call_conv<OriginalCallConv>);
asGENFUNC_t f1 = asbind20::to_asGENFUNC_t(fp<&global_func>, call_conv<asCALL_CDECL>);
asGENFUNC_t f2 = asbind20::to_asGENFUNC_t(fp<&my_class::method>, call_conv<asCALL_THISCALL>);
using asbind20::var_type;
// For function with variable type argument
// void*, int pair in C++ / ?& in AngelScript
// Use a helper to mark position of those special argument, i.e. index of the void*.
asGENFUNC_t f3 = asbind20::to_asGENFUNC_t(fp<&var_type_func>, call_conv<asCALL_THISCALL>, var_type<0>);

// Use `use_generic` tag to force generated proxies to use asCALL_GENERIC
asbind20::value_class<my_value_class>(...)
    .constructor<int>(asbind20::use_generic, "void f(int)")
    .opEquals(asbind20::use_generic)
    // Due to limitation of C++,
    // you can only pass the function pointer as template argument to the generic wrapper.
    .method<&my_value_class::mem_func>(asbind20::use_generic,"int mem_func(int arg)");
```

If you want to force all registered functions to be generic, you can set the `ForceGeneric` flag of binding generator to `true`.  
Trying to register functions by native calling convention with `ForceGeneric` enabled will cause a compile-time error.
```c++
asbind20::value_class<my_value_class, true>(...);
asbind20::ref_class<my_ref_class, true>(...);
asbind20::global<true>(...);
```

Note: If you use an outer template argument to control the mode of generator, the generator can be a dependent name, thus you need an additional `template` keyword to call the binding generator.
```c++
template <bool UseGeneric>
void register_my_class(asIScriptEngine* engine)
{
    asbind20::value_class<my_value_class, UseGeneric>(engine, "my_value_class")
        .template method<&my_value_class::mem_func>("int mem_func(int arg)")
}
```


## 2. Dispatching Function Calls Based on Type Ids
This feature is similar to how `std::visit` and `std::variant` works. It can be used for developing templated container for AngelScript.

```c++
asbind20::visit_primitive_type(
    []<typename T>(T* val)
    {
        using type = std::remove_const_t<T>;

        if constexpr(std::same_as<type, int>)
        {
            // play with int
        }
        else if constexpr(std::same_as<type, float>)
        {
            // play with float
        }
        else
        {
            // ...
        }
    },
    as_type_id, // asTYPEID_BOOL, asTYPEID_INT32, etc.
    ptr_to_val // (const) void* to value
);
```
You can find example usage in `ext/container/src/array.cpp`.

# Supported Platforms
- CMake >= 3.20
- AngelScript >= 2.37.0
- Any C++ compiler that supports C++20.  
  Currently, this library has been tested by MSVC 19.41 on Windows and GCC 12 / Clang 18 on Linux.
  You can find build and test status in the GitHub Actions.

# How to Use
Follow the tutorial of AngelScript to build and install it at first, or use a package manager like [vcpkg](https://github.com/microsoft/vcpkg).
## A. Build and Install
Build and install the library.
```sh
cmake -GNinja -DCMAKE_BUILD_TYPE=Release -S . -B build
cmake --build build
cmake --install build
```
Use the library in a `CMakeLists.txt`.
```cmake
find_package(asbind20 REQUIRED)

target_link_libraries(main PRIVATE asbind20::asbind20)
```
You can find a detailed example in `test/test_install/`.

## B. As Submodule
Clone the library into your project.
```sh
git clone https://github.com/HenryAWE/asbind20.git
```
Use the library in a `CMakeLists.txt`.
```cmake
add_subdirectory(asbind20)

target_link_libraries(main PRIVATE asbind20::asbind20)
```
You can find a detailed example in `test/test_subdir/`.

# Documentation
Detailed explanation of asbind20.

1. [Registering Object Types](./doc/object_type.md)
2. [Registering Global Entities](./doc/global.md)
3. [Customize Type Conversion Rules](./doc/custom_conv_rule.md)

# Known Limitations
Some feature of this library may not work on a broken compiler.

1. Some version of Clang (<= 15) may fail to compile extension and test due to compiler bug.
2. Some utilities are implemented by non-standard C++, but they are guaranteed to have correct result on common platforms, which are tested by CI.

3. If you bind an overloaded function using syntax like `static_cast<return_type(*)()>(&func)` on MSVC, it may crash the compiler. The workaround is to write a standalone wrapper function with no overloading, then bind this wrapper to AngelScript.

4. GCC 12 wrongly [rejects function pointer non-type template parameter without linkage](https://gcc.gnu.org/bugzilla/show_bug.cgi?id=83258). This will cause error when using wrapper with a lambda. This has been fixed in GCC 13.2.

# License
[MIT License](./LICENSE)
