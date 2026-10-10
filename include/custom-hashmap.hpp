
#pragma once

#include <cmath>
#include <cstddef>
#include <functional>
#include <limits>
#include <memory>
#include <stdexcept>
#include <type_traits>
#include <utility>
#include <vector>

template <typename Key,
          typename Value,
          typename Hash = std::hash<Key>,
          typename KeyEqual = std::equal_to<Key>>
class CustomHashMap
{
private:
    struct Entry
    {
        Key key;
        Value value;
        Entry* next = nullptr;
    };

    Hash hash_{};
    KeyEqual equal_{};

    std::size_t size_ = 0;
    std::size_t bucket_count_ = 8;
    Entry** buckets_ = nullptr;

    float max_load_factor_ = 0.75f;

    std::size_t hash_key(const Key& key) const
    {
        return hash_(key);
    }

    std::size_t bucket_index(
        const Key& key,
        std::size_t count) const
    {
        return hash_key(key) % count;
    }

    std::size_t bucket_index(const Key& key) const
    {
        return bucket_index(key, bucket_count_);
    }

    void ensure_initialized()
    {
        if (buckets_ == nullptr || bucket_count_ == 0)
        {
            constexpr std::size_t initial_count = 8;

            Entry** new_buckets =
                new Entry*[initial_count]{};

            buckets_ = new_buckets;
            bucket_count_ = initial_count;
        }
    }

public:
    explicit CustomHashMap(
        std::size_t bucket_count = 8,
        const Hash& hash = Hash{},
        const KeyEqual& equal = KeyEqual{})
        : hash_(hash),
          equal_(equal),
          bucket_count_(bucket_count == 0 ? 8 : bucket_count),
          buckets_(new Entry*[bucket_count_]{})
    {
    }

    ~CustomHashMap()
    {
        clear();
        delete[] buckets_;
    }

    // Rule of Five: Copy constructor
    CustomHashMap(const CustomHashMap& other)
        : hash_(other.hash_),
          equal_(other.equal_),
          bucket_count_(
              other.bucket_count_ == 0
                  ? 8
                  : other.bucket_count_),
          buckets_(new Entry*[bucket_count_]{}),
          max_load_factor_(other.max_load_factor_)
    {
        try
        {
            for (std::size_t i = 0;
                 i < other.bucket_count_;
                 ++i)
            {
                Entry* current = other.buckets_[i];

                while (current != nullptr)
                {
                    insert(current->key, current->value);
                    current = current->next;
                }
            }
        }
        catch (...)
        {
            clear();
            delete[] buckets_;
            buckets_ = nullptr;
            throw;
        }
    }

    // Rule of Five: Copy assignment
    CustomHashMap& operator=(const CustomHashMap& other)
    {
        if (this != &other)
        {
            CustomHashMap temporary(other);
            swap(temporary);
        }

        return *this;
    }

    // Rule of Five: Move constructor
    CustomHashMap(CustomHashMap&& other)
        : hash_(std::move(other.hash_)),
          equal_(std::move(other.equal_)),
          size_(other.size_),
          bucket_count_(other.bucket_count_),
          buckets_(other.buckets_),
          max_load_factor_(other.max_load_factor_)
    {
        other.size_ = 0;
        other.bucket_count_ = 0;
        other.buckets_ = nullptr;
    }

    // Rule of Five: Move assignment
    CustomHashMap& operator=(CustomHashMap&& other)
    {
        if (this != &other)
        {
            clear();
            delete[] buckets_;

            buckets_ = nullptr;
            size_ = 0;
            bucket_count_ = 0;

            hash_ = std::move(other.hash_);
            equal_ = std::move(other.equal_);

            size_ = other.size_;
            bucket_count_ = other.bucket_count_;
            buckets_ = other.buckets_;
            max_load_factor_ = other.max_load_factor_;

            other.size_ = 0;
            other.bucket_count_ = 0;
            other.buckets_ = nullptr;
        }

        return *this;
    }

    void swap(CustomHashMap& other)
        noexcept(
            std::is_nothrow_swappable_v<Hash> &&
            std::is_nothrow_swappable_v<KeyEqual>)
    {
        using std::swap;

        swap(hash_, other.hash_);
        swap(equal_, other.equal_);
        swap(size_, other.size_);
        swap(bucket_count_, other.bucket_count_);
        swap(buckets_, other.buckets_);
        swap(max_load_factor_, other.max_load_factor_);
    }

    bool insert(const Key& key, const Value& value)
    {
        ensure_initialized();

        std::size_t index = bucket_index(key);
        Entry* current = buckets_[index];

        while (current != nullptr)
        {
            if (equal_(current->key, key))
            {
                return false;
            }

            current = current->next;
        }

        if (size_ == std::numeric_limits<std::size_t>::max())
        {
            throw std::length_error("CustomHashMap is too large");
        }

        const double next_load =
            static_cast<double>(size_ + 1) /
            static_cast<double>(bucket_count_);

        if (next_load > max_load_factor_)
        {
            if (bucket_count_ >
                std::numeric_limits<std::size_t>::max() / 2)
            {
                throw std::length_error(
                    "CustomHashMap bucket count overflow");
            }

            rehash(bucket_count_ * 2);
            index = bucket_index(key);
        }

        Entry* new_entry =
            new Entry{key, value, buckets_[index]};

        buckets_[index] = new_entry;
        ++size_;

        return true;
    }

    Value* find(const Key& key)
    {
        if (buckets_ == nullptr || bucket_count_ == 0)
        {
            return nullptr;
        }

        const std::size_t index = bucket_index(key);
        Entry* current = buckets_[index];

        while (current != nullptr)
        {
            if (equal_(current->key, key))
            {
                return &current->value;
            }

            current = current->next;
        }

        return nullptr;
    }

    const Value* find(const Key& key) const
    {
        if (buckets_ == nullptr || bucket_count_ == 0)
        {
            return nullptr;
        }

        const std::size_t index = bucket_index(key);
        const Entry* current = buckets_[index];

        while (current != nullptr)
        {
            if (equal_(current->key, key))
            {
                return &current->value;
            }

            current = current->next;
        }

        return nullptr;
    }

    bool contains(const Key& key) const
    {
        return find(key) != nullptr;
    }

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

    bool erase(const Key& key)
    {
        if (buckets_ == nullptr || bucket_count_ == 0)
        {
            return false;
        }

        const std::size_t index = bucket_index(key);

        Entry* current = buckets_[index];
        Entry* previous = nullptr;

        while (current != nullptr)
        {
            if (equal_(current->key, key))
            {
                if (previous == nullptr)
                {
                    buckets_[index] = current->next;
                }
                else
                {
                    previous->next = current->next;
                }

                delete current;
                --size_;

                return true;
            }

            previous = current;
            current = current->next;
        }

        return false;
    }

    void clear()
    {
        if (buckets_ != nullptr)
        {
            for (std::size_t i = 0;
                 i < bucket_count_;
                 ++i)
            {
                Entry* current = buckets_[i];

                while (current != nullptr)
                {
                    Entry* next = current->next;
                    delete current;
                    current = next;
                }

                buckets_[i] = nullptr;
            }
        }

        size_ = 0;
    }

    std::size_t size() const noexcept
    {
        return size_;
    }

    bool empty() const noexcept
    {
        return size_ == 0;
    }

    std::size_t bucket_count() const noexcept
    {
        return bucket_count_;
    }

    float load_factor() const noexcept
    {
        if (bucket_count_ == 0)
        {
            return 0.0f;
        }

        return static_cast<float>(size_) /
               static_cast<float>(bucket_count_);
    }

    float max_load_factor() const noexcept
    {
        return max_load_factor_;
    }

    void max_load_factor(float factor)
    {
        if (!std::isfinite(factor) || factor <= 0.0f)
        {
            throw std::invalid_argument(
                "Maximum load factor must be finite and positive");
        }

        const float old_factor = max_load_factor_;
        max_load_factor_ = factor;

        try
        {
            if (bucket_count_ == 0 ||
                load_factor() > max_load_factor_)
            {
                rehash(bucket_count_ == 0 ? 8 : bucket_count_);
            }
        }
        catch (...)
        {
            max_load_factor_ = old_factor;
            throw;
        }
    }

    void rehash(std::size_t requested_count)
    {
        if (requested_count == 0)
        {
            requested_count = 1;
        }

        const double required_count =
            std::ceil(
                static_cast<double>(size_) /
                static_cast<double>(max_load_factor_));

        if (required_count >
            static_cast<double>(
                std::numeric_limits<std::size_t>::max()))
        {
            throw std::length_error(
                "Required bucket count is too large");
        }

        const std::size_t minimum_count =
            static_cast<std::size_t>(required_count);

        if (requested_count < minimum_count)
        {
            requested_count = minimum_count;
        }

        if (requested_count == 0)
        {
            requested_count = 1;
        }

        if (requested_count == bucket_count_ &&
            buckets_ != nullptr)
        {
            return;
        }

        std::unique_ptr<Entry*[]> new_buckets(
            new Entry*[requested_count]{});

        std::vector<std::pair<Entry*, std::size_t>> entries;
        entries.reserve(size_);

        if (buckets_ != nullptr)
        {
            for (std::size_t i = 0;
                 i < bucket_count_;
                 ++i)
            {
                Entry* current = buckets_[i];

                while (current != nullptr)
                {
                    const std::size_t new_index =
                        bucket_index(current->key, requested_count);

                    entries.emplace_back(current, new_index);
                    current = current->next;
                }
            }
        }

        // Re-link nodes only after allocation and hashing succeed.
        for (const auto& item : entries)
        {
            Entry* node = item.first;
            const std::size_t index = item.second;

            node->next = new_buckets[index];
            new_buckets[index] = node;
        }

        Entry** old_buckets = buckets_;

        buckets_ = new_buckets.release();
        bucket_count_ = requested_count;

        delete[] old_buckets;
    }
};
