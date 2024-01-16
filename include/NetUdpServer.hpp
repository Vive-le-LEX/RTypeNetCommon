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
#include "NetMessage.hpp"
#include "NetUdpConnection.hpp"

namespace RType {
    namespace net {

        /**
         * @brief UdpServerInterface is the base class for all UDP servers
         */
        class UdpServerInterface {
           public:
            /**
             * @brief UdpServerInterface constructor
             * @param context The asio context
             * @param port The port to bind to
             */
            UdpServerInterface(asio::io_context& context, uint16_t port) : asioContext_(context),
                                                                           port_(port) {}

            void CreateNewConnection() {
                auto newConnection = std::make_shared<UdpConnection>(asioContext_, port_, receiveQueue_);
                connections_.push_back(std::move(newConnection));
            }

            /**
             * @brief Start the server
             *
             * @return true
             * @return false
             */
            bool Start() {
                started_ = true;
                // if (this->IsStarted()) {
                //     std::cout << "[UDP] Server already started" << std::endl;
                //     return false;
                // }

                // auto self = this->shared_from_this();
                // auto startHandler = [this, self]() {
                //     if (this->IsStarted()) {
                //         return;
                //     }
                //     socket_.open(endpoint_.protocol());

                //     socket_.bind(endpoint_);

                //     port_ = socket_.local_endpoint().port();

                //     receiveBuffer_.resize(1024);
                //     receiveBufferLimit_ = 4096;

                //     bytesReceived_ = 0;
                //     bytesSending_ = 0;
                //     bytesSent_ = 0;
                //     datagramsReceived_ = 0;
                //     datagramsSent_ = 0;

                //     started_ = true;
                //     connected_ = true;

                //     onStarted();
                // };

                // context_.post(startHandler);
                // return true;
            }

            /**
             * @brief Stop the server
             *
             * @return true
             * @return false
             */
            bool Stop() {
                // if (!this->IsStarted()) {
                //     std::cout << "[UDP] Server is not started" << std::endl;
                //     return false;
                // }

                // auto self = this->shared_from_this();
                // auto stopHandler = [this, self]() {
                //     if (!this->IsStarted()) {
                //         return;
                //     }
                //     socket_.close();
                //     started_ = false;
                //     receiving_ = false;
                //     sending_ = false;

                //     onStopped();
                // };

                // context_.get_executor().post(stopHandler, std::allocator<void>());

                // return true;
            }

            /**
             * @brief IsStarted returns whether the server is started or not
             *
             * @return true
             * @return false
             */
            [[nodiscard]] bool IsStarted() const { return started_; }

            /**
                @brief Force update the server
                @param maxMessages The maximum number of messages to process
                @param wait Whether to wait for a message
            */
            void Update(size_t maxMessages = -1) {
                size_t messageCount = 0;
                while (messageCount < maxMessages && !receiveQueue_.empty()) {
                    auto msg = receiveQueue_.pop_front();

                    onReceived(msg.remote, msg.buffer, msg.size);

                    messageCount++;
                }
            }

           protected:
            /**
             * @brief onStarted is called when the server is started
             */
            virtual void onStarted() = 0;
            /**
             * @brief onStopped is called when the server is stopped
             */
            virtual void onStopped() = 0;

            /**
             * @brief Function called when the client is connected
             *
             */
            virtual void onConnected() = 0;
            /**
             * @brief Function called when the client is disconnected
             *
             */
            virtual void onDisconnected() = 0;

            /**
             * @brief Function called when a datagram is received
             *
             * @param endpoint The endpoint of the received datagram
             * @param buffer The buffer of the received datagram
             * @param size The size of the received datagram
             */
            virtual void onReceived(const asio::ip::udp::endpoint& endpoint, const void* buffer, size_t size) = 0;
            /**
             * @brief Function called when a datagram is sent
             *
             * @param endpoint The endpoint of the sent datagram
             * @param sent The size of the sent datagram
             */
            virtual void onSent(const asio::ip::udp::endpoint& endpoint, size_t sent) = 0;

            /**
             * @brief Function called when an error occurs
             *
             * @param error The error code
             * @param category The error category
             * @param message The error message
             */
            virtual void onError(int error, const std::string& category, const std::string& message) = 0;

           private:
            uint16_t port_;

            TsQueue<UdpMessage_t> receiveQueue_;

            std::atomic<bool> started_ = false;
            std::vector<std::shared_ptr<UdpConnection>> connections_;

            asio::io_context& asioContext_;  ///< ASIO context for networking operations
            std::thread threadContext_;      ///< Thread context for networking events
        };

    }  // namespace net
}  // namespace RType
