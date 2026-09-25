#include <asbind_test/framework.hpp>
#include <sstream>
#include <unordered_set>

TEST(RAII, LockableSharedBool)
{
    using asbind20::lockable_shared_bool;

    lockable_shared_bool flag = asbind20::make_lockable_shared_bool();
    ASSERT_TRUE(flag);
    EXPECT_THAT(flag, ::testing::NotNull());

    lockable_shared_bool copy(flag);
    EXPECT_EQ(copy.get(), flag.get());
    EXPECT_TRUE(copy);
    EXPECT_THAT(copy, ::testing::NotNull());

    lockable_shared_bool moved(std::move(copy));
    EXPECT_EQ(moved.get(), flag.get());
    EXPECT_FALSE(copy);

    copy = moved;
    EXPECT_EQ(copy.get(), flag.get());
    EXPECT_TRUE(copy);

    moved = std::move(copy);
    EXPECT_EQ(moved.get(), flag.get());
    EXPECT_FALSE(copy);

    EXPECT_FALSE(moved.get_flag());
    moved.set_flag(true);

    {
        std::lock_guard lock(moved);
        EXPECT_TRUE(moved.get_flag());
    }

    moved.set_flag(false);
    EXPECT_FALSE(moved.get_flag());
}

TEST(RAII, ScriptEngine)
{
    using asbind20::engine_pointer;
    using asbind20::script_engine;
    using asbind20::shared_script_engine;

    // Conversions are explicit
    static_assert(!std::is_convertible_v<const script_engine&, bool>);
    static_assert(!std::is_convertible_v<const script_engine&, engine_pointer>);

    script_engine null_engine;
    EXPECT_FALSE(null_engine);
    EXPECT_THAT(null_engine, ::testing::IsNull());
    EXPECT_EQ(null_engine, nullptr);
    EXPECT_EQ(null_engine, null_engine);

    auto engine = asbind20::make_script_engine();
    EXPECT_TRUE(engine);
    EXPECT_THAT(engine, ::testing::NotNull());
    EXPECT_NE(engine, nullptr);

    // Comparison with a raw pointer works in both directions
    engine_pointer raw = engine.get();
    EXPECT_EQ(engine, raw);
    EXPECT_EQ(raw, engine);
    EXPECT_NE(null_engine, raw);
    EXPECT_NE(engine, null_engine);

    auto shared = asbind20::make_shared_script_engine();
    engine_pointer shared_raw = shared.get();
    EXPECT_THAT(shared, ::testing::NotNull());
    EXPECT_EQ(shared, shared_raw);
    EXPECT_EQ(shared_raw, shared);
    EXPECT_NE(shared, nullptr);

    shared_script_engine null_shared;
    EXPECT_THAT(null_shared, ::testing::IsNull());
    EXPECT_EQ(null_shared, nullptr);
    EXPECT_NE(null_shared, shared_raw);

    script_engine moved(std::move(engine));
    EXPECT_TRUE(moved);
    EXPECT_EQ(moved, raw);
    EXPECT_FALSE(engine);
    EXPECT_EQ(engine, nullptr);
    EXPECT_THAT(engine, ::testing::IsNull());

    moved.reset();
    EXPECT_FALSE(moved);
    EXPECT_THAT(moved, ::testing::IsNull());

    shared.reset();
    EXPECT_FALSE(shared);
    EXPECT_THAT(shared, ::testing::IsNull());
}

TEST(RAII, Hashing)
{
    using namespace asbind20;

    auto engine = make_script_engine();
    asbind_test::setup_message_callback(engine);

    auto* m = create_module(engine, "test");
    m->AddScriptSection("test", "class foo { int val = 0; }");
    ASSERT_GE(m->Build(), 0);

    auto foo_t = script_typeinfo(m->GetTypeInfoByName("foo"));
    ASSERT_TRUE(foo_t);

    request_context ctx(engine);
    auto foo = instantiate_class(ctx, foo_t);
    ASSERT_TRUE(foo);

    // Wrappers holding the same object must be equal and hash equally
    script_object same(foo);
    EXPECT_EQ(foo, same);
    EXPECT_EQ(
        std::hash<script_object>{}(foo),
        std::hash<script_object>{}(same)
    );

    auto another = instantiate_class(ctx, foo_t);
    ASSERT_TRUE(another);
    EXPECT_NE(foo, another);

    std::unordered_set<script_object> objs;
    objs.insert(foo);
    objs.insert(same);
    objs.insert(another);
    EXPECT_EQ(objs.size(), 2);

    m->Discard();
}
