CustomHashMap --- Detailed Learning Notes
Project: C++ Engineering Projects --- Project #07
Topic: Custom Hash Map using Separate Chaining
Language Standard: C++17
Purpose: Implementation notes, design decisions, complexity
analysis, testing, and interview revision.

1. Project Overview
CustomHashMap is an educational implementation of a key-value
container inspired by std::unordered_map.
A hash map stores data as key-value pairs. It uses a hash function to
map a key to a bucket index. Since different keys can map to the same
bucket, the implementation must handle collisions.
This project uses:
- A dynamically allocated bucket array.
- A singly linked list for each bucket.
- A hash function to calculate bucket indices.
- Dynamic memory management for entries.
- Rehashing when the load factor threshold is exceeded.
- Copy and move operations to manage owned resources.
- Tests and benchmarks to evaluate correctness and performance.
The project is intended for learning and experimentation. It is not a
complete replacement for std::unordered_map.
2. Learning Objectives
By completing this project, the learner practices:
1. Class templates and generic programming.
2. Hash functions and bucket indexing.
3. Separate chaining for collision resolution.
4. Singly linked list traversal and deletion.
5. Dynamic memory allocation and deallocation.
6. Pointer-to-pointer concepts through Entry** buckets_.
7. operator[] and reference returns.
8. Load factor and rehashing.
9. Copy and move semantics (Rule of Five).
10. Const correctness.
11. Exception-aware resource management.
12. Unit-style testing with assert().
13. Benchmarking with std::chrono.
14. CMake project organization.
3. Project Structure
07-custom-hashmap/
├── include/
│   └── custom-hashmap.hpp
├── tests/
│   └── test_custom_hashmap.cpp
├── benchmarks/
│   └── benchmark_custom_hashmap.cpp
├── docs/
│   └── CustomHashMap_Notes.md
├── CMakeLists.txt
├── README.md
└── build/
Responsibilities
  Path                                        Responsibility
  include/custom-hashmap.hpp                Template implementation
  tests/test_custom_hashmap.cpp             Functional and edge-case tests
  benchmarks/benchmark_custom_hashmap.cpp   Timing comparison against
                                              std::unordered_map
  docs/CustomHashMap_Notes.md               Detailed learning notes
  CMakeLists.txt                            Build targets and compiler
                                              configuration
  build/                                    Generated build output; normally
                                          not committed
4. Core Data Structure
4.1 Entry Node
The map's internal node stores one key-value pair and a pointer to the
next node.
struct Entry
{
    Key key;
    Value value;
    Entry* next = nullptr;
};
- key: identifies the entry.
- value: the data associated with the key.
- next: links to another entry in the same bucket.
- nullptr means the node is currently the last node in its chain.
The Entry type is private because callers interact with the map
through its public API rather than directly manipulating nodes.
4.2 Bucket Array
std::size_t size_ = 0;
std::size_t bucket_count_ = 8;
Entry** buckets_ = nullptr;
- size_ is the number of stored entries.
- bucket_count_ is the number of buckets.
- buckets_ points to a dynamically allocated array of Entry*.
The type Entry** means that buckets_ points to an array whose
elements are pointers to Entry nodes.
Conceptually:
buckets_
   |
   v
+----------+----------+----------+----------+
| Entry*   | Entry*   | Entry*   | Entry*   | ... 8 slots initially
+----|-----+----------+----|-----+----------+
     |                    |
     v                    v
   Entry                 Entry
     |                    |
     v                    v
   Entry                nullptr
     |
     v
  nullptr
Each bucket points to the head of a linked list, or to nullptr when
empty.
4.3 Why Is Entry** Used?
The map needs an array of bucket-head pointers. Each bucket head has
type Entry*, so the pointer to the first element of the array has type
Entry**.
Allocation:
buckets_ = new Entry*[bucket_count_]{};
The trailing {} value-initializes the array's pointer elements, making
each bucket head null.
Deallocation must match the array allocation:
delete[] buckets_;
Individual nodes are separately allocated and must be deleted
individually using delete.
5. Hashing and Bucket Indexing
5.1 Hash Function
A hash function maps a key to a hash value.
The default implementation uses:
std::hash<Key>{}(key)
For the advanced template version, the hash policy is represented by:
template <
    typename Key,
    typename Value,
    typename Hash = std::hash<Key>,
    typename KeyEqual = std::equal_to<Key>
>
class CustomHashMap;
- Hash calculates hash values.
- KeyEqual compares keys.
- Default template arguments allow common key types to use standard
  hash and equality policies.
5.2 Bucket Index
A hash value can be much larger than the number of buckets. Modulo
converts it into an index within the bucket array.
index = hash(key) % bucket_count;
Example:
hash(key) = 27
bucket_count = 8

index = 27 % 8
index = 3
The key belongs in bucket index 3.
Important: bucket_count must be nonzero before using modulo.
Otherwise, modulo by zero is undefined behavior.
5.3 Why Rehashing Needs a Bucket Count Parameter
The simpler helper uses the current bucket count:
std::size_t bucket_index(const Key& key) const
{
    return hash_key(key) % bucket_count_;
}
Rehashing must calculate indices using the new bucket count before the
map switches to it. An overload supports that:
std::size_t bucket_index(
    const Key& key,
    std::size_t count) const
{
    return hash_key(key) % count;
}
This helper is useful when old entries are redistributed into a new
array.
6. Collision Handling: Separate Chaining
6.1 What Is a Collision?
A collision happens when two distinct keys map to the same bucket index.
For example, if the bucket count is eight, two different keys might both
produce index 3.
A collision does not mean the keys are equal. It only means their
calculated bucket indices are equal.
6.2 Separate Chaining
This implementation handles collisions using a linked list at each
bucket.
Bucket 3
   |
   v
[key: 19, value: 190] -> [key: 11, value: 110] -> nullptr
Each entry is still identified by its actual key. During lookup, the
implementation traverses the chain and compares keys.
6.3 Insertion at the Head
A new entry can be linked at the beginning of a bucket's chain:
Entry* new_entry = new Entry{key, value, buckets_[index]};
buckets_[index] = new_entry;
The new node's next pointer receives the old head. The bucket head
then points to the new node.
For example:
Before:  A -> B -> nullptr
Insert C
After:   C -> A -> B -> nullptr
The link update itself is constant-time. However, duplicate-key
detection first traverses the existing chain.
6.4 Duplicate Keys
The implementation rejects duplicate keys:
if (equal_(current->key, key))
{
    return false;
}
If the key already exists, the map does not insert another entry and
does not increase size_.
7. Core API Operations
7.1 insert()
Purpose: add a new key-value pair if the key is absent.
Typical flow:
1. Ensure bucket storage exists.
2. Calculate the bucket index.
3. Traverse the chain to detect duplicates.
4. Check whether insertion would exceed the load-factor limit.
5. Rehash if required.
6. Allocate and link the new node.
7. Increment size_.
8. Return true.
Return values:
- true: a new entry was inserted.
- false: the key already existed.
A failed allocation may throw an exception; a boolean return does not
mean all possible failures are represented by false.
7.2 find()
Purpose: locate a key and return a pointer to its stored value.
Value* find(const Key& key);
const Value* find(const Key& key) const;
Typical flow:
1. Calculate the bucket index.
2. Start at the bucket head.
3. Compare each node's key with the requested key.
4. Return the address of the stored value on a match.
5. Return nullptr if the key is absent.
Example:
if (int* value = map.find(10))
{
    std::cout << *value << '\n';
}
The returned pointer refers to the actual value stored inside the map.
Modifying *value changes that stored value.
Pointer validity: Erasing the corresponding entry invalidates a
pointer to its value. Rehashing in this implementation relinks existing
nodes, so pointers to values in surviving nodes remain valid, although
callers should still follow the documented API guarantees of their own
implementation.
7.3 contains()
Purpose: check whether a key exists.
bool contains(const Key& key) const
{
    return find(key) != nullptr;
}
It uses find() rather than implementing another traversal.
7.4 operator[]
Purpose: access a value by key, inserting a default-constructed value if
the key does not exist.
Value& operator[](const Key& key)
{
    Value* result = find(key);

    if (result != nullptr)
    {
        return *result;
    }

    insert(key, Value{});

    result = find(key);
    return *result;
}
Example:
CustomHashMap<int, int> map;

map[10] = 100;
std::cout << map[10]; // 100

std::cout << map[20]; // 0 for int
Why does map[20] return zero?
Value{} value-initializes an int to zero. For a class type, it
constructs a default value according to that type's initialization
rules.
operator[] returns Value&, so assignment through the result modifies
the stored value:
map[10] = 500;
This implementation requires Value to be default-constructible for
operator[].
7.5 erase()
Purpose: remove an entry by key.
The implementation maintains two pointers:
Entry* current = buckets_[index];
Entry* previous = nullptr;
There are three deletion cases.
Case 1: Delete the head
buckets_[index] = current->next;
The bucket head is updated to the second node.
Case 2: Delete a middle node
previous->next = current->next;
The previous node bypasses the deleted node.
Case 3: Delete the last node
The same previous->next assignment points to nullptr, because the
last node's next is null.
Before deleting a node, its neighbors' links must be updated so the
remaining chain stays connected.
After successful deletion:
delete current;
--size_;
return true;
If no key matches, erase() returns false.
7.6 clear()
Purpose: delete all entries while keeping the bucket array allocated.
For each bucket:
1. Start at its head.
2. Save current->next.
3. Delete the current node.
4. Move to the saved next node.
5. Set the bucket head to nullptr.
Finally:
size_ = 0;
Saving next before deleting current is essential. Accessing
current->next after deleting current would access a destroyed
object.
clear() differs from destruction: it retains the bucket array so that
new entries can be inserted afterward.
7.7 size() and empty()
std::size_t size() const noexcept
{
    return size_;
}

bool empty() const noexcept
{
    return size_ == 0;
}
Both operations are constant-time because the entry count is maintained
as part of the map's state.
8. Load Factor and Rehashing
8.1 Load Factor
The load factor is:
`\frac{\text{number of entries}}{\text{number of buckets}}`{=tex} \]

For example:

``` text
Entries: 6
Buckets: 8

Load factor = 6 / 8 = 0.75
```

The maximum load factor controls when automatic growth is triggered. It
is not a guarantee that every bucket has a small chain; a poor hash
function can still produce collisions.

### 8.2 Automatic Rehashing

Before inserting a new entry, the map calculates the load factor that
would result from the insertion.

If the projected value exceeds `max_load_factor_`, the map grows the
bucket array and redistributes entries.

The insertion must calculate the new key's bucket index again after
rehashing because the bucket count has changed.

### 8.3 Manual Rehashing

``` cpp
map.rehash(32);
```

The map chooses a bucket count large enough to satisfy its current
load-factor requirement. Therefore, the resulting bucket count may be
larger than the requested count.

### 8.4 Rehashing Existing Nodes

The advanced implementation uses a temporary bucket array and records
the destination index for each existing node before relinking nodes.

This design aims to avoid modifying the old chains until allocation and
hash calculations have succeeded.

However, exception-safety claims should be validated carefully for all
operations and custom hash/equality policies. A complex raw-pointer
container needs more than passing ordinary functional tests to prove
complete exception safety.

### 8.5 Rehashing Complexity

Rehashing processes the existing entries and the bucket arrays. Its work
is generally proportional to the number of entries plus the old and new
bucket counts, assuming hash computation itself is constant-time.

A rehash can make one insertion expensive even though insertion is
average-case constant-time over a sequence of operations.

------------------------------------------------------------------------

## 9. Resource Management and Rule of Five

The map owns dynamic resources:

-   `buckets_`: an array allocated with `new[]`.
-   Each `Entry`: an individual node allocated with `new`.

A class that owns raw resources must define copying and moving
carefully.

### 9.1 Destructor

``` cpp
~CustomHashMap()
{
    clear();
    delete[] buckets_;
}
```

`clear()` deletes every node. `delete[]` then releases the bucket array
itself.

The nodes and the bucket array are separate allocations and require
separate cleanup.

### 9.2 Why Default Copying Is Unsafe

A compiler-generated copy of a raw pointer copies its address, not the
allocation it points to.

If two maps share the same `buckets_` pointer, both destructors may
attempt to release the same memory. This can cause double deletion and
undefined behavior.

### 9.3 Copy Constructor

A deep-copy constructor creates a separate bucket array and creates
independent entries for the copied map.

Expected property:

``` cpp
CustomHashMap<int, int> original;
original.insert(1, 100);

CustomHashMap<int, int> copy(original);
*copy.find(1) = 999;

// original still stores 100
```

### 9.4 Copy Assignment

Copy assignment must handle:

-   Existing destination resources.
-   Self-assignment.
-   Allocation failure during copying.

The copy-and-swap pattern first creates a temporary copy, then swaps
resources. The temporary later destroys the destination's old resources.

### 9.5 Move Constructor

A move constructor transfers ownership of the resources instead of
copying every entry.

The source object is reset to an empty internal state.

### 9.6 Move Assignment

Move assignment must release the destination's old resources before
taking ownership of the source's resources. It must also handle
self-move assignment safely.

### 9.7 `swap()`

`swap()` exchanges the maps' resources and state. It is useful for copy
assignment and other ownership-management patterns.

### 9.8 Important Review Point

Rule of Five code must be tested with:

-   Self-copy assignment.
-   Self-move assignment.
-   Copying an empty map.
-   Copying a map with collisions.
-   Moving an empty map.
-   Reusing moved-from maps.
-   Types whose copy, move, hash, or equality operations can throw.

The test suite may not yet cover all of these cases.

------------------------------------------------------------------------

## 10. Const Correctness

The advanced implementation has both mutable and const lookup overloads:

``` cpp
Value* find(const Key& key);

const Value* find(const Key& key) const;
```

The const overload allows lookup through a const map:

``` cpp
const CustomHashMap<int, int>& const_map = map;

const int* result = const_map.find(10);
```

A `const` member function promises not to modify the map's observable
state through that member function.

`contains()`, `size()`, `empty()`, and the bucket/load-factor observers
are also const member functions.

Const correctness makes APIs easier to use safely with read-only
objects.

------------------------------------------------------------------------

## 11. Exception-Safety Considerations

Potential exceptions include:

-   Bucket-array allocation failure.
-   Entry allocation failure.
-   Key or value copy-construction failure.
-   Hash function failure.
-   Equality comparison failure.
-   Temporary metadata allocation failure during rehashing.

A robust container should define what happens to its state if these
failures occur.

The advanced rehash implementation attempts to allocate the new bucket
array and calculate new indices before relinking existing nodes. This is
a useful pattern, but exception safety must be evaluated across the
whole public operation, not just one helper.

For example, `insert()` can trigger rehashing and then allocate the new
entry. If that final allocation fails, the insertion may throw after the
bucket count has already changed. The map can still contain its existing
entries, but the operation may not provide a full no-effects guarantee.

**Learning note:** Strong exception safety means that if an operation
throws, the observable state remains unchanged. Basic exception safety
generally means invariants are preserved and resources are not leaked,
even if the state changes.

------------------------------------------------------------------------

## 12. Testing Strategy

The test program uses `assert()` to validate expected behavior.

### 12.1 Core Tests

-   Insert a new key.
-   Reject a duplicate key.
-   Find an existing key.
-   Search for a missing key.
-   Modify a stored value through a returned pointer.
-   Verify `size()` and `empty()`.

### 12.2 `operator[]` Tests

-   Access a missing integer key and check for zero.
-   Assign through `map[key]`.
-   Access the same key again without increasing the size.
-   Modify an existing value.

### 12.3 Collision Tests

A constant hash function can force all keys into the same bucket:

``` cpp
struct ConstantHash
{
    std::size_t operator()(int) const noexcept
    {
        return 0;
    }
};
```

This allows tests to exercise linked-list traversal and head, middle,
and last-node deletion deterministically.

### 12.4 Rehash Tests

Verify that:

-   Automatic rehashing increases bucket count when necessary.
-   Manual rehashing preserves values.
-   The resulting load factor respects the configured maximum.
-   Insertion and lookup continue to work after rehashing.

### 12.5 Copy and Move Tests

Verify independent copies, resource transfer, and reuse of moved-from
maps.

### 12.6 Assert Caveat

If `NDEBUG` is defined, `assert()` expressions are removed. Run the test
executable with assertions enabled, and avoid relying on side effects
inside assertions for release-mode test infrastructure.

For more robust long-term testing, use a testing framework or explicit
test checks that remain active in optimized builds.

------------------------------------------------------------------------

## 13. Benchmarking

The benchmark compares the custom implementation against
`std::unordered_map` for:

-   Insertion.
-   Successful lookup.
-   Erasure.

It uses `std::chrono::steady_clock` for timing and a fixed random seed
to generate a repeatable shuffled key order.

### 13.1 Fairness Considerations

A meaningful benchmark should:

1.  Use the same keys and values for both containers.
2.  Exclude setup time when measuring an individual operation, where
    appropriate.
3.  Verify that both containers produce equivalent results.
4.  Run multiple repetitions.
5.  Use optimized builds for performance claims.
6.  Avoid printing inside timed loops.
7.  Report actual results rather than invented sample timings.

The provided benchmark is a basic learning exercise. It does not by
itself establish universal performance conclusions.

### 13.2 Timing Ratio

``` text
Ratio = CustomHashMap time / std::unordered_map time
```

-   `1.0x`: similar elapsed time.
-   Greater than `1.0x`: the custom implementation took longer.
-   Less than `1.0x`: the custom implementation took less time.

Performance can vary with the compiler, optimization flags, CPU, system
load, memory allocator, and hash distribution.

------------------------------------------------------------------------

## 14. Complexity Reference

Let:

-   `n` = number of entries.
-   `b` = current number of buckets.
-   `b'` = requested new number of buckets.

  Operation                Average        Worst Case
  -------------- ----------------- -----------------
  `insert()`                  O(1)              O(n)
  `find()`                    O(1)              O(n)
  `contains()`                O(1)              O(n)
  `erase()`                   O(1)              O(n)
  `operator[]`                O(1)              O(n)
  `size()`                    O(1)              O(1)
  `empty()`                   O(1)              O(1)
  `clear()`               O(n + b)          O(n + b)
  `rehash()`       O(n + b + b')\*   O(n + b + b')\*

`*` Assumes constant-time hash calculations. User-defined hash functions
may have different costs.

------------------------------------------------------------------------

## 15. Build Instructions

From the project root, configure with MinGW Makefiles:

``` bat
cmake -S . -B build -G "MinGW Makefiles"
```

Build the project:

``` bat
cmake --build build
```

Run the tests:

``` bat
build\test_custom_hashmap.exe
```

Build the benchmark target:

``` bat
cmake --build build --target benchmark_custom_hashmap
```

Run the benchmark:

``` bat
build\benchmark_custom_hashmap.exe
```

If a target or executable name differs in the local CMake configuration,
use the actual target name defined in `CMakeLists.txt`.

------------------------------------------------------------------------

## 16. Troubleshooting

### CMake Cannot Find the Benchmark Source

Confirm that the source file exists at:

``` text
benchmarks/benchmark_custom_hashmap.cpp
```

The file name and path must match the `add_executable()` entry in
`CMakeLists.txt`.

### Target Is Not Built by the Project

Confirm that `add_executable(benchmark_custom_hashmap ...)` appears
before commands such as `target_compile_features()` and
`target_include_directories()` that reference the target.

### Include File Not Found

Confirm that the header is named `custom-hashmap.hpp` and that the CMake
target includes the `include/` directory.

### Assertions Do Not Run

Check whether `NDEBUG` is defined. Assertions should remain enabled in
the test build.

### Benchmark Results Vary

Close unnecessary background applications, use a consistent build
configuration, and repeat the measurements.

------------------------------------------------------------------------

## 17. Limitations and Future Work

The implementation is educational and does not yet aim to provide full
standard-container compatibility.

Possible improvements include:

-   Iterators and range-based iteration.
-   Additional lookup and insertion APIs.
-   More extensive stress tests.
-   Self-move and throwing-type tests.
-   Sanitizer runs and memory diagnostics.
-   More rigorous benchmark methodology.
-   Multiple key and value types in benchmarks.
-   CI integration.
-   A documented pointer/reference invalidation policy.
-   Further exception-safety improvements.

------------------------------------------------------------------------

## 18. Interview Questions

### Q1. What is a hash map?

A hash map stores key-value pairs and uses a hash function to determine
a bucket for a key.

### Q2. What is a collision?

A collision occurs when distinct keys map to the same bucket index.

### Q3. How does separate chaining work?

Each bucket stores the head of a linked list. Entries with the same
bucket index are connected through their `next` pointers.

### Q4. Why is `Entry**` used?

Because the bucket array contains `Entry*` elements, and `buckets_`
points to that array.

### Q5. Why is `delete[] buckets_` necessary?

The bucket array was allocated using `new[]`. Its storage must be
released using the matching array-deallocation form.

### Q6. Why save `current->next` before deleting a node?

After deleting a node, accessing its members is invalid. Saving the next
pointer first allows traversal to continue safely.

### Q7. Why can compiler-generated copying be unsafe?

A shallow copy duplicates the raw pointer value instead of creating
independent ownership. Two objects can then try to free the same
resources.

### Q8. What is rehashing?

Rehashing changes the bucket array size and recalculates bucket indices
for stored entries.

### Q9. What is load factor?

The number of stored entries divided by the number of buckets.

### Q10. Why does `operator[]` return a reference?

Returning `Value&` allows callers to read and modify the actual stored
value, for example `map[10] = 500`.

### Q11. What is the difference between `find()` and `contains()`?

`find()` returns a pointer to the value or `nullptr`; `contains()`
returns a boolean indicating whether the key exists.

### Q12. What are the average and worst-case lookup complexities?

Average-case lookup is O(1) under suitable hashing and load conditions;
worst-case lookup is O(n) if entries concentrate in one chain.

### Q13. What is the Rule of Five?

For resource-owning classes, the five relevant special member functions
are the destructor, copy constructor, copy assignment operator, move
constructor, and move assignment operator.

### Q14. Does a passing test suite prove the absence of memory bugs?

No. Tests cover only the exercised cases. Sanitizers, stress tests,
exception tests, and code review provide additional confidence.

------------------------------------------------------------------------

## 19. Final Summary

`CustomHashMap` combines several important C++ and data-structure
concepts:

-   Hashing and bucket indexing.
-   Separate chaining.
-   Linked-list insertion and deletion.
-   Dynamic memory ownership.
-   Reference and pointer semantics.
-   Load factor and rehashing.
-   Copy and move operations.
-   Const correctness.
-   Exception-safety considerations.
-   Testing and benchmarking.

The most valuable outcome is not merely having a working container. It
is understanding why each data structure, pointer update, ownership
rule, and test exists.

**Suggested next step:** run the complete tests with assertions enabled,
perform memory diagnostics, record actual benchmark results, and then
commit the verified project state.