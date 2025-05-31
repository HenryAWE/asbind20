/**
 * @file testing.hpp
 * @author HenryAWE
 * @brief Testing framework for AngelScript
 */

#ifndef ASBIND20_EXT_TESTING_TESTING_HPP
#define ASBIND20_EXT_TESTING_TESTING_HPP

#include <iosfwd>
#include <asbind20/asbind.hpp>

namespace asbind20::ext
{
namespace testing
{
    class suite;
} // namespace testing

// TODO: Comparing different types

void register_script_test_framework(
    AS_NAMESPACE_QUALIFIER asIScriptEngine* engine,
    testing::suite& instance,
    bool generic = has_max_portability()
);

namespace testing
{
    class suite
    {
    public:
        suite() = delete;
        suite(const suite&) = delete;

        explicit suite(std::string name);

        virtual ~suite();

        virtual void expect_true(bool val);
        virtual void expect_false(bool val);

        [[nodiscard]]
        const std::string& get_name() const noexcept
        {
            return m_name;
        }

        void set_failed() noexcept;

        bool failed() const noexcept
        {
            return m_failed;
        }

        [[nodiscard]]
        virtual std::ostream& get_ostream() const;

    protected:
        virtual void write_message(std::string_view msg);

        static std::string format_current_loc(AS_NAMESPACE_QUALIFIER asIScriptContext* ctx);

    private:
        std::string m_name;
        bool m_failed;
    };
} // namespace testing
} // namespace asbind20::ext

#endif
