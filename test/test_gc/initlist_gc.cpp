#include <gtest/gtest.h>
#include <shared_test_lib.hpp>
#include <vector>
#include <asbind20/asbind.hpp>
#include <asbind20/ext/assert.hpp>
#include <iterator>

namespace test_gc
{
template <typename T>
struct is_apply_to : public std::false_type
{};

template <std::size_t Size>
struct is_apply_to<asbind20::policies::apply_to<Size>> : public std::true_type
{};

class gc_init_list
{
    void set_engine()
    {
        m_engine = asbind20::current_context()->GetEngine();
    }

public:
    std::vector<int> ints;

    gc_init_list()
    {
        set_engine();
    }

    gc_init_list(int v0, int v1)
        : ints{v0, v1}
    {
        set_engine();
    }

    gc_init_list(int v0, int v1, int v2, int v3)
        : ints{v0, v1, v2, v3}
    {
        set_engine();
    }

    gc_init_list(int* ptr, std::size_t size)
        : ints(ptr, ptr + size)
    {
        set_engine();
    }

    template <typename Iterator>
    gc_init_list(Iterator start, Iterator sentinel)
        : ints(start, sentinel)
    {
        set_engine();
    }

    gc_init_list(std::initializer_list<int> il)
        : ints(il)
    {
        set_engine();
    }

    gc_init_list(asbind20::script_init_list_repeat list)
    {
        set_engine();

        int* start = (int*)list.data();
        int* sentinel = start + list.size();
        std::copy(start, sentinel, std::back_inserter(ints));
    }

    gc_init_list(std::span<const int> sp)
        : ints(sp.begin(), sp.end())
    {
        set_engine();
    }

    bool get_gc_flag() const
    {
        return m_gc_flag;
    }

    void set_gc_flag()
    {
        m_gc_flag = true;
    }

    void addref()
    {
        m_gc_flag = false;
        ++m_counter;
    }

    void release()
    {
        m_counter.dec_and_try_delete(this);
    }

    int get_refcount() const
    {
        return m_counter;
    }

    void copy(const void* ref, int type_id)
    {
        assert(ref != nullptr);
        if(type_id == AS_NAMESPACE_QUALIFIER asTYPEID_VOID)
        {
            clear_var();
            return;
        }

        asbind20::container::single::copy_assign_from(
            m_var_data, m_engine, type_id, ref
        );
        m_var_type_id = type_id;
    }

    bool get_var(void* out, int type_id) const
    {
        if(type_id != m_var_type_id)
            return false;
        if(asbind20::is_void_type(type_id))
            return false;

        asbind20::container::single::copy_assign_to(
            m_var_data, m_engine, m_var_type_id, out
        );
        return true;
    }

    void clear_var()
    {
        if(asbind20::is_void_type(m_var_type_id))
            return;

        asbind20::container::single::destroy(
            m_var_data, m_engine, m_var_type_id
        );
    }

    void release_refs()
    {
        clear_var();
    }

    void enum_refs()
    {
        if(asbind20::is_void_type(m_var_type_id))
            return;

        asbind20::container::single::enum_refs(
            m_var_data, m_engine->GetTypeInfoById(m_var_type_id)
        );
    }

    int get_ints(AS_NAMESPACE_QUALIFIER asUINT idx) const
    {
        return ints.at(idx);
    }

    AS_NAMESPACE_QUALIFIER asUINT int_count() const
    {
        return static_cast<AS_NAMESPACE_QUALIFIER asUINT>(ints.size());
    }

private:
    asbind20::atomic_counter m_counter;
    bool m_gc_flag = false;

    AS_NAMESPACE_QUALIFIER asIScriptEngine* m_engine;
    asbind20::container::single::data_type m_var_data;
    int m_var_type_id = AS_NAMESPACE_QUALIFIER asTYPEID_VOID;
};

template <bool UseGeneric>
auto register_gc_init_list_basic_methods(AS_NAMESPACE_QUALIFIER asIScriptEngine* engine)
{
    using namespace asbind20;

    return ref_class<gc_init_list, UseGeneric>(engine, "gc_init_list", AS_NAMESPACE_QUALIFIER asOBJ_GC)
        .addref(fp<&gc_init_list::addref>)
        .release(fp<&gc_init_list::release>)
        .get_refcount(fp<&gc_init_list::get_refcount>)
        .set_gc_flag(fp<&gc_init_list::set_gc_flag>)
        .get_gc_flag(fp<&gc_init_list::get_gc_flag>)
        .release_refs(fp<&gc_init_list::release_refs>)
        .enum_refs(fp<&gc_init_list::enum_refs>)
        .default_factory(use_policy<policies::notify_gc>)
        .method("void copy(const ?&in)", fp<&gc_init_list::copy>, var_type<0>)
        .method("bool get_var(?&out) const", fp<&gc_init_list::get_var>, var_type<0>)
        .method("uint get_int_count() const property", fp<&gc_init_list::int_count>)
        .method("int get_ints(uint) const property", fp<&gc_init_list::get_ints>);
}

template <asbind20::policies::initialization_list_policy IListPolicy, bool UseGeneric>
void register_gc_init_list(AS_NAMESPACE_QUALIFIER asIScriptEngine* engine)
{
    using namespace asbind20;

    auto c = register_gc_init_list_basic_methods<UseGeneric>(engine);
    if constexpr(is_apply_to<IListPolicy>::value)
    {
        std::string pattern;
        for(std::size_t i = 0; i < IListPolicy::size(); ++i)
        {
            if(i > 0)
                pattern += ",";
            pattern += "int";
        }

        c
            .template list_factory<int>(pattern.c_str(), use_policy<IListPolicy, policies::notify_gc>);
    }
    else
    {
        c
            .template list_factory<int>("repeat int", use_policy<IListPolicy, policies::notify_gc>);
    }
}

static constexpr char test_initlist_gc_script[] = R"AngelScript(class foo
{
    gc_init_list@ il_ref;
};

bool test0()
{
    gc_init_list il;
    assert(il.int_count == 0);

    foo@ f = foo();
    @f.il_ref = @il;
    il.copy(@f);

    foo@ tmp = null;
    bool result = il.get_var(@tmp);
    assert(tmp is f);

    return result;
}

bool test1()
{
    gc_init_list il = {10, 13};
    assert(il.int_count == 2);
    assert(il.ints[0] == 10);
    assert(il.ints[1] == 13);

    foo@ f = foo();
    @f.il_ref = @il;
    il.copy(@f);

    foo@ tmp = null;
    bool result = il.get_var(@tmp);
    assert(tmp is f);

    return result;
}

bool test2()
{
    gc_init_list il = {1, 0, 1, 3};
    assert(il.int_count == 4);
    assert(il.ints[0] == 1);
    assert(il.ints[1] == 0);
    assert(il.ints[2] == 1);
    assert(il.ints[3] == 3);

    foo@ f = foo();
    @f.il_ref = @il;
    il.copy(@f);

    foo@ tmp = null;
    bool result = il.get_var(@tmp);
    assert(tmp is f);

    return result;
}
)AngelScript";

template <asbind20::policies::initialization_list_policy IListPolicy, bool UseGeneric>
class basic_initlist_gc_test : public ::testing::Test
{
public:
    void SetUp() override
    {
        if constexpr(!UseGeneric)
        {
            if(asbind20::has_max_portability())
                GTEST_SKIP() << "max portability";
        }

        m_engine = asbind20::make_script_engine();

        asbind_test::setup_message_callback(m_engine, true);
        asbind20::ext::register_script_assert(
            m_engine,
            [](std::string_view msg)
            {
                FAIL() << "initlist GC assertion failed: " << msg;
            }
        );
        register_gc_init_list<IListPolicy, UseGeneric>(m_engine);
    }

    void TearDown() override
    {
        m_engine.reset();
    }

    auto build_script()
        -> AS_NAMESPACE_QUALIFIER asIScriptModule*
    {
        auto* m = m_engine->GetModule(
            "test_gc_initlist", AS_NAMESPACE_QUALIFIER asGM_ALWAYS_CREATE
        );

        m->AddScriptSection(
            "test_gc_initlist",
            test_initlist_gc_script
        );
        EXPECT_GE(m->Build(), 0);

        return m;
    }

    int max_test_idx()
    {
        return 2;
    }

private:
    asbind20::script_engine m_engine;
};

static constexpr char test_apply_to_gc_script_2[] = R"AngelScript(class foo
{
    gc_init_list@ il_ref;
};

bool test0()
{
    gc_init_list il = {10 , 13};
    assert(il.int_count == 2);
    assert(il.ints[0] == 10);
    assert(il.ints[1] == 13);

    foo@ f = foo();
    @f.il_ref = @il;
    il.copy(@f);

    foo@ tmp = null;
    bool result = il.get_var(@tmp);
    assert(tmp is f);

    return result;
}
)AngelScript";

static constexpr char test_apply_to_gc_script_4[] = R"AngelScript(class foo
{
    gc_init_list@ il_ref;
};

bool test0()
{
    gc_init_list il = {1, 0, 1, 3};
    assert(il.int_count == 4);
    assert(il.ints[0] == 1);
    assert(il.ints[1] == 0);
    assert(il.ints[2] == 1);
    assert(il.ints[3] == 3);

    foo@ f = foo();
    @f.il_ref = @il;
    il.copy(@f);

    foo@ tmp = null;
    bool result = il.get_var(@tmp);
    assert(tmp is f);

    return result;
}
)AngelScript";

template <std::size_t Size, bool UseGeneric>
requires(Size == 2 || Size == 4)
class basic_initlist_gc_test<asbind20::policies::apply_to<Size>, UseGeneric> : public ::testing::Test
{
public:
    void SetUp() override
    {
        if constexpr(!UseGeneric)
        {
            if(asbind20::has_max_portability())
                GTEST_SKIP() << "max portability";
        }

        m_engine = asbind20::make_script_engine();

        asbind_test::setup_message_callback(m_engine, true);
        asbind20::ext::register_script_assert(
            m_engine,
            [](std::string_view msg)
            {
                FAIL() << "initlist GC assertion failed: " << msg;
            }
        );
        register_gc_init_list<asbind20::policies::apply_to<Size>, UseGeneric>(m_engine);
    }

    void TearDown() override
    {
        m_engine.reset();
    }

    auto build_script()
        -> AS_NAMESPACE_QUALIFIER asIScriptModule*
    {
        auto* m = m_engine->GetModule(
            "test_gc_initlist", AS_NAMESPACE_QUALIFIER asGM_ALWAYS_CREATE
        );

        if constexpr(Size == 2)
        {
            m->AddScriptSection(
                "test_gc_initlist",
                test_apply_to_gc_script_2
            );
        }
        else if constexpr(Size == 4)
        {
            m->AddScriptSection(
                "test_gc_initlist",
                test_apply_to_gc_script_4
            );
        }
        EXPECT_GE(m->Build(), 0);

        return m;
    }

    int max_test_idx()
    {
        return 0;
    }

private:
    asbind20::script_engine m_engine;
};

void run_initlist_gc_test(AS_NAMESPACE_QUALIFIER asIScriptModule* m, int max_test_idx)
{
    auto* engine = m->GetEngine();

    for(int i = 0; i <= max_test_idx; ++i)
    {
        std::string decl = asbind20::string_concat(
            "bool test", std::to_string(i), "()"
        );
        SCOPED_TRACE("Decl: " + decl);

        auto* f = m->GetFunctionByDecl(decl.c_str());
        EXPECT_NE(f, nullptr);
        if(!f)
            continue;

        asbind20::request_context ctx(engine);
        auto result = asbind20::script_invoke<bool>(ctx, f);
        EXPECT_TRUE(asbind_test::result_has_value(result));
        EXPECT_TRUE(result.value());
    }
}

class list_policies_name
{
public:
    template <typename T>
    static std::string GetName(int)
    {
        using std::same_as;
        using namespace asbind20::policies;

        if constexpr(is_apply_to<T>::value)
            return asbind20::string_concat("apply_to<", std::to_string(T::size()), '>');
        if constexpr(same_as<T, pointer_and_size>)
            return "pointer_and_size";
        if constexpr(same_as<T, as_span>)
            return "as_span";
        if constexpr(same_as<T, as_iterators>)
            return "as_iterators";
        if constexpr(same_as<T, as_initializer_list>)
            return "as_initializer_list";
        if constexpr(same_as<T, repeat_list_proxy>)
            return "repeat_list_proxy";
    }
};
} // namespace test_gc

using test_gc::list_policies_name;

template <asbind20::policies::initialization_list_policy IListPolicy>
using initlist_gc_test_native = test_gc::basic_initlist_gc_test<IListPolicy, false>;

template <asbind20::policies::initialization_list_policy IListPolicy>
using initlist_gc_test_generic = test_gc::basic_initlist_gc_test<IListPolicy, true>;

using initlist_policies = ::testing::Types<
    asbind20::policies::apply_to<2>,
    asbind20::policies::apply_to<4>,
    asbind20::policies::pointer_and_size,
    asbind20::policies::as_span,
    asbind20::policies::as_iterators,
#ifdef ASBIND20_HAS_AS_INITIALIZER_LIST
    asbind20::policies::as_initializer_list,
#endif
    asbind20::policies::repeat_list_proxy>;

TYPED_TEST_SUITE(initlist_gc_test_native, initlist_policies, list_policies_name);
TYPED_TEST_SUITE(initlist_gc_test_generic, initlist_policies, list_policies_name);

TYPED_TEST(initlist_gc_test_native, run_script)
{
    AS_NAMESPACE_QUALIFIER asIScriptModule* m = TestFixture::build_script();
    test_gc::run_initlist_gc_test(m, TestFixture::max_test_idx());
}

TYPED_TEST(initlist_gc_test_generic, run_script)
{
    AS_NAMESPACE_QUALIFIER asIScriptModule* m = TestFixture::build_script();
    test_gc::run_initlist_gc_test(m, TestFixture::max_test_idx());
}

namespace test_gc
{
static constexpr char test_custom_list_function[] = R"AngelScript(class foo
{
    gc_init_list@ il_ref;
};

bool test0()
{
    gc_init_list il = {182, 376};
    assert(il.int_count == 3);
    assert(il.ints[0] == 18);
    assert(il.ints[1] == 23);
    assert(il.ints[2] == 76);

    foo@ f = foo();
    @f.il_ref = @il;
    il.copy(@f);

    foo@ tmp = null;
    bool result = il.get_var(@tmp);
    assert(tmp is f);

    return result;
}
)AngelScript";

static void custom_factory_set_ints(std::vector<int>& out, void* list_buf)
{
    asbind20::script_init_list_repeat list(list_buf);
    int* data = (int*)list.data();
    EXPECT_EQ(list.size(), 2);
    out.push_back(data[0] / 10);
    EXPECT_EQ(out[0], 18);
    out.push_back((data[0] % 10) * 10 + (data[1] / 100));
    EXPECT_EQ(out[1], 23);
    out.push_back(data[1] % 100);
    EXPECT_EQ(out[2], 76);
}

static gc_init_list* gc_init_list_custom_list_factory_objfirst(AS_NAMESPACE_QUALIFIER asITypeInfo* ti, void* list_buf)
{
    std::unique_ptr<gc_init_list> p(new gc_init_list);

    custom_factory_set_ints(p->ints, list_buf);

    ti->GetEngine()->NotifyGarbageCollectorOfNewObject(p.get(), ti);
    return p.release();
}

static gc_init_list* gc_init_list_custom_list_factory_objlast(void* list_buf, AS_NAMESPACE_QUALIFIER asITypeInfo* ti)
{
    std::unique_ptr<gc_init_list> p(new gc_init_list);

    custom_factory_set_ints(p->ints, list_buf);

    ti->GetEngine()->NotifyGarbageCollectorOfNewObject(p.get(), ti);
    return p.release();
}

template <bool Objfirst, bool UseGeneric>
class basic_custom_function_suite : public ::testing::Test
{
public:
    void SetUp() override
    {
        if constexpr(!UseGeneric)
        {
            if(asbind20::has_max_portability())
                GTEST_SKIP() << "max portability";
        }

        m_engine = asbind20::make_script_engine();

        asbind_test::setup_message_callback(m_engine, true);
        asbind20::ext::register_script_assert(
            m_engine,
            [](std::string_view msg)
            {
                FAIL() << "initlist GC assertion failed: " << msg;
            }
        );

        using namespace asbind20;
        auto c = register_gc_init_list_basic_methods<UseGeneric>(m_engine);
        if constexpr(Objfirst)
        {
            c.list_factory_function("repeat int", fp<&gc_init_list_custom_list_factory_objfirst>, auxiliary(this_type));
        }
        else // Objlast
        {
            c.list_factory_function("repeat int", fp<&gc_init_list_custom_list_factory_objlast>, auxiliary(this_type));
        }
    }

    void TearDown() override
    {
        m_engine.reset();
    }

    auto build_script()
        -> AS_NAMESPACE_QUALIFIER asIScriptModule*
    {
        auto* m = m_engine->GetModule(
            "test_gc_initlist", AS_NAMESPACE_QUALIFIER asGM_ALWAYS_CREATE
        );

        m->AddScriptSection(
            "test_gc_initlist",
            test_custom_list_function
        );
        EXPECT_GE(m->Build(), 0);
        return m;
    }

    int max_test_idx()
    {
        return 0;
    }

private:
    asbind20::script_engine m_engine;
};
} // namespace test_gc

using custom_list_function_objfirst_generic = test_gc::basic_custom_function_suite<true, true>;
using custom_list_function_objfirst_native = test_gc::basic_custom_function_suite<true, false>;

using custom_list_function_objlast_generic = test_gc::basic_custom_function_suite<false, true>;
using custom_list_function_objlast_native = test_gc::basic_custom_function_suite<false, false>;

TEST_F(custom_list_function_objfirst_native, run_script)
{
    AS_NAMESPACE_QUALIFIER asIScriptModule* m = build_script();
    test_gc::run_initlist_gc_test(m, max_test_idx());
}

TEST_F(custom_list_function_objfirst_generic, run_script)
{
    AS_NAMESPACE_QUALIFIER asIScriptModule* m = build_script();
    test_gc::run_initlist_gc_test(m, max_test_idx());
}

TEST_F(custom_list_function_objlast_native, run_script)
{
    AS_NAMESPACE_QUALIFIER asIScriptModule* m = build_script();
    test_gc::run_initlist_gc_test(m, max_test_idx());
}

TEST_F(custom_list_function_objlast_generic, run_script)
{
    AS_NAMESPACE_QUALIFIER asIScriptModule* m = build_script();
    test_gc::run_initlist_gc_test(m, max_test_idx());
}
