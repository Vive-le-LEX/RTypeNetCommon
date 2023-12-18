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

#include "NetAConnection.hpp"
#include "NetServer.hpp"

namespace RType {
    namespace net {
        template <typename T>
        class ServerInterface;

        template <typename MessageType>
        class UdpConnection : public std::enable_shared_from_this<UdpConnection<MessageType>> {
        public:
            UdpConnection(owner parent,
                          asio::io_context& context,
                          asio::ip::udp::endpoint endpoint,
                          TsQueue<owned_message<MessageType, UdpConnection<MessageType>>>& incomingMessages) : asioContext(context),
                                                                                                               udpEndpoint(std::move(endpoint)),
                                                                                                               incomingUdpMessages(incomingMessages),
                                                                                                               udpSocket(context)
            {
                connectionOwner = parent;
                udpSocket.open(udpEndpoint.protocol());
                if (connectionOwner == owner::server) {
                    udpSocket.bind(udpEndpoint);
                } else {
                    udpSocket.bind(asio::ip::udp::endpoint(udpEndpoint.protocol(), 0));
                }
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

            [[nodiscard]] bool IsConnected() const {
                return udpSocket.is_open();
            }

            [[nodiscard]] uint32_t GetID() const {
                return id;
            }

            [[nodiscard]] asio::ip::udp::endpoint GetEndpoint() const {
                return udpEndpoint;
            }

            void ConnectToClient(RType::net::ServerInterface<MessageType>* server, uint32_t uid = 0) {
                if (connectionOwner == owner::server) {
                    if (udpSocket.is_open()) {
                        id = uid;
                        ReadValidation();
                    }
                }
            }

            void ConnectToServer() {
                if (connectionOwner == owner::client) {
                    WriteValidation();
                }
            }

            void Disconnect() {
                if (connectionOwner == owner::client) {
                    std::cout << "Client Disconnected" << std::endl;
                    udpSocket.close();
                }
            }

            void Send(const message<MessageType>& msg) {
                asio::post(asioContext,
                           [this, msg]() {
                               bool writingMessage = !outgoingMessages.empty();
                               outgoingMessages.push_back(msg);
                               if (!writingMessage) {
                                   WriteHeader();
                               }
                           });
            }

        private:
            uint64_t scramble(uint64_t input) {
                uint64_t out = input ^ 0xDEADBEEFC0DECAFE;
                out = (out & 0xF0F0F0F0F0F0F0) >> 4 | (out & 0x0F0F0F0F0F0F0F) << 4;
                return out ^ 0xC0DEFACE12345678;
            }

            void WriteHeader() {
                udpSocket.async_send_to(asio::buffer(&outgoingMessages.front().header, sizeof(message_header<MessageType>)), udpEndpoint,
                                        [this](std::error_code ec, std::size_t length) {
                                            (void)length;
                                            if (!ec) {
                                                if (outgoingMessages.front().body.size() > 0) {
                                                    WriteBody();
                                                } else {
                                                    outgoingMessages.pop_front();
                                                    if (!outgoingMessages.empty()) {
                                                        WriteHeader();
                                                    }
                                                }
                                            } else {
                                                std::cout << "[Error UDP][" << id << "] Write header failed: " << ec.message() << std::endl;
                                                udpSocket.close();
                                            }
                                        });
            }

            void WriteBody() {
                udpSocket.async_send_to(asio::buffer(outgoingMessages.front().body.data(), outgoingMessages.front().body.size()), udpEndpoint,
                                        [this](std::error_code ec, std::size_t length) {
                                            (void)length;
                                            if (!ec) {
                                                outgoingMessages.pop_front();
                                                if (!outgoingMessages.empty()) {
                                                    WriteHeader();
                                                }
                                            } else {
                                                std::cout << "[Error UDP][" << id << "] Write body failed: " << ec.message() << std::endl;
                                                udpSocket.close();
                                            }
                                        });
            }

            void ReadHeader() {
                udpSocket.async_receive_from(asio::buffer(&tempIncomingMessage.header, sizeof(message_header<MessageType>)), udpEndpoint,
                                             [this](std::error_code ec, std::size_t length) {
                                                 (void)length;
                                                 if (!ec) {
                                                     if (tempIncomingMessage.header.size > 0) {
                                                         tempIncomingMessage.body.resize(tempIncomingMessage.header.size);
                                                         ReadBody();
                                                     } else {
                                                         AddToIncomingMessageQueue();
                                                     }
                                                 } else {
                                                     std::cout << "[Error UDP][" << id << "] Read header failed: " << ec.message() << std::endl;
                                                     udpSocket.close();
                                                 }
                                             });
            }

            void ReadBody() {
                udpSocket.async_receive_from(asio::buffer(tempIncomingMessage.body.data(), tempIncomingMessage.body.size()), udpEndpoint,
                                             [this](std::error_code ec, std::size_t length) {
                                                 (void)length;
                                                 if (!ec) {
                                                     AddToIncomingMessageQueue();
                                                 } else {
                                                     std::cout << "[Error UDP][" << id << "] Read body failed: " << ec.message() << std::endl;
                                                     udpSocket.close();
                                                 }
                                             });
            }

            void WriteValidation() {
                udpSocket.async_send_to(asio::buffer(&handshakeOut, sizeof(uint64_t)), udpEndpoint,
                                        [this](std::error_code ec, std::size_t length) {
                                            (void)length;
                                            if (!ec) {
                                                if (connectionOwner == owner::client) {
                                                    ReadHeader();
                                                }
                                            } else {
                                                std::cout << "[Error UDP][" << id << "] Write validation failed: " << ec.message() << std::endl;
                                                udpSocket.close();
                                            }
                                        });
            }

            virtual void ReadValidation(RType::net::ServerInterface<MessageType>* server = nullptr) final {
                if (connectionOwner == owner::server) {
                    AsyncTimer::GetInstance()->addTimer(id, 1000, [this, server]() {
                        std::cout << "Client Timed out while reading validation" << std::endl;
                        udpSocket.close();
                    });
                }
                udpSocket.async_receive_from(asio::buffer(&handshakeIn, sizeof(uint64_t)), udpEndpoint,
                                             [this, server](std::error_code ec, std::size_t length) {
                                                 (void)length;
                                                 if (!ec) {
                                                     if (connectionOwner == owner::server) {
                                                         // Connection is a server, so check response from client

                                                         // Compare sent data to actual solution
                                                         if (handshakeIn == handshakeCheck) {
                                                             AsyncTimer::GetInstance()->removeTimer(id);
                                                             // Client has provided valid solution, so allow it to connect properly
                                                             std::cout << "Client Validated" << std::endl;
                                                             // TODO: UDP
                                                             //  server->OnClientValidated(this->shared_from_this());

                                                             // Sit waiting to receive data now
                                                             ReadHeader();
                                                         } else {
                                                             // Client gave incorrect data, so disconnect
                                                             std::cout << "Client Disconnected (Fail Validation)" << std::endl;
                                                             udpSocket.close();
                                                         }
                                                     } else {
                                                         // Connection is a client, so solve puzzle
                                                         handshakeOut = scramble(handshakeIn);

                                                         // Write the result
                                                         WriteValidation();
                                                     }
                                                 } else {
                                                     // Some bigger failure occurred
                                                     std::cout << "Client Disconnected (ReadValidation)" << std::endl;
                                                     udpSocket.close();
                                                 }
                                             });
            }

            void AddToIncomingMessageQueue() {
                if (connectionOwner == owner::server) {
                    owned_message<MessageType, UdpConnection<MessageType>> msg;
                    msg.remote = this->shared_from_this();
                    msg.msg = tempIncomingMessage;
                    incomingUdpMessages.push_back(msg);
                } else {
                    owned_message<MessageType, UdpConnection<MessageType>> msg{nullptr, tempIncomingMessage};
                    incomingUdpMessages.push_back(msg);
                }

                ReadHeader();
            }

            owner connectionOwner;

            asio::io_context& asioContext;
            asio::ip::udp::socket udpSocket;
            asio::ip::udp::endpoint udpEndpoint;

            TsQueue<message<MessageType>> outgoingMessages;
            TsQueue<owned_message<MessageType, UdpConnection<MessageType>>>& incomingUdpMessages;
            message<MessageType> tempIncomingMessage;

            uint64_t handshakeOut = 0;
            uint64_t handshakeIn = 0;
            uint64_t handshakeCheck = 0;
            uint32_t id = 0;
        };
    }  // namespace net
}  // namespace RType
