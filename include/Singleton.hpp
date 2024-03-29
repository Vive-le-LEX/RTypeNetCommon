/**
 * Copyright (c) 2023 - Kleo
 * Authors:
 * - Antoine FRANKEL <antoine.frankel@epitech.eu>
 * NOTICE: All information contained herein is, and remains
 * the property of Kleo © and its suppliers, if any.
 * Dissemination of this information or reproduction of this material
 * is strictly forbidden unless prior written permission is obtained
 * from Kleo ©.
 * Credits: https://github.com/jimmy-park/singleton/
 */

#ifndef SINGLETON_DCLP_HPP_
#define SINGLETON_DCLP_HPP_
#define SINGLETON_INJECT_ABSTRACT_CLASS
#include <atomic>
#include <mutex>
#include <shared_mutex>
#include <utility>

/**
 * @brief Singleton class
 *
 * @tparam Derived
 */
template <typename Derived>
class Singleton {
   public:
    template <typename... Args>
    static void Construct(Args&&... args) {
#ifndef SINGLETON_INJECT_ABSTRACT_CLASS
        using Instance = Derived;
#else
        struct Dummy : public Derived {
            using Derived::Derived;
            void ProhibitConstructFromDerived() const override {}
        };
        using Instance = Dummy;
#endif  // SINGLETON_INJECT_ABSTRACT_CLASS

        if (!instance_.load(std::memory_order_acquire)) {
            std::lock_guard lock{mutex_};

            if (!instance_.load(std::memory_order_relaxed))
                instance_.store(new Instance{std::forward<Args>(args)...}, std::memory_order_release);
        }
    }

    static void Destruct() {
        if (instance_.load(std::memory_order_acquire)) {
            std::lock_guard lock{mutex_};

            if (auto* instance = instance_.load(std::memory_order_relaxed); instance) {
                delete instance;
                instance_.store(nullptr, std::memory_order_release);
            }
        }
    }

    static Derived* GetInstance() {
        auto* instance = instance_.load(std::memory_order_acquire);

        if (!instance) {
            std::shared_lock lock{mutex_};

            instance = instance_.load(std::memory_order_relaxed);
        }

        return instance;
    }

   protected:
    Singleton() = default;
    Singleton(const Singleton&) = delete;
    Singleton(Singleton&&) noexcept = delete;
    Singleton& operator=(const Singleton&) = delete;
    Singleton& operator=(Singleton&&) noexcept = delete;
#ifndef SINGLETON_INJECT_ABSTRACT_CLASS
    ~Singleton() = default;
#else
    virtual ~Singleton() = default;
#endif  // SINGLETON_INJECT_ABSTRACT_CLASS

   private:
#ifdef SINGLETON_INJECT_ABSTRACT_CLASS
    virtual void ProhibitConstructFromDerived() const = 0;
#endif  // SINGLETON_INJECT_ABSTRACT_CLASS

    inline static std::atomic<Derived*> instance_{nullptr};
    inline static std::shared_mutex mutex_;
};

#endif  // SINGLETON_DCLP_HPP_
