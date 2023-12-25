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
        class UdpServerInterface : public std::enable_shared_from_this<UdpServerInterface<MessageType>> {
           public:
            UdpServerInterface(asio::io_context& context, uint16_t port) : context_(context),
                                                                           port_(port),
                                                                           socket_(context),
                                                                           bytesReceived_(0),
                                                                           bytesSending_(0),
                                                                           bytesSent_(0),
                                                                           datagramsReceived_(0),
                                                                           datagramsSent_(0) {
                endpoint_ = asio::ip::udp::endpoint(asio::ip::udp::v4(), port);
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

                    onStarted();
                };

                context_.post(startHandler);
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
                    socket_.close();
                    _started = false;
                    receiving_ = false;
                    sending_ = false;

                    onStopped();
                };

                context_.get_executor().post(stopHandler);

                return true;
            }

            size_t Send(const message<MessageType>& msg) {
                return Send(endpoint_, msg);
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

                tempOutgoingMessage_ = msg;

                size_t sent = socket_.send_to(asio::buffer(&tempOutgoingMessage_.header, sizeof(message_header<MessageType>)), endpoint, 0, ec);

                if (sent > 0) {
                    ++datagramsSent_;
                    bytesSent_ += sent;

                    onSent(endpoint, sent);
                }

                if (ec) {
                    SendError(ec);
                }

                return sent;
            }

            bool SendAsync(const message<MessageType>& msg) {
                return SendAsync(endpoint_, msg);
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

                if (sending_) {
                    std::cout << "[UDP] Server is already sending a message" << std::endl;
                    return false;
                }

                sending_ = true;

                bytesSending_ = msg.size();

                auto self = this->shared_from_this();
                auto sendHandler = [this, self](std::error_code ec, size_t sent) {
                    sending_ = false;

                    if (!this->IsStarted()) {
                        return;
                    }

                    if (sent > 0) {
                        bytesSending_ = 0;
                        ++datagramsSent_;
                        bytesSent_ += sent;

                        onSent(endpoint_, sent);
                    }

                    if (ec) {
                        SendError(ec);
                        return;
                    }
                };

                tempOutgoingMessage_ = msg;

                socket_.async_send_to(asio::buffer(&tempOutgoingMessage_.header, sizeof(message_header<MessageType>)), endpoint, sendHandler);

                return true;
            }

            size_t Receive(void* buffer, size_t size) {
                return Receive(endpoint_, buffer, size);
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

                size_t received = socket_.receive_from(asio::buffer(buffer, size), endpoint, 0, ec);

                ++datagramsReceived_;
                bytesReceived_ += received;

                onReceived(endpoint, buffer, received);

                if (ec) {
                    SendError(ec);
                }

                return received;
            }

            //! Receive datagram from the client (asynchronous)
            void ReceiveAsync() {
                if (receiving_) {
                    std::cout << "[UDP] Server is already receiving a message" << std::endl;
                    return;
                }

                if (!this->IsStarted()) {
                    std::cout << "[UDP] Server is not started" << std::endl;
                    return;
                }

                receiving_ = true;
                auto self = this->shared_from_this();
                auto receiveHandler = [this, self](std::error_code ec, size_t received) {
                    receiving_ = false;

                    if (!this->IsStarted()) {
                        return;
                    }

                    if (ec) {
                        SendError(ec);

                        onReceived(endpoint_, receiveBuffer_.data(), 0);
                        return;
                    }

                    ++datagramsReceived_;
                    bytesReceived_ += received;

                    onReceived(endpoint_, receiveBuffer_.data(), received);

                    if (receiveBuffer_.size() == received) {
                        // Check the reception buffer limit
                        if (((2 * received) > receiveBufferLimit_) && (receiveBufferLimit_ > 0)) {
                            SendError(asio::error::no_buffer_space);

                            // Call the datagram received zero handler
                            onReceived(endpoint_, receiveBuffer_.data(), 0);

                            return;
                        }

                        receiveBuffer_.resize(2 * received);
                    }
                };

                socket_.async_receive_from(asio::buffer(receiveBuffer_.data(), receiveBuffer_.size()), endpoint_, receiveHandler);
            }

            [[nodiscard]] bool IsStarted() const { return _started; }

            [[nodiscard]] uint16_t GetPort() const { return port_; }

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
            uint16_t port_;

            asio::io_context& context_;
            asio::ip::udp::socket socket_;
            asio::ip::udp::endpoint endpoint_;

            std::vector<uint8_t> receiveBuffer_;
            size_t receiveBufferLimit_{0};

            message<MessageType> tempOutgoingMessage_;

            std::atomic<bool> _started = false;
            bool sending_ = false;
            bool receiving_ = false;

            // Server statistic
            uint64_t bytesSending_;
            uint64_t bytesSent_;
            uint64_t bytesReceived_;
            uint64_t datagramsSent_;
            uint64_t datagramsReceived_;

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
