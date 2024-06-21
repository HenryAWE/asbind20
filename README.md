# asbind20
C++ 20 AngelScript binding library.

# Brief Examples
## 1. Binding Application Interfaces
```c++
class my_value_class
{
public:
    my_value_class() = default;
    my_value_class(const my_value_class&) = default;

    my_value_class(int val)
        : value(val) {}

    ~my_value_class() = default;

    my_value_class& operator=(const my_value_class&) = default;

    int get_val() const
    {
        return value;
    }

    void set_val(int new_val)
    {
        value = new_val;
    }

    int value = 0;
};

void add_obj_last(int val, my_value_class* this_)
{
    this_->value += val;
}

void mul_obj_first(my_value_class* this_, int val)
{
    this_->value *= val;
}
```
Binding code
```c++
asIScriptEngine* engine = /* ... */;
asbind20::value_class<my_value_class>(engine, "my_value_class")
    .register_basic_methods()
    .register_ctor<int>("void f(int val)")
    .method("void set_val(int)", &my_value_class::set_val)
    .method("int get_val() const", &my_value_class::get_val)
    .method("void add(int val)", add_obj_last)
    .method("void mul(int val)", mul_obj_first)
    .property("int value", &my_value_class::value);
```

## 2. Invoking a Script Function
AngelScript side
```angelscript
void add_ref(int i, int &out o)
{
    o = i + 1;
}
```
C++ side
```c++
asIScriptModule* m = /* Build the above script */;
asIScriptFunction* add_ref = m->GetFunctionByName("add_ref");
if(!add_ref)
    /* Error */

asIScriptContext* ctx = /* Get a script context */;

int val = 0;
asbind20::script_invoke<void>(ctx, add_ref, 1, std::ref(val));

assert(val == 2);
```

## 3. Instantiating a Script Class
Script class
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
asIScriptModule* m = /* Build the above script */;
asITypeInfo* my_class_t = m->GetTypeInfoByName("my_class");

asIScriptContext* ctx = /* Get a script context */;

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

# License
[MIT License](./LICENSE)
