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

#include <utility>

#include "NetCommon.hpp"
#include "NetMessage.hpp"
#include "NetUdpConnection.hpp"

namespace RType {
    namespace net {
        /**
         * @brief Abstract class for an UDP client
         *
         */
        class UdpClientInterface : public UdpConnection {
           public:
            /**
             * @brief Construct a new Udp Client Interface object
             *
             * @param context ASIO context
             * @param host The host to connect to
             * @param port The port to connect to
             */
            UdpClientInterface(asio::io_context& context, std::string host, uint16_t port) : UdpConnection(context, port),
                                                                                             host_(std::move(host)) {}

            /**
             * @brief Connect to the server
             *
             * @return true
             * @return false
             */
            bool Connect() {
                if (IsConnected()) {
                    return false;
                }

                assert(!host_.empty() && "Server address must not be empty!");
                if (host_.empty()) {
                    return false;
                }

                assert((port_ > 0) && "Server port number must be valid!");
                if (port_ <= 0) {
                    return false;
                }

                endpoint_ = asio::ip::udp::endpoint(asio::ip::make_address(host_), (unsigned short)port_);
                socket_.open(endpoint_.protocol());

                socket_.bind(asio::ip::udp::endpoint(endpoint_.protocol(), 0));

                receiveBuffer_.resize(1024);
                receiveBufferLimit_ = 4096;

                bytesSending_ = 0;
                bytesSent_ = 0;
                bytesReceived_ = 0;
                datagramsSent_ = 0;
                datagramsReceived_ = 0;

                connected_ = true;

                onConnected();

                return true;
            }

            /**
             * @brief Reconnect to the server
             *
             * @return true
             * @return false
             */
            bool Reconnect() {
                if (!Disconnect())
                    return false;

                return Connect();
            }

            /**
             * @brief connect to the server asynchronously
             *
             * @return true
             * @return false
             */
            bool ConnectAsync() {
                if (IsConnected())
                    return false;

                // Post the connect handler
                auto self(this->shared_from_this());
                auto connect_handler = [this, self]() { Connect(); };
                context_.post(connect_handler);

                return true;
            }

            /**
             * @brief Get the Host IP
             *
             * @return the host IP
             */
            [[nodiscard]] std::string GetHost() const noexcept { return host_; }

           protected:
            /**
             * @brief Function called when the client is connected
             *
             */
            virtual void onConnected() {}

            /**
             * @brief Function called when the client is disconnected
             *
             */
            virtual void onDisconnected() {}

            /**
             * @brief Function called when a message is received
             *
             * @param endpoint The endpoint from which the message was received
             * @param buffer The buffer containing the message
             * @param size The size of the message
             */
            virtual void onReceived(const asio::ip::udp::endpoint& endpoint, const void* buffer, size_t size) {}

            /**
             * @brief Function called when a message is sent
             *
             * @param endpoint The endpoint to which the message was sent
             * @param buffer The buffer containing the message
             * @param size The size of the message
             */
            virtual void onSent(const asio::ip::udp::endpoint& endpoint, const void* buffer, size_t size) {}

            /**
             * @brief Function called when an error occurs
             *
             * @param error The error code
             */
            virtual void onError(const asio::error_code& error) {}

           private:
            std::string host_;
        };
    }  // namespace net

}  // namespace RType
