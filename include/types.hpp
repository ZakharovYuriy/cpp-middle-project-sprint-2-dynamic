#pragma once

#include <expected>
#include <charconv>
#include <cstdint>
#include <tuple>
#include <string>

namespace stdx::details {

// Класс для хранения ошибки неуспешного сканирования

struct scan_error {
    std::string message;
};

// Шаблонный класс для хранения результатов успешного сканирования

template <typename... Ts>
struct scan_result {
    std::tuple<Ts ...> scannedValues;
};

constexpr char dType[] = "%d";
constexpr char sType[] = "%s";
constexpr char uType[] = "%u";
constexpr char fType[] = "%f";

template <typename T, typename ...Ts>
consteval bool isSupportedType(){
    return ((std::is_same_v<T, Ts> || std::is_same_v<T, const Ts>) || ...);
}

template <typename T>
concept SupportedDType = isSupportedType<T,int8_t,int16_t,int32_t,int64_t>();

template <typename T>
concept SupportedSType = isSupportedType<T,std::string,std::string_view>();

template <typename T>
concept SupportedUType = isSupportedType<T,uint8_t,uint16_t,uint32_t,uint64_t>();

template <typename T>
concept SupportedFType = isSupportedType<T,float,double>();

template <typename T>
concept AnySupportedType = SupportedDType<T> || SupportedSType<T> || SupportedUType<T> || SupportedFType<T>;

} // namespace stdx::details
