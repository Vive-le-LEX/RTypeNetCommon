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
#include "NetServer.hpp"
#include "NetTsqueue.hpp"

namespace RType {
    namespace net {
        template <typename T>
        class ServerInterface;

        template <typename MessageType>
        class AConnection {
           public:
            AConnection(owner parent,
                        asio::io_context& context,
                        asio::ip::tcp::socket socket,
                        TsQueue<owned_message<MessageType, TcpConnection<MessageType>>>& incomingMessages) : asioContext(context),
                                                                                                             tcpSocket(std::move(socket)),
                                                                                                             incomingTcpMessages(incomingMessages) {
                connectionOwner = parent;
                connectionType = connection_type::tcp;
                AsyncTimer::Construct();
                if (connectionOwner == owner::server) {
                    handshakeOut = uint64_t(std::chrono::system_clock::now().time_since_epoch().count());

                    handshakeCheck = scramble(handshakeOut);
                } else {
                    // Connection is Client -> Server, so we have nothing to define,
                    handshakeIn = 0;
                    handshakeOut = 0;
                }
            }

            virtual ~AConnection() = default;

            [[nodiscard]] uint32_t GetID() const {
                return id;
            }

            [[nodiscard]] connection_type GetType() const {
                return connectionType;
            }

            [[nodiscard]] owner GetOwner() const {
                return connectionOwner;
            }

            void ConnectToClient(RType::net::ServerInterface<MessageType>* server, uint32_t uid = 0) {
                if (connectionOwner == owner::server) {
                    if (connectionType == connection_type::tcp) {
                        if (tcpSocket.is_open()) {
                            id = uid;

                            WriteValidation();

                            ReadValidation(server);
                        }
                    } else {
                        if (udpSocket->is_open()) {
                            id = uid;

                            //TODO: UDP
                        }
                    }
                }
            }
            /*
                @brief Disconnect from server
            */
            void Disconnect() {
                if (IsConnected()) {
                    if (connectionType == connection_type::tcp) {
                        asio::post(asioContext, [this]() {
                            tcpSocket.close();
                        });
                    } else {
                        asio::post(asioContext, [this]() {
                            udpSocket->close();
                        });
                    }
                }
            }

            /*
                @brief Check if the client is connected to a server
                @return True if the client is connected to a server, false otherwise
            */
            [[nodiscard]] bool IsConnected() const {
                if (connectionType == connection_type::tcp)
                    return tcpSocket.is_open();
                else
                    return udpSocket->is_open();
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
            owner connectionOwner;
            connection_type connectionType;

            asio::io_context& asioContext;
            asio::ip::tcp::socket tcpSocket;
            std::unique_ptr<asio::ip::udp::socket> udpSocket;

            TsQueue<message<MessageType>> outgoingMessages;
            TsQueue<owned_message<MessageType, TcpConnection<MessageType>>>& incomingTcpMessages;
            message<MessageType> tempIncomingMessage;

            uint64_t handshakeOut = 0;
            uint64_t handshakeIn = 0;
            uint64_t handshakeCheck = 0;
            uint32_t id = 0;
        };
    }  // namespace net
}  // namespace RType
