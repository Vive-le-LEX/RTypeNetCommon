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
        class UdpClientInterface : public std::enable_shared_from_this<UdpClientInterface<MessageType>> {
           public:
            UdpClientInterface(asio::io_context& context, std::string host, uint16_t port) : context_(context),
                                                                                             port_(port),
                                                                                             host_(std::move(host)),
                                                                                             socket_(context),
                                                                                             bytesSending_(0),
                                                                                             bytesSent_(0),
                                                                                             bytesReceived_(0),
                                                                                             datagramsSent_(0),
                                                                                             datagramsReceived_(0)

            {
                std::random_device rd;
                auto seed_data = std::array<int, std::mt19937::state_size>{};
                std::generate(std::begin(seed_data), std::end(seed_data), std::ref(rd));
                std::seed_seq seq(std::begin(seed_data), std::end(seed_data));
                std::mt19937 generator(seq);
                uuids::uuid_random_generator gen{generator};
                uuid_ = gen();
            }

            bool Connect() {
                assert(!host_.empty() && "Server address must not be empty!");
                if (host_.empty()) {
                    return false;
                }

                assert((port_ > 0) && "Server port number must be valid!");
                if (port_ <= 0) {
                    return false;
                }

                if (IsConnected()) {
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

            bool Disconnect() { return DisconnectInternal(); }

            size_t Send(const message<MessageType>& msg) {
                return Send(endpoint_, msg);
            }

            size_t Send(const asio::ip::udp::endpoint& endpoint, const message<MessageType>& msg) {
                if (!IsConnected()) {
                    return 0;
                }

                if (msg.size() == 0) {
                    return 0;
                }

                asio::error_code ec;

                tempOutgoingMessage_ = msg;

                size_t sent = socket_.send_to(asio::buffer(&tempOutgoingMessage_, tempOutgoingMessage_.size()), endpoint, 0, ec);

                if (sent > 0) {
                    ++datagramsSent_;
                    bytesSent_ += sent;

                    onSent(endpoint, sent);
                }

                if (ec) {
                    SendError(ec);
                    Disconnect();
                }

                return sent;
            }

            bool SendAsync(const message<MessageType>& msg) {
                return SendAsync(endpoint_, msg);
            }

            bool SendAsync(const asio::ip::udp::endpoint& endpoint, const message<MessageType>& msg) {
                if (sending_) {
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

                socket_.async_send_to(asio::buffer(&tempOutgoingMessage_.header, sizeof(message_header<MessageType>)), endpoint, 0, sendHandler);

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
                    Disconnect();
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

                    if (!IsConnected())
                        return;

                    // Disconnect on error
                    if (ec) {
                        SendError(ec);
                        DisconnectInternalAsync(true);
                        return;
                    }

                    ++datagramsReceived_;
                    bytesReceived_ += received;

                    onReceived(endpoint_, receiveBuffer_.data(), received);

                    if (receiveBuffer_.size() == received) {
                        // Check the reception buffer limit
                        if (((2 * received) > receiveBufferLimit_) && (receiveBufferLimit_ > 0)) {
                            SendError(asio::error::no_buffer_space);
                            DisconnectInternalAsync(true);
                            return;
                        }

                        receiveBuffer_.resize(2 * received);
                    }
                };

                socket_.async_receive_from(asio::buffer(receiveBuffer_.data(), receiveBuffer_.size()), endpoint_, receiveHandler);
            }

            [[nodiscard]] uuids::uuid GetUuid() const noexcept { return uuid_; }

            [[nodiscard]] uint16_t GetPort() const noexcept { return port_; }
            [[nodiscard]] std::string GetHost() const noexcept { return host_; }

            [[nodiscard]] asio::io_context& GetContext() noexcept { return context_; }
            [[nodiscard]] asio::ip::udp::socket& GetSocket() noexcept { return socket_; }
            [[nodiscard]] asio::ip::udp::endpoint& GetEndpoint() noexcept { return endpoint_; }

            [[nodiscard]] bool IsStarted() const noexcept { return IsConnected(); }
            [[nodiscard]] bool IsConnected() const noexcept { return connected_; }

            [[nodiscard]] uint64_t GetBytesSending() const noexcept { return bytesSending_; }
            [[nodiscard]] uint64_t GetBytesSent() const noexcept { return bytesSent_; }
            [[nodiscard]] uint64_t GetBytesReceived() const noexcept { return bytesReceived_; }
            [[nodiscard]] uint64_t GetDatagramsSent() const noexcept { return datagramsSent_; }
            [[nodiscard]] uint64_t GetDatagramsReceived() const noexcept { return datagramsReceived_; }

            [[nodiscard]] bool IsSending() const noexcept { return sending_; }
            [[nodiscard]] bool IsReceiving() const noexcept { return receiving_; }

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
            uuids::uuid uuid_;

            uint16_t port_;
            std::string host_;

            asio::io_context& context_;

            asio::ip::udp::socket socket_;
            asio::ip::udp::endpoint endpoint_;
            std::atomic<bool> resolving_ = false;
            std::atomic<bool> connected_ = false;

            std::vector<uint8_t> receiveBuffer_;
            size_t receiveBufferLimit_{0};

            message<MessageType> tempOutgoingMessage_;

            bool sending_ = false;
            bool receiving_ = false;

            // Client statistic
            uint64_t bytesSending_;
            uint64_t bytesSent_;
            uint64_t bytesReceived_;
            uint64_t datagramsSent_;
            uint64_t datagramsReceived_;

            //! Disconnect the client (internal synchronous)
            bool DisconnectInternal() {
                if (!IsConnected())
                    return false;

                // Close the client socket
                socket_.close();

                // Update the connected flag
                resolving_ = false;
                connected_ = false;

                // Update sending/receiving flags
                receiving_ = false;
                sending_ = false;

                // Clear send/receive buffers
                ClearBuffers();

                // Call the client disconnected handler
                onDisconnected();

                return true;
            }
            //! Disconnect the client (internal asynchronous)
            bool DisconnectInternalAsync(bool dispatch) {
                (void)dispatch;
                if (!IsConnected())
                    return false;

                asio::error_code ec;

                // Cancel the client socket
                (void)socket_.cancel(ec);

                // Dispatch or post the disconnect handler
                auto self(this->shared_from_this());
                auto disconnect_handler = [this, self]() { DisconnectInternal(); };
                context_.post(disconnect_handler);
                return true;
            }

            //! Clear send/receive buffers
            void ClearBuffers() {
                receiveBuffer_.clear();

                bytesSending_ = 0;
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
