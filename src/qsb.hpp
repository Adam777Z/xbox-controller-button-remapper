#pragma once

#include <climits>
#include <type_traits>

namespace handy
{
	namespace util
	{
		template <typename T>
		constexpr unsigned int compute_bytes_needed_for_decimal_representation_of_type (T val)
		{
			static_assert(std::is_integral<T>::value, "Only integral types");
			return ((CHAR_BIT * sizeof(T) - 1) / 3 + 2);
		}

	}
}
