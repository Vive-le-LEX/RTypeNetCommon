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
        class UdpClientInterface : public UdpConnection {
           public:
            UdpClientInterface(asio::io_context& context, std::string host, uint16_t port) : UdpConnection(context, port),
                                                                                             host_(std::move(host)) {}

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

            bool Reconnect() {
                if (!Disconnect())
                    return false;

                return Connect();
            }

            bool ConnectAsync() {
                if (IsConnected())
                    return false;

                // Post the connect handler
                auto self(this->shared_from_this());
                auto connect_handler = [this, self]() { Connect(); };
                context_.post(connect_handler);

                return true;
            }

            [[nodiscard]] std::string GetHost() const noexcept { return host_; }

           protected:
            virtual void onConnected() {}
            virtual void onDisconnected() {}

            virtual void onReceived(const asio::ip::udp::endpoint& endpoint, const void* buffer, size_t size) {}
            virtual void onSent(const asio::ip::udp::endpoint& endpoint, const void* buffer, size_t size) {}

            virtual void onError(const asio::error_code& error) {}

           private:
            std::string host_;
        };
    }  // namespace net

}  // namespace RType
