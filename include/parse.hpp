#pragma once

#include <expected>
#include <string_view>
#include <utility>
#include <vector>

#include "types.hpp"

namespace stdx::details {

template <typename T>
requires (!AnySupportedType<T> || std::is_reference_v<T>)
std::expected<T, scan_error> parse(std::string_view input){
    return std::unexpected(scan_error{"Unexpected Type"});
}

template <typename T>
requires (SupportedDType<T> || SupportedUType<T> || SupportedFType<T>) && (!std::is_reference_v<T>)
std::expected<T, scan_error> parse(std::string_view input){
    using T2 = std::remove_const<T>::type;
    T2 value{};
    auto [ptr, ec] = std::from_chars(input.data(), input.data() + input.size(), value);

    if (ec == std::errc::invalid_argument)
        return std::unexpected(scan_error{"Invalid Type: not a number"});
    if (ec == std::errc::result_out_of_range)
        return std::unexpected(scan_error{"Invalid Type: out of range"});
    if (ptr != input.data() + input.size())
        return std::unexpected(scan_error{"Invalid Type: extra characters"});
    
    return T{value};
}

template <typename T>
requires (SupportedSType<T>) && (!std::is_reference_v<T>)
std::expected<T, scan_error> parse(std::string_view input){
        return T{input};
}

// Функция для парсинга значения с учетом спецификатора формата
template <typename T>
std::expected<T, scan_error> parse_value_with_format(std::string_view input, std::string_view fmt) 
{
    bool isRightDFormat = SupportedDType<T> && (fmt == dType);
    bool isRightSFormat = SupportedSType<T> && (fmt == sType);
    bool isRightUFormat = SupportedUType<T> && (fmt == uType);
    bool isRightFFormat = SupportedFType<T> && (fmt == fType);
    bool isAnyFormat = fmt.empty();

    if (isRightDFormat || isRightUFormat || isRightFFormat || isRightSFormat || isAnyFormat) {
        return parse<T>(input);
    }else{
        return std::unexpected(scan_error{"The type and format do not match"});
    }
}

// Функция для проверки корректности входных данных и выделения из обеих строк интересующих данных для парсинга
template <typename... Ts>
std::expected<std::pair<std::vector<std::string_view>, std::vector<std::string_view>>, scan_error>
parse_sources(std::string_view input, std::string_view format) {
    std::vector<std::string_view> format_parts;  // Части формата между {}
    std::vector<std::string_view> input_parts;
    size_t start = 0;
    while (true) {
        size_t open = format.find('{', start);
        if (open == std::string_view::npos) {
            break;
        }
        size_t close = format.find('}', open);
        if (close == std::string_view::npos) {
            break;
        }

        // Если между предыдущей } и текущей { есть текст,
        // проверяем его наличие во входной строке
        if (open > start) {
            std::string_view between = format.substr(start, open - start);
            auto pos = input.find(between);
            if (input.size() < between.size() || pos == std::string_view::npos) {
                return std::unexpected(scan_error{"Unformatted text in input and format string are different"});
            }
            if (start != 0) {
                input_parts.emplace_back(input.substr(0, pos));
            }

            input = input.substr(pos + between.size());
        }

        // Сохраняем спецификатор формата (то, что между {})
        format_parts.push_back(format.substr(open + 1, close - open - 1));
        start = close + 1;
    }

    // Проверяем оставшийся текст после последней }
    if (start < format.size()) {
        std::string_view remaining_format = format.substr(start);
        auto pos = input.find(remaining_format);
        if (input.size() < remaining_format.size() || pos == std::string_view::npos) {
            return std::unexpected(scan_error{"Unformatted text in input and format string are different"});
        }
        input_parts.emplace_back(input.substr(0, pos));
        input = input.substr(pos + remaining_format.size());
    } else {
        input_parts.emplace_back(input);
    }
    return std::pair{input_parts, format_parts};
}

} // namespace stdx::details