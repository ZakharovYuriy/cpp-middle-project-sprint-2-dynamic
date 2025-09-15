#pragma once
#include <cstddef>
#include <expected>
#include <tuple>
#include <utility>

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

template <typename VecIn, typename VecOut, typename T,size_t position>
auto createExpectedVal(const VecIn& vecIn, const VecOut& vecOut){
    using namespace stdx::details;
    return parse_value_with_format<T>(vecIn[position],vecOut[position]);
}

template <typename VecIn, typename VecOut, typename T, size_t position>
auto createPureVal(const VecIn& vecIn, const VecOut& vecOut) {
    using namespace stdx::details;
    return parse_value_with_format<T>(vecIn[position], vecOut[position]).value();
}

// ---------- ВАЖНО: impl, который ДЕДУЦИРУЕТ Is... из Positions<Is...>
template <typename VecIn, typename VecOut, typename... Ts, std::size_t... Is>
auto makeFilledExpectedTuple_impl(VecIn&& vecIn, VecOut&& vecOut, Positions<Is...>) {
    return std::tuple{ createExpectedVal<VecIn, VecOut, Ts, Is>(std::forward<VecIn>(vecIn), std::forward<VecOut>(vecOut))... };
}

template <typename VecIn, typename VecOut, typename... Ts>
auto makeFilledExpectedTuple(VecIn&& vecIn, VecOut&& vecOut) {
    return makeFilledExpectedTuple_impl<VecIn, VecOut, Ts...>(std::forward<VecIn>(vecIn), std::forward<VecOut>(vecOut), CreatePositions<sizeof...(Ts)>{});
}

template <typename VecIn, typename VecOut, typename... Ts, std::size_t... Is>
auto makeFilledPureValTuple_impl(VecIn&& vecIn, VecOut&& vecOut, Positions<Is...>) {
    return std::tuple{ createPureVal<VecIn, VecOut, Ts, Is>(std::forward<VecIn>(vecIn), std::forward<VecOut>(vecOut))... };
}

template <typename VecIn, typename VecOut, typename... Ts>
auto makeFilledPureValTuple(VecIn&& vecIn, VecOut&& vecOut) {
    return makeFilledPureValTuple_impl<VecIn, VecOut, Ts...>(std::forward<VecIn>(vecIn), std::forward<VecOut>(vecOut), CreatePositions<sizeof...(Ts)>{});
}

template <typename... Ts>
std::expected<details::scan_result<Ts...>, details::scan_error> scan(std::string_view input, std::string_view format) {
    using namespace stdx::details;
    const auto& parsedData = parse_sources<Ts...>(input,format);
    if (!parsedData.has_value()) return std::unexpected(parsedData.error());
    const auto& [vecIn,vecOut] = parsedData.value();

    if (vecIn.size() != sizeof...(Ts)) return std::unexpected(details::scan_error{"The number of types does not match the number of placeholders"});

    auto expected_tuple = makeFilledExpectedTuple<decltype(vecIn),decltype(vecOut),Ts...>(std::forward<decltype(vecIn)>(vecIn), std::forward<decltype(vecIn)>(vecOut));
    bool allElementsAreExpected = true;
    std::apply([&allElementsAreExpected](auto&&... elems) {
        allElementsAreExpected = ((elems.has_value()) && ...); 
    }, expected_tuple);

    
    if (allElementsAreExpected){
        details::scan_result<Ts...> result{ makeFilledPureValTuple<decltype(vecIn),decltype(vecOut),Ts...>(std::forward<decltype(vecIn)>(vecIn), std::forward<decltype(vecIn)>(vecOut))};
        return result;
    }
    return std::unexpected(details::scan_error{"Some formats are wrong"});
}

} // namespace stdx
