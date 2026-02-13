/**
 * @file validator.hpp
 * @author HenryAWE
 * @brief Validators for checking function signature at compile-time
 */

#ifndef ASBIND20_META_VALIDATOR_HPP
#define ASBIND20_META_VALIDATOR_HPP

#pragma once

#include <concepts>
#include <type_traits>
#include "../meta.hpp"

namespace asbind20::meta
{
namespace validator
{
    template <typename T, typename Base>
    concept ptrref_of =
        (std::is_pointer_v<std::decay_t<T>> || std::is_reference_v<T>) &&
        ((!std::is_pointer_v<T> || std::same_as<std::remove_cv_t<std::remove_pointer_t<T>>, Base>) ||
         (!std::is_reference_v<T> || std::same_as<std::remove_cvref_t<T>, Base>));
}
} // namespace asbind20::meta

#endif
