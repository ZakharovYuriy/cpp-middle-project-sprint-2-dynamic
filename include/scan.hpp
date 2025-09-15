#pragma once
#include <cstddef>
#include <expected>
#include <tuple>

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
auto makeFilledExpectedTuple_impl(const VecIn& vecIn, const VecOut& vecOut, Positions<Is...>) {
    return std::tuple{ createExpectedVal<VecIn, VecOut, Ts, Is>(vecIn, vecOut)... };
}

template <typename VecIn, typename VecOut, typename... Ts>
auto makeFilledExpectedTuple(const VecIn& vecIn, const VecOut& vecOut) {
    return makeFilledExpectedTuple_impl<VecIn, VecOut, Ts...>(vecIn, vecOut, CreatePositions<sizeof...(Ts)>{});
}

template <typename VecIn, typename VecOut, typename... Ts, std::size_t... Is>
auto makeFilledPureValTuple_impl(const VecIn& vecIn, const VecOut& vecOut, Positions<Is...>) {
    return std::tuple{ createPureVal<VecIn, VecOut, Ts, Is>(vecIn, vecOut)... };
}

template <typename VecIn, typename VecOut, typename... Ts>
auto makeFilledPureValTuple(const VecIn& vecIn, const VecOut& vecOut) {
    return makeFilledPureValTuple_impl<VecIn, VecOut, Ts...>(vecIn, vecOut, CreatePositions<sizeof...(Ts)>{});
}

template <typename... Ts>
std::expected<details::scan_result<Ts...>, details::scan_error> scan(std::string_view input, std::string_view format) {
    using namespace stdx::details;
    const auto& parsedData = parse_sources<Ts...>(input,format);
    if (!parsedData.has_value()) return std::unexpected(parsedData.error());
    const auto& [vecIn,vecOut] = parsedData.value();

    auto expected_tuple = makeFilledExpectedTuple<decltype(vecIn),decltype(vecOut),Ts...>(vecIn, vecOut);
    bool allElementsAreExpected = true;
    std::apply([&allElementsAreExpected](auto&&... elems) {
        allElementsAreExpected = ((elems.has_value()) && ...); 
    }, expected_tuple);

    details::scan_result<Ts...> result;
    if (allElementsAreExpected){
        result.scannedValues = makeFilledPureValTuple<decltype(vecIn),decltype(vecOut),Ts...>(vecIn, vecOut);
        return result;
    }
    return std::unexpected(details::scan_error{"Dumb implementation"});
}

} // namespace stdx
