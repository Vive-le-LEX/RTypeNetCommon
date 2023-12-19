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

namespace RType {
    namespace net {
        template <typename MessageType>
        class UdpClient {
           public:
            UdpClient(asio::io_context& context, std::string host, uint16_t port) : _context(context),
                                                                                    _port(port),
                                                                                    _host(std::move(host)),
                                                                                    _socket(context),
                                                                                    _bytesSending(0),
                                                                                    _bytesSent(0),
                                                                                    _bytesReceived(0),
                                                                                    _datagramsSent(0),
                                                                                    _datagramsReceived(0)

            {
                std::random_device rd;
                auto seed_data = std::array<int, std::mt19937::state_size>{};
                std::generate(std::begin(seed_data), std::end(seed_data), std::ref(rd));
                std::seed_seq seq(std::begin(seed_data), std::end(seed_data));
                std::mt19937 generator(seq);
                uuids::uuid_random_generator gen{generator};
                _uuid = gen();
            }

            bool Connect() {
                assert(!_host.empty() && "Server address must not be empty!");
                if (_host.empty()) {
                    return false;
                }

                assert((_port > 0) && "Server port number must be valid!");
                if (_port <= 0) {
                    return false;
                }

                if (IsConnected()) {
                    return false;
                }

                _endpoint = asio::ip::udp::endpoint(asio::ip::make_address(_host), (unsigned short)_port);
                _socket.open(_endpoint.protocol());

                _socket.bind(asio::ip::udp::endpoint(_endpoint.protocol(), 0));

                _receiveBuffer.resize(1024);
                _receiveBufferLimit = 4096;

                _bytesSending = 0;
                _bytesSent = 0;
                _bytesReceived = 0;
                _datagramsSent = 0;
                _datagramsReceived = 0;

                _connected = true;

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
                _context.post(connect_handler);

                return true;
            }

            bool Disconnect() { return DisconnectInternal(); }

            size_t Send(const message<MessageType>& msg) {
                return Send(_endpoint, msg);
            }

            size_t Send(const asio::ip::udp::endpoint& endpoint, const message<MessageType>& msg) {
                if (!IsConnected()) {
                    return 0;
                }

                if (msg.size() == 0) {
                    return 0;
                }

                assert((msg != nullptr) && "Pointer to the buffer should not be null!");
                if (msg == nullptr) {
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
                    Disconnect();
                }

                return sent;
            }

            bool SendAsync(const message<MessageType>& msg) {
                return SendAsync(_endpoint, msg);
            }

            bool SendAsync(const asio::ip::udp::endpoint& endpoint, const message<MessageType>& msg) {
                if (_sending) {
                    return false;
                }

                if (!IsConnected()) {
                    return false;
                }

                if (msg.size() == 0) {
                    return false;
                }

                assert((msg != nullptr) && "Pointer to the buffer should not be null!");
                if (msg == nullptr) {
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

                _socket.async_send_to(asio::buffer(&tempOutgoingMessage.header, sizeof(message_header<MessageType>)), endpoint, 0, sendHandler);

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
                    Disconnect();
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

                    if (!IsConnected())
                        return;

                    // Disconnect on error
                    if (ec)
                    {
                        SendError(ec);
                        DisconnectInternalAsync(true);
                        return;
                    }

                    ++_datagramsReceived;
                    _bytesReceived += received;

                    onReceived(_endpoint, _receiveBuffer.data(), received);

                    if (_receiveBuffer.size() == received) {
                        // Check the reception buffer limit
                        if (((2 * received) > _receiveBufferLimit) && (_receiveBufferLimit > 0)) {
                            SendError(asio::error::no_buffer_space);
                            DisconnectInternalAsync(true);
                            return;
                        }

                        _receiveBuffer.resize(2 * received);
                    }
                };

                _socket.async_receive_from(asio::buffer(_receiveBuffer.data(), _receiveBuffer.size()), _endpoint, receiveHandler);
            }

            [[nodiscard]] bool IsConnected() const noexcept { return _connected; }

           protected:
            //! Handle client connected notification
            virtual void onConnected() {}
            //! Handle client disconnected notification
            virtual void onDisconnected() {}

            //! Handle datagram received notification
            /*!
                Notification is called when another datagram was received
                from some endpoint.

                \param endpoint - Received endpoint
                \param buffer - Received datagram buffer
                \param size - Received datagram buffer size
            */
            virtual void onReceived(const asio::ip::udp::endpoint& endpoint, const void* buffer, size_t size) = 0;
            //! Handle datagram sent notification
            /*!
                Notification is called when a datagram was sent to the server.

                This handler could be used to send another datagram to the server
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
            uuids::uuid _uuid;

            uint16_t _port;
            std::string _host;

            asio::io_context& _context;

            asio::ip::udp::socket _socket;
            asio::ip::udp::endpoint _endpoint;
            std::atomic<bool> _resolving = false;
            std::atomic<bool> _connected = false;

            std::vector<uint8_t> _receiveBuffer;
            size_t _receiveBufferLimit{0};

            message<MessageType> tempOutgoingMessage;

            bool _sending = false;
            bool _receiving = false;

            // Client statistic
            uint64_t _bytesSending;
            uint64_t _bytesSent;
            uint64_t _bytesReceived;
            uint64_t _datagramsSent;
            uint64_t _datagramsReceived;

            //! Disconnect the client (internal synchronous)
            bool DisconnectInternal() {
                if (!IsConnected())
                    return false;

                // Close the client socket
                _socket.close();

                // Update the connected flag
                _resolving = false;
                _connected = false;

                // Update sending/receiving flags
                _receiving = false;
                _sending = false;

                // Clear send/receive buffers
                ClearBuffers();

                // Call the client disconnected handler
                onDisconnected();

                return true;
            }
            //! Disconnect the client (internal asynchronous)
            bool DisconnectInternalAsync(bool dispatch) {
                if (!IsConnected())
                    return false;

                asio::error_code ec;

                // Cancel the client socket
                (void)_socket.cancel(ec);

                // Dispatch or post the disconnect handler
                auto self(this->shared_from_this());
                auto disconnect_handler = [this, self]() { DisconnectInternal(); };
                _context.post(disconnect_handler);
                return true;
            }

            //! Clear send/receive buffers
            void ClearBuffers() {
                _receiveBuffer.clear();

                _bytesSending = 0;
            }

            //! Send error notification
            void SendError(std::error_code ec) {
                // Skip Asio disconnect errors
                if ((ec == asio::error::connection_aborted) ||
                    (ec == asio::error::connection_refused) ||
                    (ec == asio::error::connection_reset) ||
                    (ec == asio::error::eof) ||
                    (ec == asio::error::operation_aborted))
                    return;

                onError(ec.value(), ec.category().name(), ec.message());
            }
        };
    }  // namespace net

}  // namespace RType
