#pragma once
#include <cstddef>
#include <expected>
#include <tuple>
#include <utility>
#include <functional>

#include "parse.hpp"
#include "types.hpp"

namespace stdx {

template <size_t ...PositionNums>
struct Positions {};

template <size_t Num, size_t ...PositionNumsCollector>
struct PositionCounter : PositionCounter<Num-1,Num-1,PositionNumsCollector...> {};

template <size_t ...PositionNumsCollector>
struct PositionCounter<0,PositionNumsCollector...> {
    using positions = Positions<PositionNumsCollector...>;
};

template <size_t N>
using CreatePositions = typename PositionCounter<N>::positions;

template <bool Unwrap, typename VecIn, typename VecFmt, typename T, std::size_t Position>
auto createVal(const VecIn& vecIn, const VecFmt& vecFmt) {
    using namespace stdx::details;
    if constexpr (Unwrap) {
        return parse_value_with_format<T>(vecIn[Position], vecFmt[Position]).value();
    } else {
        return parse_value_with_format<T>(vecIn[Position], vecFmt[Position]);
    }
}

template <bool Unwrap, typename VecIn, typename VecFmt, typename... Ts, std::size_t... Is>
auto makeFilledTuple_impl(VecIn&& vecIn, VecFmt&& vecFmt, std::index_sequence<Is...>) {
    return std::tuple{
        createVal<Unwrap, VecIn, VecFmt, Ts, Is>(
            std::forward<VecIn>(vecIn), std::forward<VecFmt>(vecFmt)
        )...
    };
}

template <typename VecIn, typename VecFmt, typename... Ts>
auto makeFilledExpectedTuple(VecIn&& vecIn, VecFmt&& vecFmt) {
    return makeFilledTuple_impl<false, VecIn, VecFmt, Ts...>(
        std::forward<VecIn>(vecIn), std::forward<VecFmt>(vecFmt),
        std::make_index_sequence<sizeof...(Ts)>{}
    );
}

template <typename VecIn, typename VecFmt, typename... Ts>
auto makeFilledPureValTuple(VecIn&& vecIn, VecFmt&& vecFmt) {
    return makeFilledTuple_impl<true, VecIn, VecFmt, Ts...>(
        std::forward<VecIn>(vecIn), std::forward<VecFmt>(vecFmt),
        std::make_index_sequence<sizeof...(Ts)>{}
    );
}

template <typename... Ts>
std::expected<details::scan_result<Ts...>, details::scan_error> scan(std::string_view input, std::string_view format) {
    using namespace stdx::details;
    const auto& parsedData = parse_sources<Ts...>(input,format);
    if (!parsedData.has_value()) return std::unexpected(parsedData.error());
    const auto& [vecIn,vecFmt] = parsedData.value();

    if (vecIn.size() != sizeof...(Ts)) return std::unexpected(details::scan_error{"The number of types does not match the number of placeholders"});

    auto expected_tuple = makeFilledExpectedTuple<decltype(vecIn),decltype(vecFmt),Ts...>(std::forward<decltype(vecIn)>(vecIn), std::forward<decltype(vecIn)>(vecFmt));
    std::vector<scan_error> errors;
    std::apply([&errors](auto&&... elems) {
        ([&errors](auto&& elem){if (!elem.has_value()) errors.push_back(elem.error());}(elems), ...); 
    }, expected_tuple);

    
    if (errors.empty()){
        details::scan_result<Ts...> result{ makeFilledPureValTuple<decltype(vecIn),decltype(vecFmt),Ts...>(std::forward<decltype(vecIn)>(vecIn), std::forward<decltype(vecIn)>(vecFmt))};
        return result;
    }
    std::string message = "Some formats errors:'\n'";
    for (const auto& error : errors){
        message += error.message + '\n';
    }
    return std::unexpected(details::scan_error{message});
}

} // namespace stdx
