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
        /**
         * @brief Thread safe queue
         *
         * @tparam T
         */
        template <typename T>
        class TsQueue {
           public:
            TsQueue() = default;
            TsQueue(const TsQueue<T>&) = delete;
            virtual ~TsQueue() { clear(); }

           public:
            /**
             * @brief Returns and maintains item at front of Queue
             *
             * @return const T&
             */
            const T& front() {
                std::scoped_lock lock(mutex_);
                return queue_.front();
            }

            /**
             * @brief Returns and maintains item at back of Queue
             *
             * @return const T&
             */
            const T& back() {
                std::scoped_lock lock(mutex_);
                return queue_.back();
            }

            /**
             * @brief Removes and returns item from front of Queue
             *
             * @return T
             */
            T pop_front() {
                std::scoped_lock lock(mutex_);
                auto t = std::move(queue_.front());
                queue_.pop_front();
                return t;
            }

            /**
             * @brief Removes and returns item from back of Queue
             *
             * @return T
             */
            T pop_back() {
                std::scoped_lock lock(mutex_);
                auto t = std::move(queue_.back());
                queue_.pop_back();
                return t;
            }

            /**
             * @brief Adds an item to back of Queue
             *
             * @param item
             */
            void push_back(const T& item) {
                std::scoped_lock lock(mutex_);
                queue_.emplace_back(std::move(item));

                std::unique_lock<std::mutex> ul(blockingMutex_);
                blocking_.notify_one();
            }

            /**
             * @brief Adds an item to front of Queue
             *
             * @param item
             */
            void push_front(const T& item) {
                std::scoped_lock lock(mutex_);
                queue_.emplace_front(std::move(item));

                std::unique_lock<std::mutex> ul(blockingMutex_);
                blocking_.notify_one();
            }

            /**
             * @brief Returns true if Queue has no items
             *
             * @return true
             * @return false
             */
            bool empty() {
                std::scoped_lock lock(mutex_);
                return queue_.empty();
            }

            /**
             * @brief Returns number of items in Queue
             *
             * @return size_t
             */
            size_t count() {
                std::scoped_lock lock(mutex_);
                return queue_.size();
            }

            /**
             * @brief Clears Queue
             *
             */
            void clear() {
                std::scoped_lock lock(mutex_);
                queue_.clear();
            }

            /**
             * @brief Waits until Queue is not empty then returns front item from Queue
             */
            void wait() {
                while (empty()) {
                    std::unique_lock<std::mutex> ul(blockingMutex_);
                    blocking_.wait(ul);
                }
            }

           protected:
            std::mutex mutex_;                  ///< Mutex to protect the queue
            std::deque<T> queue_;               ///< Queue to be used for thread-safe operations
            std::condition_variable blocking_;  ///< Blocking condition variable
            std::mutex blockingMutex_;          ///< Mutex for the blocking condition variable
        };
    }  // namespace net
}  // namespace RType
