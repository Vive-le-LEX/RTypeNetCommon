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

namespace RType {
    namespace net {

        template <typename MessageType>
        class UdpServer : public std::enable_shared_from_this<UdpServer<MessageType>> {
           public:
            UdpServer(asio::io_context& context, uint16_t port) : _port(port),
                                                                  _context(context),
                                                                  _socket(context, asio::ip::udp::endpoint(asio::ip::udp::v4(), port)) {}

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
                    _socket.open(_endpoint.protocol());

                    _socket.bind(_endpoint);

                    _started = true;
                    onStarted();
                };

                _context.get_executor().post(startHandler);
                return true;
            }

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
                    _socket.close();
                    _started = false;
                    onStopped();
                };

                _context.get_executor().post(stopHandler);

                return true;
            }

            size_t Send(const asio::ip::udp::endpoint& endpoint, const message<MessageType>& msg) {
                if (!this->IsStarted()) {
                    std::cout << "[UDP] Server is not started" << std::endl;
                    return 0;
                }

                if (msg == nullptr) {
                    std::cout << "[UDP] Message is null" << std::endl;
                    return 0;
                }

                asio::error_code ec;

                size_t sent = _socket.send_to(asio::buffer(msg.header, msg.header.size), endpoint, 0, ec);

                if (sent > 0) {
                    std::cout << "[UDP] Message sent to " << endpoint << std::endl;
                } else {
                    std::cout << "[UDP] Message not sent to " << endpoint << std::endl;
                }

                if (ec) {
                    std::cout << "[UDP] Error: " << ec.message() << std::endl;
                }

                return sent;
            }

            bool SendAsync(const asio::ip::udp::endpoint& endpoint, const message<MessageType>& msg) {
                if (!this->IsStarted()) {
                    std::cout << "[UDP] Server is not started" << std::endl;
                    return false;
                }

                if (_sending) {
                    std::cout << "[UDP] Server is already sending a message" << std::endl;
                    return false;
                }

                if (msg == nullptr) {
                    std::cout << "[UDP] Message is null" << std::endl;
                    return false;
                }

                _sending = true;
                auto self = this->shared_from_this();
                auto sendHandler = [this, self](std::error_code ec, size_t sent) {
                    (void)sent;
                    _sending = false;

                    if (!this->IsStarted()) {
                        return;
                    }

                    if (ec) {
                        std::cout << "[UDP] Error: " << ec.message() << std::endl;
                        return;
                    }
                };

                _socket.async_send_to(asio::buffer(msg.header, sizeof(message_header<MessageType>)), endpoint, sendHandler);

                return true;
            }

            size_t Receive(const asio::ip::udp::endpoint& endpoint, message<MessageType>& msg) {
                if (!this->IsStarted()) {
                    std::cout << "[UDP] Server is not started" << std::endl;
                    return 0;
                }

                if (msg == nullptr) {
                    std::cout << "[UDP] Message is null" << std::endl;
                    return 0;
                }

                asio::error_code ec;

                size_t received = _socket.receive_from(asio::buffer(msg.header, msg.header.size), endpoint, 0, ec);

                if (received > 0) {
                    std::cout << "[UDP] Message received from " << endpoint << std::endl;
                } else {
                    std::cout << "[UDP] Message not received from " << endpoint << std::endl;
                }

                if (ec) {
                    std::cout << "[UDP] Error: " << ec.message() << std::endl;
                }

                return received;
            }

            void ReceiveAsync() {
                if (!this->IsStarted()) {
                    std::cout << "[UDP] Server is not started" << std::endl;
                    return;
                }

                if (_receiving) {
                    std::cout << "[UDP] Server is already receiving a message" << std::endl;
                    return;
                }

                _receiving = true;
                auto self = this->shared_from_this();
                auto receiveHandler = [this, self](std::error_code ec, size_t received) {
                    (void)received;
                    _receiving = false;

                    if (!this->IsStarted()) {
                        return;
                    }

                    if (ec) {
                        std::cout << "[UDP] Error: " << ec.message() << std::endl;
                        return;
                    }
                };

                _socket.async_receive_from(asio::buffer(tempIncomingMessage.header, sizeof(message_header<MessageType>)), _endpoint, receiveHandler);
            }

            [[nodiscard]] bool IsStarted() const { return _started; }

            [[nodiscard]] u_int16_t GetPort() const { return _port; }

           protected:
            //! Handle server started notification
            virtual void onStarted() {}
            //! Handle server stopped notification
            virtual void onStopped() {}

           private:
            u_int16_t _port;

            asio::io_context& _context;
            asio::ip::udp::socket _socket;
            asio::ip::udp::endpoint _endpoint;

            message<MessageType> tempIncomingMessage;

            bool _started = false;
            bool _sending = false;
            bool _receiving = false;
        };

    }  // namespace net
}  // namespace RType
