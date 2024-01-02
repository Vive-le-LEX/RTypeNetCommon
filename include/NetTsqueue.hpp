/**
 * Copyright (c) 2023 - Kleo
 * Authors:
 * - Antoine FRANKEL <antoine.frankel@epitech.eu>
 * NOTICE: All information contained herein is, and remains
 * the property of Kleo © and its suppliers, if any.
 * Dissemination of this information or reproduction of this material
 * is strictly forbidden unless prior written permission is obtained
 * from Kleo ©.
 */

#pragma once

#include "NetCommon.hpp"

namespace RType {

    namespace net {
        template <typename T>
        class TsQueue {
           public:
            TsQueue() = default;
            TsQueue(const TsQueue<T>&) = delete;
            virtual ~TsQueue() { clear(); }

           public:
            // Returns and maintains item at front of Queue
            const T& front() {
                std::scoped_lock lock(mutex_);
                return queue_.front();
            }

            // Returns and maintains item at back of Queue
            const T& back() {
                std::scoped_lock lock(mutex_);
                return queue_.back();
            }

            // Removes and returns item from front of Queue
            T pop_front() {
                std::scoped_lock lock(mutex_);
                auto t = std::move(queue_.front());
                queue_.pop_front();
                return t;
            }

            // Removes and returns item from back of Queue
            T pop_back() {
                std::scoped_lock lock(mutex_);
                auto t = std::move(queue_.back());
                queue_.pop_back();
                return t;
            }

            // Adds an item to back of Queue
            void push_back(const T& item) {
                std::scoped_lock lock(mutex_);
                queue_.emplace_back(std::move(item));

                std::unique_lock<std::mutex> ul(blockingMutex_);
                blocking_.notify_one();
            }

            // Adds an item to front of Queue
            void push_front(const T& item) {
                std::scoped_lock lock(mutex_);
                queue_.emplace_front(std::move(item));

                std::unique_lock<std::mutex> ul(blockingMutex_);
                blocking_.notify_one();
            }

            // Returns true if Queue has no items
            bool empty() {
                std::scoped_lock lock(mutex_);
                return queue_.empty();
            }

            // Returns number of items in Queue
            size_t count() {
                std::scoped_lock lock(mutex_);
                return queue_.size();
            }

            // Clears Queue
            void clear() {
                std::scoped_lock lock(mutex_);
                queue_.clear();
            }

            void wait() {
                while (empty()) {
                    std::unique_lock<std::mutex> ul(blockingMutex_);
                    blocking_.wait(ul);
                }
            }

           protected:
            std::mutex mutex_;
            std::deque<T> queue_;
            std::condition_variable blocking_;
            std::mutex blockingMutex_;
        };
    }  // namespace net
}  // namespace RType
