#pragma once

#include "varia/randomizable.hpp"
#include <cassert>
#include <optional>
#include <unordered_map>

namespace mnkg::repository {

template <typename T, typename Key>
struct base {
public:
        using data = T;
        using key  = Key;

        void
        store(T data, const Key &id)
        {
                assert(!contains(id));
                return store_(std::move(data));
        }

        void
        store(T &&data, const Key &id)
        {
                assert(!contains(id));
                return store_(std::forward<T>(data));
        }

        [[nodiscard]] key
        store(T data)
        {
                return store(std::move(data));
        }

        [[nodiscard]] Key
        store(T &&data)
        {
                auto uid = make_uid_();
                store_(std::forward<T>(data), uid);
                return uid;
        }

        std::optional<T> // nullopt if not found
        get(const Key &id) const
        {
                return get_(id);
        }

        void
        erase(const Key &id)
        {
                erase_(id);
        }

        std::optional<T> // nullopt if not found
        extract(const Key &id)
        {
                return extract_(id);
        }

        std::optional<T>
        operator[](const Key &id)
        {
                return get(id);
        }

        bool
        contains(const Key &id) const
        {
                return contains_(id);
        }

        size_t
        size() const
        {
                return size_();
        }

        virtual ~base() = default;

private:
        virtual void
        store_(T &&data, const Key &id)
            = 0;

        virtual std::optional<T>
        get_(const Key &id) const = 0;

        virtual void
        erase_(const Key &id)
            = 0;

        virtual std::optional<T>
        extract_(const Key &id)
        {
                auto data = get(id);
                if (data)
                        erase(id);
                return data;
        }

        virtual Key
        make_uid_()
            = 0;

        virtual bool
        contains_(const Key &id) const
        {
                return get(id).has_value();
        }

        virtual size_t
        size_() const
            = 0;
};

namespace id_assigners {

template <typename T, randomizable Key>
class random : virtual base<T, Key> {
        virtual Key
        make_uid_() override final
        {
                Key uid;
                do {
                        uid = make_random<Key>();
                } while (contains(uid));
                return uid;
        }
};

template <typename T, typename Key>
class sequential : virtual base<T, Key> {

        virtual Key
        make_uid_() override final
        {
                static Key id = {};
                while (contains(id)) {
                        id++;
                }
                return id++;
        }
};

} // namespace id_assigners

template <typename T, typename Key>
class unordered_map : public virtual base<T, Key> {

        std::unordered_map<Key, T> data_;

        virtual void
        store_(T &&data, const Key &id) override
        {
                data_[id] = std::move(data);
        }

        virtual std::optional<T>
        get_(const Key &id) const override
        {
                auto it = data_.find(id);
                if (it == data_.end())
                        return std::nullopt;
                return it->second;
        }

        virtual void
        erase_(const Key &id) override
        {
                data_.erase(id);
        }

        virtual size_t
        size_() const override
        {
                return data_.size();
        }

        virtual bool
        contains_(const Key &id) const override
        {
                return data_.contains(id);
        }

        virtual std::optional<T>
        extract_(const Key &id) override
        {
                auto node = data_.extract(id);
                if (node.empty()) {
                        return std::nullopt;
                }
                return node.mapped();
        };
};

} // namespace mnkg::repository
