/**
 * Copyright (c) 2024 - Kleo
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
         * @brief Abstract class for an UDP connection
         *
         */
        class UdpConnection : public std::enable_shared_from_this<UdpConnection> {
           public:
            /**
             * @brief Construct a new Udp Connection object
             *
             * @param context ASIO context
             * @param port The port to listen on
             */
            UdpConnection(asio::io_context& context, uint16_t port) : context_(context),
                                                                      port_(port),
                                                                      socket_(context),
                                                                      bytesSending_(0),
                                                                      bytesSent_(0),
                                                                      bytesReceived_(0),
                                                                      datagramsSent_(0),
                                                                      datagramsReceived_(0) {}

            /**
             * @brief Disconnect
             *
             * @return true
             * @return false
             */
            virtual bool Disconnect() { return DisconnectInternal(); }

            /**
             * @brief Send datagram (synchronous)
             *
             * @param buffer The buffer to send
             * @param size The size of the buffer
             * @return size_t The size of the sent datagram
             */
            size_t Send(const void* buffer, size_t size) {
                return Send(endpoint_, buffer, size);
            }

            /**
             * @brief Send datagram (synchronous)
             *
             * @param endpoint  The endpoint to send to
             * @param buffer  The buffer to send
             * @param size The size of the buffer
             * @return size_t The size of the sent datagram
             */
            size_t Send(const asio::ip::udp::endpoint& endpoint, const void* buffer, size_t size) {
                if (!IsConnected()) {
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

                size_t sent = socket_.send_to(asio::const_buffer(buffer, size), endpoint, 0, ec);
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

            /**
             * @brief Send datagram (asynchronous)
             *
             * @param buffer The buffer to send
             * @param size The size of the buffer
             * @return true
             * @return false
             */
            bool SendAsync(const void* buffer, size_t size) {
                return SendAsync(endpoint_, buffer, size);
            }

            /**
             * @brief Send datagram (asynchronous)
             *
             * @param endpoint The endpoint to send to
             * @param buffer The buffer to send
             * @param size The size of the buffer
             * @return true
             * @return false
             */
            bool SendAsync(const asio::ip::udp::endpoint& endpoint, const void* buffer, size_t size) {
                if (sending_) {
                    return false;
                }

                if (!IsConnected()) {
                    return false;
                }

                if (size == 0) {
                    return false;
                }

                assert((buffer != nullptr) && "Pointer to the buffer should not be null!");
                if (buffer == nullptr) {
                    return false;
                }

                sending_ = true;
                bytesSending_ = size;

                auto self = this->shared_from_this();
                auto sendHandler = [this, self](std::error_code ec, size_t sent) {
                    sending_ = false;

                    if (!this->IsConnected()) {
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

                socket_.async_send_to(asio::buffer(buffer, size), endpoint, 0, sendHandler);

                return true;
            }

            /**
             * @brief Receive datagram from the client (synchronous)
             *
             * @param buffer The buffer to receive the datagram
             * @param size The size of the buffer
             * @return size_t The size of the received datagram
             */
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
                if (!this->IsConnected()) {
                    std::cout << "[UDP] Connection is not active" << std::endl;
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

            /**
             * @brief Receive datagram from the client (asynchronous)
             *
             */
            void ReceiveAsync() {
                if (receiving_) {
                    std::cout << "[UDP] Connection is already receiving a message" << std::endl;
                    return;
                }

                if (!this->IsConnected()) {
                    std::cout << "[UDP] Connection is not active" << std::endl;
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

            /**
             * @brief Get the Port object
             *
             * @return uint16_t
             */
            [[nodiscard]] uint16_t GetPort() const noexcept { return port_; }

            /**
             * @brief Get the Context object
             *
             * @return asio::io_context&
             */
            [[nodiscard]] asio::io_context& GetContext() noexcept { return context_; }
            /**
             * @brief Get the Socket object
             *
             * @return asio::ip::udp::socket&
             */
            [[nodiscard]] asio::ip::udp::socket& GetSocket() noexcept { return socket_; }
            /**
             * @brief Get the Endpoint object
             *
             * @return asio::ip::udp::endpoint&
             */
            [[nodiscard]] asio::ip::udp::endpoint& GetEndpoint() noexcept { return endpoint_; }

            /**
             * @brief Get the Receive Buffer object
             *
             * @return std::vector<uint8_t>&
             */
            [[nodiscard]] std::vector<uint8_t>& GetReceiveBuffer() noexcept { return receiveBuffer_; }
            /**
             * @brief Get the Receive Buffer Limit object
             *
             * @return size_t
             */
            [[nodiscard]] size_t GetReceiveBufferLimit() const noexcept { return receiveBufferLimit_; }
            /**
             * @brief Set the Receive Buffer Limit object
             *
             * @return true
             * @return false
             */
            [[nodiscard]] bool IsConnected() const noexcept { return connected_; }

            /**
             * @brief Get the Bytes Sending object
             *
             * @return uint64_t
             */
            [[nodiscard]] uint64_t GetBytesSending() const noexcept { return bytesSending_; }
            /**
             * @brief Get the Bytes Sent object
             *
             * @return uint64_t
             */
            [[nodiscard]] uint64_t GetBytesSent() const noexcept { return bytesSent_; }
            /**
             * @brief Get the Bytes Received object
             *
             * @return uint64_t
             */
            [[nodiscard]] uint64_t GetBytesReceived() const noexcept { return bytesReceived_; }
            /**
             * @brief Get the Datagrams Sent object
             *
             * @return uint64_t
             */
            [[nodiscard]] uint64_t GetDatagramsSent() const noexcept { return datagramsSent_; }
            /**
             * @brief Get the Datagrams Received object
             *
             * @return uint64_t
             */
            [[nodiscard]] uint64_t GetDatagramsReceived() const noexcept { return datagramsReceived_; }

            /**
             * @brief Get the Sending object
             *
             * @return true
             * @return false
             */
            [[nodiscard]] bool IsSending() const noexcept { return sending_; }
            /**
             * @brief Get the Receiving object
             *
             * @return true
             * @return false
             */
            [[nodiscard]] bool IsReceiving() const noexcept { return receiving_; }

           protected:
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

            uint16_t port_;  ///< Port to listen on

            asio::io_context& context_;  ///< ASIO context for networking operations

            asio::ip::udp::socket socket_;         ///< ASIO socket
            asio::ip::udp::endpoint endpoint_;     ///< Remote endpoint
            std::atomic<bool> resolving_ = false;  ///< Resolve flag
            std::atomic<bool> connected_ = false;  ///< Connected flag

            std::vector<uint8_t> receiveBuffer_;  ///< Receive buffer
            size_t receiveBufferLimit_{0};        ///< Receive buffer limit

            bool sending_ = false;    ///< Sending flag
            bool receiving_ = false;  ///< Receiving flag

            // Transfer statistic
            uint64_t bytesSending_;       ///< Bytes sending
            uint64_t bytesSent_;          ///< Bytes sent
            uint64_t bytesReceived_;      ///< Bytes received
            uint64_t datagramsSent_;      ///< Datagrams sent
            uint64_t datagramsReceived_;  ///< Datagrams received

            /**
             * @brief Disconnect the client (internal)
             *
             * @return true
             * @return false
             */
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

            /**
             * @brief Clear the send/receive buffers
             *
             */
            void ClearBuffers() {
                receiveBuffer_.clear();

                bytesSending_ = 0;
            }

            /**
             * @brief Send error
             *
             * @param ec The error code
             */
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
