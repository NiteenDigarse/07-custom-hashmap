
#include <algorithm>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <iomanip>
#include <iostream>
#include <random>
#include <string>
#include <unordered_map>
#include <vector>

#include "custom-hashmap.hpp"

using Clock = std::chrono::steady_clock;
using Milliseconds = std::chrono::duration<double, std::milli>;

constexpr std::size_t ELEMENT_COUNT = 200000;
constexpr unsigned int RANDOM_SEED = 42;

using StandardMap = std::unordered_map<int, int>;
using CustomMap = CustomHashMap<int, int>;

std::vector<int> generate_keys(std::size_t count)
{
    std::vector<int> keys;
    keys.reserve(count);

    for (std::size_t i = 0; i < count; ++i)
    {
        keys.push_back(static_cast<int>(i));
    }

    // Shuffle keys for a more varied access order.
    std::mt19937 generator(RANDOM_SEED);
    std::shuffle(keys.begin(), keys.end(), generator);

    return keys;
}

template <typename Duration>
double to_milliseconds(Duration duration)
{
    return Milliseconds(duration).count();
}

// --------------------------------------------------
// INSERT BENCHMARK
// --------------------------------------------------

double benchmark_custom_insert(const std::vector<int>& keys)
{
    CustomMap map;

    const auto start = Clock::now();

    for (int key : keys)
    {
        map.insert(key, key * 2);
    }

    const auto finish = Clock::now();

    if (map.size() != keys.size())
    {
        std::cerr << "CustomHashMap insert validation failed\n";
    }

    return to_milliseconds(finish - start);
}

double benchmark_standard_insert(const std::vector<int>& keys)
{
    StandardMap map;

    const auto start = Clock::now();

    for (int key : keys)
    {
        map.emplace(key, key * 2);
    }

    const auto finish = Clock::now();

    if (map.size() != keys.size())
    {
        std::cerr << "std::unordered_map insert validation failed\n";
    }

    return to_milliseconds(finish - start);
}

// --------------------------------------------------
// FIND BENCHMARK
// Map construction happens outside the timed section.
// --------------------------------------------------

double benchmark_custom_find(
    const std::vector<int>& keys,
    std::uint64_t& checksum)
{
    CustomMap map;

    for (int key : keys)
    {
        map.insert(key, key * 2);
    }

    checksum = 0;

    const auto start = Clock::now();

    for (int key : keys)
    {
        const int* value = map.find(key);

        if (value != nullptr)
        {
            checksum += static_cast<std::uint64_t>(*value);
        }
    }

    const auto finish = Clock::now();

    return to_milliseconds(finish - start);
}

double benchmark_standard_find(
    const std::vector<int>& keys,
    std::uint64_t& checksum)
{
    StandardMap map;

    for (int key : keys)
    {
        map.emplace(key, key * 2);
    }

    checksum = 0;

    const auto start = Clock::now();

    for (int key : keys)
    {
        const auto it = map.find(key);

        if (it != map.end())
        {
            checksum += static_cast<std::uint64_t>(it->second);
        }
    }

    const auto finish = Clock::now();

    return to_milliseconds(finish - start);
}

// --------------------------------------------------
// ERASE BENCHMARK
// Map construction happens outside the timed section.
// --------------------------------------------------

double benchmark_custom_erase(const std::vector<int>& keys)
{
    CustomMap map;

    for (int key : keys)
    {
        map.insert(key, key * 2);
    }

    const auto start = Clock::now();

    for (int key : keys)
    {
        map.erase(key);
    }

    const auto finish = Clock::now();

    if (!map.empty())
    {
        std::cerr << "CustomHashMap erase validation failed\n";
    }

    return to_milliseconds(finish - start);
}

double benchmark_standard_erase(const std::vector<int>& keys)
{
    StandardMap map;

    for (int key : keys)
    {
        map.emplace(key, key * 2);
    }

    const auto start = Clock::now();

    for (int key : keys)
    {
        map.erase(key);
    }

    const auto finish = Clock::now();

    if (!map.empty())
    {
        std::cerr << "std::unordered_map erase validation failed\n";
    }

    return to_milliseconds(finish - start);
}

// --------------------------------------------------
// RESULT DISPLAY
// --------------------------------------------------

void print_result(
    const std::string& operation,
    double custom_ms,
    double standard_ms)
{
    std::cout << "\n" << operation << '\n';

    std::cout << "  CustomHashMap:       "
              << custom_ms << " ms\n";

    std::cout << "  std::unordered_map:  "
              << standard_ms << " ms\n";

    if (standard_ms > 0.0)
    {
        std::cout << "  Custom / Standard:   "
                  << custom_ms / standard_ms
                  << "x\n";
    }
}

int main()
{
    std::cout << std::fixed << std::setprecision(3);

    const std::vector<int> keys =
        generate_keys(ELEMENT_COUNT);

    std::cout << "========================================\n";
    std::cout << "       CustomHashMap Benchmark\n";
    std::cout << "========================================\n";

    std::cout << "Elements: " << ELEMENT_COUNT << '\n';
    std::cout << "Compiler standard: C++17 or newer\n";
    std::cout << "Key type: int\n";
    std::cout << "Value type: int\n";

    // Insert
    const double custom_insert =
        benchmark_custom_insert(keys);

    const double standard_insert =
        benchmark_standard_insert(keys);

    print_result(
        "INSERT",
        custom_insert,
        standard_insert);

    // Find
    std::uint64_t custom_checksum = 0;
    std::uint64_t standard_checksum = 0;

    const double custom_find =
        benchmark_custom_find(keys, custom_checksum);

    const double standard_find =
        benchmark_standard_find(keys, standard_checksum);

    print_result(
        "FIND",
        custom_find,
        standard_find);

    std::cout << "  Checksums match: "
              << std::boolalpha
              << (custom_checksum == standard_checksum)
              << '\n';

    // Erase
    const double custom_erase =
        benchmark_custom_erase(keys);

    const double standard_erase =
        benchmark_standard_erase(keys);

    print_result(
        "ERASE",
        custom_erase,
        standard_erase);

    std::cout << "\n========================================\n";
    std::cout << "Benchmark completed.\n";
    std::cout << "========================================\n";

    return 0;
}
