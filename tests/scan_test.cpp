#include <gtest/gtest.h>
#include <print>
#include <string>
#include <type_traits>

#include "scan.hpp"
#include "types.hpp"

using namespace std::literals;
using namespace stdx::details;
using namespace stdx;

TEST(ScanTest, SimpleTest) {
    auto result = stdx::scan<std::string>("number", "{}");
    ASSERT_TRUE(result.has_value());
    const auto& [number] = result->scannedValues;
    EXPECT_EQ(number, "number");
}

template<typename T1>
void compareVals (const T1& val1, const T1& val2)
{
    EXPECT_EQ(val1, val2);
}

template<typename T>
requires (std::is_same_v<T, float>)
void compareVals (const T& val1, const T& val2)
{
    EXPECT_FLOAT_EQ(val1, val2);
}

template<typename T>
requires (std::is_same_v<T, double>)
void compareVals (const T& val1, const T& val2)
{
    EXPECT_DOUBLE_EQ(val1, val2);
}

template<typename T1, typename T2>
void doublePlaceholderCheck (const std::string& input, const std::string& format, T1 ref1, T2 ref2)
{
    auto result = stdx::scan<T1,T2>(input, format);
    ASSERT_TRUE(result.has_value());
    const auto& [val1,val2] = result->scannedValues;
    compareVals(val1, ref1);
    compareVals(val2, ref2);
}

template<typename T>
void TypedAndGenericPlaceholderCheck (const std::string& input, const std::string& format, T ref)
{
    doublePlaceholderCheck<T,T>(input,format,ref,ref);
}

// Обёртка, которая прогоняет проверки для базового типа и всех cv-вариантов.
template <typename T, typename... Args>
void checkWithCV(Args&&... args) {
    // базовый
    TypedAndGenericPlaceholderCheck<T>(std::forward<Args>(args)...);
    // const
    TypedAndGenericPlaceholderCheck<const T>(std::forward<Args>(args)...);
}

constexpr std::string& repeatTwice (const std::string& word, std::string& modifiedWord){
    modifiedWord = std::format("{} {}", word, word );
    return modifiedWord;
};

// Форматирующая строка должна поддерживать следующие conversion specifiers: 
// d — в исходной строке на месте плейсхолдера находится целое число;
// s — в исходной строке на месте плейсхолдера находится строка;
// u — в исходной строке на месте плейсхолдера находится натуральное число;
// f — в исходной строке на месте плейсхолдера находится число с плавающей точкой.

// ---------- УСПЕШНЫЕ СЦЕНАРИИ ----------

// Корректный разбор: формат "ID={}"  +   вход "ID=42"
TEST(ParseSourcesTest, SimpleMatch) {
    std::string_view fmt   = "ID={}";
    std::string_view line  = "ID=42";

    auto result = parse_sources<>(line, fmt);
    ASSERT_TRUE(result.has_value()) << "parse_sources вернул ошибку";

    const auto& [input_parts,format_parts] = *result;

    ASSERT_EQ(format_parts.size(), 1u);
    EXPECT_EQ(format_parts[0], "");         // пустой плейсхолдер

    ASSERT_EQ(input_parts.size(), 1u);
    EXPECT_EQ(input_parts[0], "42");        // число между литералами
}

TEST(ParseSourcesTest, MultiplePlaceholdersWithLiterals) {
    std::string_view fmt = "user: {%s}, age: {%d}, score: {%f}";
    std::string_view line = "user: alice, age: 30, score: 99.5";

    auto r = parse_sources<>(line, fmt);
    ASSERT_TRUE(r.has_value()) << "Expected successful parse";

    const auto& [input_parts, format_parts] = *r;

    ASSERT_EQ(format_parts.size(), 3u);  // 3 плейсхолдера
    EXPECT_EQ(format_parts[0], "%s");
    EXPECT_EQ(format_parts[1], "%d");
    EXPECT_EQ(format_parts[2], "%f");

    ASSERT_EQ(input_parts.size(), 3u);   // 3 значения в строке
    EXPECT_EQ(input_parts[0], "alice");
    EXPECT_EQ(input_parts[1], "30");
    EXPECT_EQ(input_parts[2], "99.5");
}

TEST(ScanTest, DefaultTest) {
    auto result = stdx::scan<int,float>("I want to sum 42 and 3.14 numbers.", "I want to sum {} and {%f} numbers.");
    ASSERT_TRUE(result.has_value());
    const auto& [val1,val2] = result->scannedValues;
    EXPECT_EQ(val1, 42);
    EXPECT_FLOAT_EQ(val2, 3.14f);
}

// Проверка поддержки числовых типов
//  int8_t, int16_t, int32_t, int64_t, 
//  uint8_t, uint16_t, uint32_t, uint64_t, 
//  float, double
//
// а также в cv-квалифицированные версии этих типов.
TEST(ScanTest, NumTypesTest) {
    std::string testVal;
    checkWithCV<int8_t>(repeatTwice("-1",testVal),"{%d} {}",-1);
    checkWithCV<int16_t>(repeatTwice("-1",testVal),"{%d} {}",-1);
    checkWithCV<int32_t>(repeatTwice("-1",testVal),"{%d} {}",-1);
    checkWithCV<int64_t>(repeatTwice("-1",testVal),"{%d} {}",-1);

    checkWithCV<uint8_t>(repeatTwice("1",testVal),"{%u} {}",1);
    checkWithCV<uint16_t>(repeatTwice("1",testVal),"{%u} {}",1);
    checkWithCV<uint32_t>(repeatTwice("1",testVal),"{%u} {}",1);
    checkWithCV<uint64_t>(repeatTwice("1",testVal),"{%u} {}",1);

    checkWithCV<float>(repeatTwice("0.85",testVal),"{%f} {}",0.85);
    checkWithCV<double>(repeatTwice("0.85",testVal),"{%f} {}",0.85);
}

// Проверка поддержки строковых типов
//  std::string_view и std::string 
//
// а также в cv-квалифицированные версии этих типов.
TEST(ScanTest, StringTypesTest) {
    const std::string strWord = "HelloWorld"s;
    std::string testVal;
    std::string line = repeatTwice(strWord,testVal);
    std::string_view strViewWord (strWord);
    checkWithCV<std::string_view>(line,"{%s} {}",strViewWord);
    checkWithCV<std::string>(line,"{%s} {}",strWord);
}

// ---------- ОШИБКИ/ОТКАЗЫ ----------

// Расхождение литералов: формат "X={}"  +  вход "Y=10"  →  ошибка
TEST(ParseSourcesTest, LiteralMismatch) {
    auto result = parse_sources<>("Y=10", "X={}");
    ASSERT_FALSE(result.has_value());
}

TEST(ScanTest, TypesMismatch) 
{
    const std::string strWord = "HelloWorld"s;
    std::string testVal;
    std::string line = repeatTwice(strWord,testVal);
    auto result = stdx::scan<int,int>(line,"{%s} {}");
    ASSERT_FALSE(result.has_value());
}

TEST(ScanTest, PlaceholderCount_TooFewTypes) {
    auto result = stdx::scan<int>("10 20", "{} {}");  // типов меньше чем плейсхолдеров
    ASSERT_FALSE(result.has_value());
}

TEST(ScanTest, PlaceholderCount_TooManyTypes) {
    auto result = stdx::scan<int, int>("10", "{}");   // типов больше чем плейсхолдеров
    ASSERT_FALSE(result.has_value());
}

TEST(ScanTest, Specifier_TypeIncompatibility_FloatIntoInt) {
    auto result = stdx::scan<int>("3.14", "{%f}");    // %f несовместим с int
    ASSERT_FALSE(result.has_value());
}

TEST(ScanTest, ParseError_UnsignedButNegative) {
    auto result = stdx::scan<uint32_t>("-1", "{%u}");
    ASSERT_FALSE(result.has_value());
}

TEST(ScanTest, RangeError_Uint8Overflow) {
    auto result = stdx::scan<uint8_t>("300", "{%u}");
    ASSERT_FALSE(result.has_value());
}

TEST(ScanTest, RangeError_Int8Underflow) {
    auto result = stdx::scan<int8_t>("-200", "{%d}");
    ASSERT_FALSE(result.has_value());
}

TEST(ScanTest, TrailingDataNotConsumed) {
    auto result = stdx::scan<int>("42zz", "{}");      // 'zz' не описано форматом
    ASSERT_FALSE(result.has_value());

    auto result2 = stdx::scan<int>("42zz", "{}zz");   // теперь описано
    ASSERT_TRUE(result2.has_value());
    auto [v] = result2->scannedValues;
    EXPECT_EQ(v, 42);
}

TEST(ScanTest, BadSpecifierRejected) {
    auto result = stdx::scan<int>("10", "{%x}");      // %x не поддержан
    ASSERT_FALSE(result.has_value());
}

TEST(ScanTest, UnsupportedTypes_RuntimeError) {
    // bool не входит в список поддержанных
    {
        auto result = stdx::scan<bool>("1", "{}");
        ASSERT_FALSE(result.has_value());
    }

    // long double тоже не входит
    {
        auto result = stdx::scan<long double>("3.14", "{%f}");
        ASSERT_FALSE(result.has_value());
    }
}