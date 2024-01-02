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
        template <typename T>
        class ServerInterface;

        template <typename T>
        class TcpConnection;

        template <typename MessageType>
        class AConnection {
           public:
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

            virtual ~AConnection() = default;

            [[nodiscard]] uint32_t GetID() const {
                return id_;
            }

            [[nodiscard]] owner GetOwner() const {
                return connectionOwner_;
            }

            void ConnectToClient(RType::net::ServerInterface<MessageType>* server, uint32_t uid = 0) {
                if (connectionOwner_ == owner::server) {
                    if (tcpSocket.is_open()) {
                        id_ = uid;

                        WriteValidation();

                        ReadValidation(server);
                    }
                }
            }
            /*
                @brief Disconnect from server
            */
            void Disconnect() {
                if (IsConnected()) {
                    asio::post(asioContext_, [this]() {
                        tcpSocket.close();
                    });
                }
            }

            /*
                @brief Check if the client is connected to a server
                @return True if the client is connected to a server, false otherwise
            */
            [[nodiscard]] bool IsConnected() const {
                    return tcpSocket.is_open();
            }

            /*
                @brief Start listening for incoming messages
            */
            void StartListening() {
            }

            virtual void Send(const message<MessageType>& msg) = 0;

           private:
            virtual void WriteHeader() = 0;

            virtual void WriteBody() = 0;

            virtual void ReadHeader() = 0;

            virtual void ReadBody() = 0;

            virtual void WriteValidation() = 0;

            virtual void ReadValidation(RType::net::ServerInterface<MessageType>* server = nullptr) = 0;

           protected:
            virtual void AddToIncomingMessageQueue() = 0;

            uint64_t scramble(uint64_t input) {
                uint64_t out = input ^ 0xDEADBEEFC0DECAFE;
                out = (out & 0xF0F0F0F0F0F0F0) >> 4 | (out & 0x0F0F0F0F0F0F0F) << 4;
                return out ^ 0xC0DEFACE12345678;
            }

           protected:
            owner connectionOwner_;

            asio::io_context& asioContext_;
            asio::ip::tcp::socket tcpSocket;

            TsQueue<message<MessageType>> outgoingMessages_;
            TsQueue<owned_message<MessageType, TcpConnection<MessageType>>>& incomingTcpMessages_;
            message<MessageType> tempIncomingMessage_;

            uint64_t handshakeOut_ = 0;
            uint64_t handshakeIn_ = 0;
            uint64_t handshakeCheck_ = 0;
            uint32_t id_ = 0;
        };
    }  // namespace net
}  // namespace RType
