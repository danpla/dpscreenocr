
#pragma once

#include <mutex>
#include <utility>


namespace dpso {


template<typename T>
class RefLock {
public:
    RefLock(T& v, std::mutex& mutex)
        : v{v}
        , lock{mutex}
    {
    }

    const T& get() const
    {
        return v;
    }

    T& get()
    {
        return v;
    }
private:
    T& v;
    std::unique_lock<std::mutex> lock;
};


/**
 * Synchronized value.
 *
 * This is a wrapper that ties a value with a mutex.
 */
template<typename T>
class Synchronized {
public:
    template<typename... Args>
    Synchronized(std::in_place_t, Args&&... args)
        : v{std::forward<Args>(args)...}
    {
    }

    Synchronized()
        : v{}
    {
    }

    Synchronized(const T& v)
        : v{v}
    {
    }

    Synchronized(T&& v) noexcept
        : v{std::move(v)}
    {
    }

    Synchronized& operator=(const T& newV)
    {
        const std::lock_guard lock{mutex};
        v = newV;

        return *this;
    }

    Synchronized& operator=(T&& newV) noexcept
    {
        const std::lock_guard lock{mutex};
        v = std::move(newV);

        return *this;
    }

    Synchronized(const Synchronized& other)
        : v{(std::lock_guard{other.mutex}, other.v)}
    {
    }

    Synchronized& operator=(const Synchronized& other)
    {
        if (this != &other) {
            const std::scoped_lock lock{mutex, other.mutex};
            v = other.v;
        }

        return *this;
    }

    Synchronized(Synchronized&& other) noexcept
        : v{(std::lock_guard{other.mutex}, std::move(other.v))}
    {
    }

    Synchronized& operator=(Synchronized&& other) noexcept
    {
        if (this != &other) {
            const std::scoped_lock lock{mutex, other.mutex};
            v = std::move(other.v);
        }

        return *this;
    }

    RefLock<const T> getLock() const
    {
        return {v, mutex};
    }

    RefLock<T> getLock()
    {
        return {v, mutex};
    }
private:
    T v;
    mutable std::mutex mutex;
};


}
