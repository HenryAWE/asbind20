#include <asbind_test/framework.hpp>
#include <unordered_set>

TEST(RAII, LockableSharedBool)
{
    using asbind20::lockable_shared_bool;

    lockable_shared_bool flag = asbind20::make_lockable_shared_bool();
    ASSERT_TRUE(flag);

    lockable_shared_bool copy(flag);
    EXPECT_EQ(copy.get(), flag.get());
    EXPECT_TRUE(copy);

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
