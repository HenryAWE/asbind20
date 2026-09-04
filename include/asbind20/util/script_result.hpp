#ifndef ASBIND20_UTIL_SCRIPT_RESULT_HPP
#define ASBIND20_UTIL_SCRIPT_RESULT_HPP

#include <memory>
#include <string>
#include "../fwd.hpp"
#include "../detail/err_handler.hpp"
#include "../io/to_string.hpp"

namespace asbind20
{
class bad_script_result_access : std::exception
{
public:
    [[nodiscard]]
    const char* what() const noexcept override
    {
        return "bad script result access";
    }
};

struct bad_script_result_t
{};

inline constexpr bad_script_result_t bad_script_result{};

enum class script_result_policy
{
    nonnegative,
    return_code
};

namespace detail
{
    template <script_result_policy Policy>
    struct script_result_policy_helper;

    template <>
    struct script_result_policy_helper<
        script_result_policy::nonnegative>
    {
        using error_type = int;

        static bool has_value(error_type e) noexcept
        {
            return e >= 0;
        }

        static std::string error_description(error_type e)
        {
            using std::to_string;
            return to_string(e);
        }

        static constexpr error_type bad_status =
            AS_NAMESPACE_QUALIFIER asERROR; // -1
        static constexpr error_type good_status =
            AS_NAMESPACE_QUALIFIER asSUCCESS; // 0
    };

    template <>
    struct script_result_policy_helper<
        script_result_policy::return_code>
    {
        using error_type = AS_NAMESPACE_QUALIFIER asERetCodes;

        static bool has_value(error_type e) noexcept
        {
            return static_cast<int>(e) >= 0;
        }

        static std::string error_description(error_type e)
        {
            // From IO module
            return to_string(e);
        }

        static constexpr error_type bad_status =
            AS_NAMESPACE_QUALIFIER asERROR;
        static constexpr error_type good_status =
            AS_NAMESPACE_QUALIFIER asSUCCESS;
    };
} // namespace detail

template <script_result_policy Policy = script_result_policy::nonnegative>
class script_result_base
{
    using helper = detail::script_result_policy_helper<Policy>;

public:
    using error_type = typename helper::error_type;

    class bad_access : public bad_script_result_access
    {
    public:
        using error_type = error_type;

        bad_access() = delete;
        constexpr bad_access(const bad_access&) noexcept = default;

        explicit constexpr bad_access(error_type status) noexcept
            : m_status(status) {}

        [[nodiscard]]
        constexpr error_type get_error() const noexcept
        {
            return m_status;
        }

        [[nodiscard]]
        std::string error_description() const
        {
            return helper::error_description(get_error());
        }

    private:
        error_type m_status;
    };

    static constexpr script_result_policy get_result_policy() noexcept
    {
        return Policy;
    }

protected:
    [[noreturn]]
    static void throw_bad_access(error_type status)
    {
        throw bad_access(status);
    }
};

template <
    typename T,
    script_result_policy Policy = script_result_policy::nonnegative>
class script_result : public script_result_base<Policy>
{
    using my_base = script_result_base<Policy>;
    using helper = detail::script_result_policy_helper<Policy>;

    static_assert(
        !std::same_as<std::remove_cv_t<T>, bad_script_result_t>,
        "invalid script result type"
    );

public:
    using value_type = T;
    using error_type = typename helper::error_type;

    using reference_type = value_type&;
    using const_reference_type = const value_type&;

    template <typename U>
    using rebind = script_result<U, Policy>;

    script_result(bad_script_result_t, error_type e) noexcept
    {
        // The error status shouldn't have value for this constructor
        if(helper::has_value(e)) [[unlikely]]
            e = helper::bad_status;
        m_status = e;
    }

    explicit script_result(const T& value, error_type status = helper::good_status)
        : script_result(
              std::piecewise_construct,
              std::forward_as_tuple(value),
              std::forward_as_tuple(status)
          )
    {}

    template <typename... Args1, typename Args2>
    script_result(
        std::piecewise_construct_t,
        std::tuple<Args1...> args,
        std::tuple<Args2> status
    )
    {
        [&]<std::size_t... Is>(std::index_sequence<Is...>)
        {
            this->emplace_value(std::get<Is>(args)...);
        }(std::make_index_sequence<sizeof...(Args1)>{});

        auto e = static_cast<error_type>(std::get<0>(status));
        if(!helper::has_value(e)) [[unlikely]]
            e = helper::good_status;
        m_status = e;
    }

    ~script_result()
    {
        destroy_impl();
    }

    bool has_value() const noexcept
    {
        return helper::has_value(m_status);
    }

    explicit operator bool() const noexcept
    {
        return has_value();
    }

    template <typename... Args>
    void emplace_value(Args&&... args)
    {
        destroy_impl();
        m_status = helper::bad_status;
        this->val_emplace_impl(std::forward<Args>(args)...);
        m_status = helper::good_status;
    }

    value_type& operator*() & noexcept
    {
        return m_value;
    }

    value_type&& operator*() && noexcept
    {
        return std::move(m_value);
    }

    const value_type& operator*() const& noexcept
    {
        return m_value;
    }

    const value_type* operator->() const noexcept
    {
        return std::addressof(m_value);
    }

    value_type& value()
    {
        if(!has_value()) [[unlikely]]
            my_base::throw_bad_access(m_status);
        return m_value;
    }

    const value_type& value() const
    {
        if(!has_value()) [[unlikely]]
            my_base::throw_bad_access(m_status);
        return m_value;
    }

    [[nodiscard]]
    error_type error() const noexcept
    {
        return m_status;
    }

    [[nodiscard]]
    std::string error_description() const
    {
        return helper::error_description(m_status);
    }

    template <std::convertible_to<value_type> U>
    value_type value_or(U&& alt)
    {
        return *this ? m_value : std::forward<U>(alt);
    }

private:
    union
    {
        value_type m_value;
    };

    error_type m_status = helper::bad_status;

    void destroy_impl()
    {
        std::destroy_at(std::addressof(m_value));
    }

    template <typename... Args>
    void val_emplace_impl(Args&&... args)
    {
        new(std::addressof(m_value)) value_type(std::forward<Args>(args)...);
    }
};

/**
 * @brief Script result for void type
 */
template <script_result_policy Policy>
class script_result<void, Policy> : public script_result_base<Policy>
{
    using my_base = script_result_base<Policy>;
    using helper = detail::script_result_policy_helper<Policy>;

public:
    using value_type = void;
    using error_type = typename helper::error_type;

    template <typename U>
    using rebind = script_result<U, Policy>;

    script_result() noexcept
        : m_status(helper::good_status)
    {}

    script_result(bad_script_result_t, error_type e) noexcept
    {
        // The error status shouldn't have value for this constructor
        if(helper::has_value(e)) [[unlikely]]
            e = helper::bad_status;
        m_status = e;
    }

    explicit script_result(error_type status) noexcept
        : m_status(status)
    {}

    bool has_value() const noexcept
    {
        return helper::has_value(m_status);
    }

    explicit operator bool() const noexcept
    {
        return has_value();
    }

    void emplace_value() noexcept
    {
        m_status = helper::good_status;
    }

    void operator*() const noexcept
    {
        ASBIND20_ASSERT(has_value());
    }

    void value() const
    {
        if(!has_value()) [[unlikely]]
            my_base::throw_bad_access(m_status);
    }

    [[nodiscard]]
    error_type error() const noexcept
    {
        return m_status;
    }

    [[nodiscard]]
    std::string error_description() const
    {
        return helper::error_description(m_status);
    }

private:
    error_type m_status = helper::good_status;
};
} // namespace asbind20

#endif
