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
#include "NetTsqueue.hpp"

namespace RType {
    namespace net {
        /// @private
        template <typename T>
        class ServerInterface;

        /// @private
        template <typename T>
        class TcpConnection;

        /**
         * @brief Abstract class for a connection
         * @tparam MessageType The enum class containing all the message types
         */
        template <typename MessageType>
        class AConnection {
           public:
            /**
             * @brief Construct a new AConnection object
             *
             * @param parent The owner of the connection
             * @param context The asio context
             * @param socket The asio socket
             * @param incomingMessages The incoming message queue
             */
            AConnection(owner parent,
                        asio::io_context& context,
                        asio::ip::tcp::socket socket,
                        TsQueue<owned_message<MessageType, TcpConnection<MessageType>>>& incomingMessages) : asioContext_(context),
                                                                                                             tcpSocket(std::move(socket)),
                                                                                                             incomingTcpMessages_(incomingMessages) {
                connectionOwner_ = parent;
                AsyncTimer::Construct();
                if (connectionOwner_ == owner::server) {
                    handshakeOut_ = uint64_t(std::chrono::system_clock::now().time_since_epoch().count());

                    handshakeCheck_ = scramble(handshakeOut_);
                } else {
                    // Connection is Client -> Server, so we have nothing to define,
                    handshakeIn_ = 0;
                    handshakeOut_ = 0;
                }
            }

            /**
             * @brief Destroy the AConnection object
             */
            virtual ~AConnection() = default;

            /**
             * @brief Get the id of the connection
             * @return id The id of the connection
             */
            [[nodiscard]] uint32_t GetID() const {
                return id_;
            }

            /**
             * @brief Get the owner of the connection
             * @return owner The owner of the connection
             */
            [[nodiscard]] owner GetOwner() const {
                return connectionOwner_;
            }

            /**
             * @brief Connect to a server
             * @param server The server to connect from
             * @param uid The id of the connection
             */
            void ConnectToClient(RType::net::ServerInterface<MessageType>* server, uint32_t uid = 0) {
                if (connectionOwner_ == owner::server) {
                    if (tcpSocket.is_open()) {
                        id_ = uid;

                        WriteValidation();

                        ReadValidation(server);
                    }
                }
            }

            /**
             * @brief Disconnect the connection
             *
             */
            void Disconnect() {
                if (IsConnected()) {
                    asio::post(asioContext_, [this]() {
                        tcpSocket.close();
                    });
                }
            }

            /**
             * @brief Check if the connection is connected
             * @return bool True if the connection is connected, false otherwise
             */
            [[nodiscard]] bool IsConnected() const {
                return tcpSocket.is_open();
            }

            /// @private
            void StartListening() {
            }

            /**
             * @brief Send a message
             *
             * @param msg The message to send
             */
            virtual void Send(const message<MessageType>& msg) = 0;

           private:
            virtual void WriteHeader() = 0;

            virtual void WriteBody() = 0;

            virtual void ReadHeader() = 0;

            virtual void ReadBody() = 0;

            virtual void WriteValidation() = 0;

            virtual void ReadValidation(RType::net::ServerInterface<MessageType>* server = nullptr) = 0;

           protected:
            /**
             * @brief add a message to the incoming message queue
             */
            virtual void AddToIncomingMessageQueue() = 0;

            /**
             * @brief scramble the input
             * @param input the input to scramble
             * @return uint64_t the scrambled input
             */
            uint64_t scramble(uint64_t input) {
                uint64_t out = input ^ 0xDEADBEEFC0DECAFE;
                out = (out & 0xF0F0F0F0F0F0F0) >> 4 | (out & 0x0F0F0F0F0F0F0F) << 4;
                return out ^ 0xC0DEFACE12345678;
            }

           protected:
            owner connectionOwner_;  ///< The owner of the connection

            asio::io_context& asioContext_;   ///< The asio context
            asio::ip::tcp::socket tcpSocket;  ///< The asio socket

            TsQueue<message<MessageType>> outgoingMessages_;                                        ///< The outgoing message queue
            TsQueue<owned_message<MessageType, TcpConnection<MessageType>>>& incomingTcpMessages_;  ///< The incoming message queue
            message<MessageType> tempIncomingMessage_;                                              ///< The temporary incoming message

            uint64_t handshakeOut_ = 0;    ///< The outgoing handshake
            uint64_t handshakeIn_ = 0;     ///< The incoming handshake
            uint64_t handshakeCheck_ = 0;  ///< The handshake check
            uint32_t id_ = 0;              ///< The connection id
        };
    }  // namespace net
}  // namespace RType
