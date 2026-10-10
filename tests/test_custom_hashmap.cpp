
#include <cassert>
#include <iostream>
#include <stdexcept>
#include <string>
#include <utility>

#include "custom-hashmap.hpp"

struct ConstantHash
{
    std::size_t operator()(int) const noexcept
    {
        return 0;
    }
};

void test_operator_subscript()
{
    CustomHashMap<int, int> map;

    assert(map[10] == 0);
    assert(map.size() == 1);

    map[10] = 100;
    assert(map[10] == 100);
    assert(map.size() == 1);

    assert(map[20] == 0);
    assert(map.size() == 2);

    map[10] = 500;
    assert(map[10] == 500);

    std::cout << "operator[] tests passed\n";
}

void test_insert_find_contains()
{
    CustomHashMap<int, std::string> map;

    assert(map.empty());
    assert(map.insert(1, "one"));
    assert(map.insert(2, "two"));

    assert(!map.insert(1, "replacement"));
    assert(map.size() == 2);

    assert(map.contains(1));
    assert(!map.contains(99));

    std::string* result = map.find(1);

    assert(result != nullptr);
    assert(*result == "one");

    *result = "updated";
    assert(*map.find(1) == "updated");

    assert(map.find(99) == nullptr);

    std::cout << "insert/find/contains tests passed\n";
}

void test_collision_and_erase()
{
    CustomHashMap<int, int, ConstantHash> map(8);

    assert(map.insert(10, 100));
    assert(map.insert(20, 200));
    assert(map.insert(30, 300));

    assert(map.size() == 3);
    assert(*map.find(10) == 100);
    assert(*map.find(20) == 200);
    assert(*map.find(30) == 300);

    // Chain: 30 -> 20 -> 10

    assert(map.erase(20)); // Middle node
    assert(!map.contains(20));
    assert(map.contains(10));
    assert(map.contains(30));

    assert(map.erase(10)); // Last node
    assert(!map.contains(10));
    assert(map.contains(30));

    assert(map.erase(30)); // Head node
    assert(map.empty());

    assert(!map.erase(30));
    assert(!map.erase(999));

    std::cout << "collision/erase tests passed\n";
}

void test_clear_and_reuse()
{
    CustomHashMap<int, int> map;

    assert(map.insert(1, 10));
    assert(map.insert(2, 20));

    map.clear();

    assert(map.empty());
    assert(map.size() == 0);
    assert(!map.contains(1));

    map.clear();
    map.clear();

    assert(map.empty());

    assert(map.insert(3, 30));
    assert(map.size() == 1);
    assert(*map.find(3) == 30);

    std::cout << "clear/reuse tests passed\n";
}

void test_load_factor_and_rehash()
{
    CustomHashMap<int, int> map(2);

    const std::size_t initial_buckets = map.bucket_count();

    assert(map.insert(1, 10));
    assert(map.insert(2, 20));

    assert(map.bucket_count() > initial_buckets);
    assert(*map.find(1) == 10);
    assert(*map.find(2) == 20);

    assert(map.load_factor() <= map.max_load_factor());

    map.rehash(32);

    assert(map.bucket_count() >= 32);
    assert(*map.find(1) == 10);
    assert(*map.find(2) == 20);

    assert(map.insert(3, 30));
    assert(*map.find(3) == 30);

    map.max_load_factor(0.25f);

    assert(map.load_factor() <= map.max_load_factor());

    std::cout << "load-factor/rehash tests passed\n";
}

void test_copy_semantics()
{
    CustomHashMap<int, int> original;

    assert(original.insert(1, 100));
    assert(original.insert(2, 200));

    CustomHashMap<int, int> copied(original);

    assert(copied.size() == 2);
    assert(*copied.find(1) == 100);

    *copied.find(1) = 999;

    assert(*copied.find(1) == 999);
    assert(*original.find(1) == 100);

    CustomHashMap<int, int> assigned;
    assert(assigned.insert(99, 999));

    assigned = original;

    assert(assigned.size() == 2);
    assert(*assigned.find(2) == 200);
    assert(!assigned.contains(99));

    assigned = assigned;

    assert(assigned.size() == 2);
    assert(*assigned.find(1) == 100);

    std::cout << "copy semantics tests passed\n";
}

void test_move_semantics()
{
    CustomHashMap<int, int> original;

    assert(original.insert(1, 100));
    assert(original.insert(2, 200));

    CustomHashMap<int, int> moved(std::move(original));

    assert(moved.size() == 2);
    assert(*moved.find(1) == 100);
    assert(original.empty());

    // Reuse the moved-from map.
    assert(original.insert(7, 700));
    assert(*original.find(7) == 700);

    CustomHashMap<int, int> destination;
    assert(destination.insert(99, 999));

    destination = std::move(moved);

    assert(destination.size() == 2);
    assert(*destination.find(2) == 200);
    assert(moved.empty());

    std::cout << "move semantics tests passed\n";
}

void test_const_correctness()
{
    CustomHashMap<int, int> map;

    assert(map.insert(1, 100));

    const CustomHashMap<int, int>& const_map = map;

    const int* result = const_map.find(1);

    assert(result != nullptr);
    assert(*result == 100);
    assert(const_map.contains(1));
    assert(!const_map.contains(99));

    std::cout << "const correctness tests passed\n";
}

void test_invalid_load_factor()
{
    CustomHashMap<int, int> map;

    bool exception_caught = false;

    try
    {
        map.max_load_factor(0.0f);
    }
    catch (const std::invalid_argument&)
    {
        exception_caught = true;
    }

    assert(exception_caught);

    std::cout << "invalid load-factor test passed\n";
}

int main()
{
    test_operator_subscript();
    test_insert_find_contains();
    test_collision_and_erase();
    test_clear_and_reuse();
    test_load_factor_and_rehash();
    test_copy_semantics();
    test_move_semantics();
    test_const_correctness();
    test_invalid_load_factor();

    std::cout << "\nAll CustomHashMap tests passed!\n";

    return 0;
}
