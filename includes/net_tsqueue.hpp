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

#include "net_common.hpp"

namespace RType {

    namespace net {
        template <typename T>
        class tsqueue {
           public:
            tsqueue() = default;
            tsqueue(const tsqueue<T>&) = delete;
            virtual ~tsqueue() { clear(); }

           public:
            // Returns and maintains item at front of Queue
            const T& front() {
                std::scoped_lock lock(mutex);
                return queue.front();
            }

            // Returns and maintains item at back of Queue
            const T& back() {
                std::scoped_lock lock(mutex);
                return queue.back();
            }

            // Removes and returns item from front of Queue
            T pop_front() {
                std::scoped_lock lock(mutex);
                auto t = std::move(queue.front());
                queue.pop_front();
                return t;
            }

            // Removes and returns item from back of Queue
            T pop_back() {
                std::scoped_lock lock(mutex);
                auto t = std::move(queue.back());
                queue.pop_back();
                return t;
            }

            // Adds an item to back of Queue
            void push_back(const T& item) {
                std::scoped_lock lock(mutex);
                queue.emplace_back(std::move(item));

                std::unique_lock<std::mutex> ul(blockingMutex);
                cvBlocking.notify_one();
            }

            // Adds an item to front of Queue
            void push_front(const T& item) {
                std::scoped_lock lock(mutex);
                queue.emplace_front(std::move(item));

                std::unique_lock<std::mutex> ul(blockingMutex);
                cvBlocking.notify_one();
            }

            // Returns true if Queue has no items
            bool empty() {
                std::scoped_lock lock(mutex);
                return queue.empty();
            }

            // Returns number of items in Queue
            size_t count() {
                std::scoped_lock lock(mutex);
                return queue.size();
            }

            // Clears Queue
            void clear() {
                std::scoped_lock lock(mutex);
                queue.clear();
            }

            void wait() {
                while (empty()) {
                    std::unique_lock<std::mutex> ul(blockingMutex);
                    cvBlocking.wait(ul);
                }
            }

           protected:
            std::mutex mutex;
            std::deque<T> queue;
            std::condition_variable cvBlocking;
            std::mutex blockingMutex;
        };
    }  // namespace net
}  // namespace RType
