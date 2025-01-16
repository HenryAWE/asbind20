# Registering Global Entities
## Global Functions and Properties
C++ code
```c++
void func(int arg);

void func_gen(asIScriptGeneric* gen);

class my_class
{
    int f();
};

my_class instance{...};

int global_var = ...;
```

Registering
```c++
asIScriptEngine* engine = ...;
asbind20::global(engine)
    // Ordinary function (native)
    .function("void func(int arg)", &func)
    // Ordinary function (generic)
    .function("void func_gen(int arg)", &func_gen)
    // asCALL_THISCALL_ASGLOBAL
    // Equivalent to `instance.f()`
    .function("int f()", &my_class::f, instance)
    .property("int global_var", global_var);
```

## Type Aliases and Enumerations
```c++
enum class my_enum : int
{
    A,
    B
};

asIScriptEngine* engine = ...;
asbind20::global(engine)
    .funcdef("bool callback(int, int)")
    .typedef_("float", "real32")
    // For those who feel more comfortable with the C++11 style `using alias = type`
    .using_("float32", "float")
    .enum_type("my_enum")
    .enum_value("my_enum", my_enum::A, "A")
    .enum_value("my_enum", my_enum::B, "B");
```

## Special Functions
Please check the official documentation of AngelScript for the requirements of following functions.

### Message Callback
Registered by `message_callback`.

### Exception Translator
Registered by `exception_translator`.
