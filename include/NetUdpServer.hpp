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
        class UdpServerInterface : public UdpConnection {
           public:
            /**
             * @brief UdpServerInterface constructor
             * @param context The asio context
             * @param port The port to bind to
             */
            UdpServerInterface(asio::io_context& context, uint16_t port) : UdpConnection(context, port) {
                endpoint_ = asio::ip::udp::endpoint(asio::ip::udp::v4(), port);
            }

            /**
             * @brief Start the server
             *
             * @return true
             * @return false
             */
            bool Start() {
                if (this->IsStarted()) {
                    std::cout << "[UDP] Server already started" << std::endl;
                    return false;
                }

                auto self = this->shared_from_this();
                auto startHandler = [this, self]() {
                    if (this->IsStarted()) {
                        return;
                    }
                    socket_.open(endpoint_.protocol());

                    socket_.bind(endpoint_);

                    port_ = socket_.local_endpoint().port();

                    receiveBuffer_.resize(1024);
                    receiveBufferLimit_ = 4096;

                    bytesReceived_ = 0;
                    bytesSending_ = 0;
                    bytesSent_ = 0;
                    datagramsReceived_ = 0;
                    datagramsSent_ = 0;

                    _started = true;
                    connected_ = true;

                    onStarted();
                };

                context_.post(startHandler);
                return true;
            }

            /**
             * @brief Stop the server
             *
             * @return true
             * @return false
             */
            bool Stop() {
                if (!this->IsStarted()) {
                    std::cout << "[UDP] Server is not started" << std::endl;
                    return false;
                }

                auto self = this->shared_from_this();
                auto stopHandler = [this, self]() {
                    if (!this->IsStarted()) {
                        return;
                    }
                    socket_.close();
                    _started = false;
                    receiving_ = false;
                    sending_ = false;

                    onStopped();
                };

                context_.get_executor().post(stopHandler, std::allocator<void>());

                return true;
            }

            /**
             * @brief IsStarted returns whether the server is started or not
             *
             * @return true
             * @return false
             */
            [[nodiscard]] bool IsStarted() const { return _started; }

           protected:
            /**
             * @brief onStarted is called when the server is started
             */
            virtual void onStarted() = 0;
            /**
             * @brief onStopped is called when the server is stopped
             */
            virtual void onStopped() = 0;

           private:
            std::atomic<bool> _started = false;
        };

    }  // namespace net
}  // namespace RType
