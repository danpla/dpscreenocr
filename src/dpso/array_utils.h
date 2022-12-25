
#pragma once

#include <array>
#include <cstddef>
#include <utility>


namespace dpso {


template <class T, std::size_t N, std::size_t... I>
constexpr std::array<T, N>
    makeArrayHelper(T (&&a)[N], std::index_sequence<I...>)
{
    return {std::move(a[I])...};
}


/**
 * Make an array safely.
 *
 * The problem with aggregate array initialization is that it allows
 * you to provide fewer values than specified by the array type, and
 * compilers don't even warn about this case. This is especially
 * dangerous when the array size is defined via a constant, which one
 * day may become N times larger.
 *
 * This function solves the problem by forcing the user to explicitly
 * provide all array elements (and hence requires an explicit array
 * size). As a bonus, it also guards against missing commas in arrays
 * initialized by a list of string literals, e.g.:
 *
 * const char* array[3] = {
 *     "strA",
 *     "strB"   // <-
 *     "strC"
 * };
 *
 * T is a type of array element; Size is the required size of the
 * array. N is the deduced size of the array passed to the function;
 * it should not be set manually. makeArray() gives a compile-time
 * error if Size != N.
 */
template<typename T, std::size_t Size, std::size_t N>
constexpr std::array<T, N> makeArray(T (&&a)[N])
{
    // An alternative way to implement this function is to use a
    // parameter pack like:
    //
    // ... makeArray(Args&&... args)
    //    return {std::forward<Args>(args)...};
    //
    // This approach requires less code, doesn't expose the real array
    // size (N) as a template argument, and prevents fooling the
    // function by std::move-ing an already constructed array.
    //
    // However, there is a catch: the parameter pack expansion is not
    // a constant expression, which in combination with curly braces
    // gives more narrowing conversion warnings compared to an
    // analogous aggregate initialization:
    //
    // // Ok, compiler knows there's no narrowing:
    // unsigned array1[1] = {1};
    // // Narrowing conversion from int to unsigned:
    // auto array2 = makeArray<unsigned, 1>(1);
    //
    // The narrowing conversion warnings is a good thing in general,
    // we are certainly not going to cast std::forward to T as this
    // will disable them altogether. All we need is not to hide
    // constant expressions from the compiler.
    static_assert(Size == N, "Invalid number of elements");
    return makeArrayHelper(
        std::move(a), std::make_index_sequence<N>{});
}


}
