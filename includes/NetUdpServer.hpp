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
            UdpServer(asio::io_context& context, uint16_t port) : _context(context),
                                                                  _port(port),
                                                                  _socket(context),
                                                                  _bytesReceived(0),
                                                                  _bytesSending(0),
                                                                  _bytesSent(0),
                                                                  _datagramsReceived(0),
                                                                  _datagramsSent(0)
            {
                _endpoint = asio::ip::udp::endpoint(asio::ip::udp::v4(), port);
            }

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

                    _receiveBuffer.resize(1024);
                    _receiveBufferLimit = 4096;

                    _bytesReceived = 0;
                    _bytesSending = 0;
                    _bytesSent = 0;
                    _datagramsReceived = 0;
                    _datagramsSent = 0;

                    _started = true;

                    onStarted();
                };

                _context.post(startHandler);
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
                    _receiving = false;
                    _sending = false;

                    onStopped();
                };

                _context.get_executor().post(stopHandler);

                return true;
            }

            size_t Send(const message<MessageType>& msg) {
                return Send(_endpoint, msg);
            }

            //! Send datagram to the given endpoint (synchronous)
            /*!
                \param endpoint - Endpoint to send to
                \param msg - Message to send
                \return Size of sent datagram
            */
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

                tempOutgoingMessage = msg;

                size_t sent = _socket.send_to(asio::buffer(&tempOutgoingMessage.header, sizeof(message_header<MessageType>)), endpoint, 0, ec);

                if (sent > 0) {
                    ++_datagramsSent;
                    _bytesSent += sent;

                    onSent(endpoint, sent);
                }

                if (ec) {
                    SendError(ec);
                }

                return sent;
            }

            bool SendAsync(const message<MessageType>& msg) {
                return SendAsync(_endpoint, msg);
            }

            //! Send datagram to the given endpoint (asynchronous)
            /*!
                \param endpoint - Endpoint to send to
                \param msg - Message to send
                \return True if the message was sent, false otherwise
            */
            bool SendAsync(const asio::ip::udp::endpoint& endpoint, const message<MessageType>& msg) {
                if (!this->IsStarted()) {
                    std::cout << "[UDP] Server is not started" << std::endl;
                    return false;
                }

                if (_sending) {
                    std::cout << "[UDP] Server is already sending a message" << std::endl;
                    return false;
                }

                _sending = true;

                _bytesSending = msg.size();

                auto self = this->shared_from_this();
                auto sendHandler = [this, self](std::error_code ec, size_t sent) {
                    _sending = false;

                    if (!this->IsStarted()) {
                        return;
                    }

                    if (sent > 0) {
                        _bytesSending = 0;
                        ++_datagramsSent;
                        _bytesSent += sent;

                        onSent(_endpoint, sent);
                    }

                    if (ec) {
                        SendError(ec);
                        return;
                    }
                };

                tempOutgoingMessage = msg;

                _socket.async_send_to(asio::buffer(&tempOutgoingMessage.header, sizeof(message_header<MessageType>)), endpoint, sendHandler);

                return true;
            }

            size_t Receive(void* buffer, size_t size) {
                return Receive(_endpoint, buffer, size);
            }

            //! Receive datagram from the given endpoint (synchronous)
            /*!
                \param endpoint - Endpoint to receive from
                \param buffer - Datagram buffer to receive
                \param size - Datagram buffer size to receive
                \return Size of received datagram
            */
            size_t Receive(asio::ip::udp::endpoint& endpoint, void* buffer, size_t size) {
                if (!this->IsStarted()) {
                    std::cout << "[UDP] Server is not started" << std::endl;
                    return 0;
                }

                if (size == 0) {
                    return 0;
                }

                assert((buffer != nullptr) && "Pointer to the buffer should not be null!");
                if (buffer == nullptr) {
                    return 0;
                }

                asio::error_code ec;

                size_t received = _socket.receive_from(asio::buffer(buffer, size), endpoint, 0, ec);

                ++_datagramsReceived;
                _bytesReceived += received;

                onReceived(endpoint, buffer, received);

                if (ec) {
                    SendError(ec);
                }

                return received;
            }

            //! Receive datagram from the client (asynchronous)
            void ReceiveAsync() {
                if (_receiving) {
                    std::cout << "[UDP] Server is already receiving a message" << std::endl;
                    return;
                }

                if (!this->IsStarted()) {
                    std::cout << "[UDP] Server is not started" << std::endl;
                    return;
                }

                _receiving = true;
                auto self = this->shared_from_this();
                auto receiveHandler = [this, self](std::error_code ec, size_t received) {
                    _receiving = false;

                    if (!this->IsStarted()) {
                        return;
                    }

                    if (ec) {
                        SendError(ec);

                        onReceived(_endpoint, _receiveBuffer.data(), 0);
                        return;
                    }

                    ++_datagramsReceived;
                    _bytesReceived += received;

                    onReceived(_endpoint, _receiveBuffer.data(), received);

                    if (_receiveBuffer.size() == received) {
                        // Check the reception buffer limit
                        if (((2 * received) > _receiveBufferLimit) && (_receiveBufferLimit > 0)) {
                            SendError(asio::error::no_buffer_space);

                            // Call the datagram received zero handler
                            onReceived(_endpoint, _receiveBuffer.data(), 0);

                            return;
                        }

                        _receiveBuffer.resize(2 * received);
                    }
                };

                _socket.async_receive_from(asio::buffer(_receiveBuffer.data(), _receiveBuffer.size()), _endpoint, receiveHandler);
            }

            [[nodiscard]] bool IsStarted() const { return _started; }

            [[nodiscard]] uint16_t GetPort() const { return _port; }

           protected:
            //! Handle server started notification
            virtual void onStarted() = 0;
            //! Handle server stopped notification
            virtual void onStopped() = 0;

            //! Handle datagram received notification
            /*!
                Notification is called when another datagram was received from
                some endpoint.

                \param endpoint - Received endpoint
                \param buffer - Received datagram buffer
                \param size - Received datagram buffer size
            */
            virtual void onReceived(const asio::ip::udp::endpoint& endpoint, const void* buffer, size_t size) = 0;

            //! Handle datagram sent notification
            /*!
                Notification is called when a datagram was sent to the client.

                This handler could be used to send another datagram to the client
                for instance when the pending size is zero.

                \param endpoint - Endpoint of sent datagram
                \param sent - Size of sent datagram buffer
            */
            virtual void onSent(const asio::ip::udp::endpoint& endpoint, size_t sent) = 0;

            //! Handle error notification
            /*!
                \param error - Error code
                \param category - Error category
                \param message - Error message
            */
            virtual void onError(int error, const std::string& category, const std::string& message) = 0;

           private:
            uint16_t _port;

            asio::io_context& _context;
            asio::ip::udp::socket _socket;
            asio::ip::udp::endpoint _endpoint;

            std::vector<uint8_t> _receiveBuffer;
            size_t _receiveBufferLimit{0};

            message<MessageType> tempOutgoingMessage;

            std::atomic<bool> _started = false;
            bool _sending = false;
            bool _receiving = false;

            // Server statistic
            uint64_t _bytesSending;
            uint64_t _bytesSent;
            uint64_t _bytesReceived;
            uint64_t _datagramsSent;
            uint64_t _datagramsReceived;

            void SendError(std::error_code ec) {
                // Skip Asio disconnect errors
                if ((ec == asio::error::connection_aborted) || (ec == asio::error::connection_refused) ||
                    (ec == asio::error::connection_reset) || (ec == asio::error::eof) ||
                    (ec == asio::error::operation_aborted)) {
                    return;
                }

                onError(ec.value(), ec.category().name(), ec.message());
            }
        };

    }  // namespace net
}  // namespace RType