#pragma once
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

} // namespace stdx::details
