[English](README.md) | **中文**

# asbind20
[![Build](https://github.com/HenryAWE/asbind20/actions/workflows/build.yml/badge.svg)](https://github.com/HenryAWE/asbind20/actions/workflows/build.yml)

**asbind20** 是一个基于模板的 C++20 [AngelScript](https://www.angelcode.com/angelscript/) 绑定库。

尽管与其他脚本语言的大多数绑定 API 相比，AngelScript 的接口对 C++ 非常友好，
它在绑定构造函数等场景时，仍然需要许多额外的代理函数。
这些代理大多数逻辑相似，可以通过一些模板技巧来生成。

该库还提供了用于轻松调用脚本函数的工具，
以及用于管理 AngelScript 对象生命周期的 RAII 辅助函数。

总的来说，该库旨在自动化所有可以通过模板元编程生成的内容。

其名称和 API 设计灵感源自著名的 [pybind11 库](https://github.com/pybind/pybind11)。

## 文档

完整文档（英文）托管在 [Read the Docs](https://asbind20.readthedocs.io/en/) 上。

中文文档制作中，敬请期待。为 Sphinx 添加多语言支持有些复杂，欢迎 PR 帮忙~

## 简易示例
### 1. 注册宿主类型

#### 注册值类型（value class）

C++ 类的声明：
```c++
class my_value_class
{
public:
    my_value_class() = default;
    my_value_class(const my_value_class&) = default;

    explicit my_value_class(int val);

    ~my_value_class() = default;

    my_value_class& operator=(const my_value_class&) = default;

    friend bool operator==(
        const my_value_class& lhs, const my_value_class& rhs
    );

    friend std::strong_ordering operator<=>(
        const my_value_class& lhs, const my_value_class& rhs
    );

    int get_val() const;
    void set_val(int new_val);

    // 设计上用 ADL 查找到的接口
    friend void process(my_value_class& val);

    // 操作符重载
    friend my_value_class operator+(const my_value_class& lhs, int rhs);
    friend my_value_class operator+(int lhs, const my_value_class& rhs);

    int value = 0;
    int another_value = 0;
};

// 在不直接更改 C++ 类型自身实现的前提下，
// 拓展其接口的包装函数
void mul_obj_first(my_value_class* this_, int val);
void add_obj_last(int val, my_value_class* this_);
void my_op_gen(asIScriptGeneric* gen);
```

Register the C++ class to the AngelScript engine:
```c++
asIScriptEngine* engine = /* 获取脚本引擎 */;
asbind20::value_class<my_value_class>(
    engine,
    "my_value_class",
    // 其他标志会用 asGetTypeTraits<T>() 自动设置
    asOBJ_APP_CLASS_ALLINTS | asOBJ_APP_CLASS_MORE_CONSTRUCTORS
)
    // 基于类型特性，生成并绑定默认构造函数、复制构造函数、析构函数
    // 和拷贝赋值函数（operator=/opAssign）
    .behaviours_by_traits()
    // 构造函数 `my_value_class::my_value_class(int val)`
    // 标签 `use_explicit` 指明该构造函数是显式的
    .constructor<int>("int val", asbind20::use_explicit)
    // 使用 C++ 中的 operator== 为 AngelScript 生成 opEquals
    .opEquals()
    // 基于 C++ 中 operator<=> 为 AngelScript 生成 opCmp，
    // 从 C++ 的 enum 比较结果翻译到 AS 中的 int 值
    .opCmp()
    // 普通成员函数
    .method("int get_val() const", &my_value_class::get_val)
    .method("void set_val(int new_val) const", &my_value_class::set_val)
    // 自动推导包装函数的调用约定
    .method("void mul(int val)", &mul_obj_first) // asCALL_CDECL_OBJFIRST
    .method("void add(int val)", &add_obj_last)  // asCALL_CDECL_OBJLAST
    .method("void my_op(int)", &set_val_gen)     // asCALL_GENERIC
    // 使用 lambda 绑定需要 ADL 查找的接口
    .method(
        "void process()",
        [](my_value_class& val) -> void { process(val); }
    )
    // 操作符重载
    .use(const_this + param<int>)
    .use(param<int> + const_this)
    // 基于成员指针注册属性
    .property("int value", &my_value_class::value)
    // 基于偏移量注册属性
    .property("int another_value", offsetof(my_value_class, another_value));
```

#### 注册全局函数

给定如下的全局函数：
```c++
float sin(float x);
float cos(float x);

void gfn(asIScriptGeneric* gen);
```

把他们注册到 AngelScript 的脚本引擎中：

```c++
asIScriptEngine* engine = /* 获取脚本引擎 */;
asbind20::global(engine)
    .function("float sin(float x)", &sin)
    .function("float cos(float x)", &cos)
    .function("int gen(int arg)", &gfn);
```

绑定生成器还支持向 AngelScript 引擎注册引用类型（reference type），接口（interface）等。
此外，它还支持在编译期生成 generic 包装器，对于像 Emscripten 这样子不支持 native 调用约定的平台非常有用。

请阅读文档以获取更多信息。

### 2. 在 C++ 侧使用 AngelScript 对象
#### 调用脚本函数

调用 AngelScript 函数时，该库可以自动转换 C++ 中的参数。
此外，该库还提供 RAII 辅助类型，便于管理如 `asIScriptContext*` 这样的 AngelScript 对象的生命周期。

AngelScript 函数：
```angelscript
string test(int a, int&out b)
{
    b = a + 1;
    return "test";
}
```

C++ 代码：
```c++
asIScriptEngine* engine = /* 获取脚本引擎 */;
asIScriptModule* m = /* 构建上文的脚本 */;
asIScriptFunction* func = m->GetFunctionByName("test");
if(!func)
    /* 错误处理 */

// 使用 RAII 辅助类管理脚本上下文
asbind20::request_context ctx(engine);

int val = 0;
auto result = asbind20::script_invoke<std::string>(
    ctx, func, 1, std::ref(val)
);

assert(result.value() == "test");
assert(val == 2);
```

#### 使用脚本类

该库提供实例化脚本类的工具。 `script_invoke` 也支持调用方法（即成员函数）。

在 AngelScript 中定义的脚本类：

```angelscript
class my_class
{
    int m_val;

    void set_val(int new_val) { m_val = new_val; }
    int get_val() const { return m_val; }
    int& get_val_ref() { return m_val; }
};
```

C++ 代码:

```c++
asIScriptEngine* engine = /* 获取脚本引擎 */;
asIScriptModule* m = /* 构建上文的脚本 */;
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

## 开源许可
[MIT 许可证](./LICENSE)
