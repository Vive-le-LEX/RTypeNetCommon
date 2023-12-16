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
        class TcpConnection : public AConnection<MessageType> {
           public:
            TcpConnection(owner parent,
                          asio::io_context& context,
                          asio::basic_stream_socket<asio::ip::tcp> socket,
                          TsQueue<owned_message<MessageType, TcpConnection<MessageType>>>& incomingMessages) : AConnection<MessageType>(parent, context, std::move(socket), incomingMessages) {}

            virtual ~TcpConnection() = default;

            void ConnectToServer(const asio::ip::tcp::resolver::results_type& endpoints) {
                if (this->connectionOwner == owner::server) {
                    throw std::runtime_error("Cannot connect a server to a server");
                }

                asio::async_connect(this->tcpSocket, endpoints,
                                    [this](std::error_code ec, asio::ip::tcp::endpoint& endpoint) {
                                        (void)endpoint;
                                        if (!ec) {
                                            this->ReadValidation();
                                        } else {
                                            std::cout << "[Error] Connection to server failed: " << ec.message() << std::endl;
                                            this->tcpSocket.close();
                                        }
                                    });
            }

            void Send(const message<MessageType>& msg) override {
                asio::post(this->asioContext,
                           [this, msg]() {
                               bool writingMessage = !this->outgoingMessages.empty();
                               this->outgoingMessages.push_back(msg);
                               if (!writingMessage) {
                                   WriteHeader();
                               }
                           });
            }

           private:
            virtual void WriteHeader() final {
                asio::async_write(this->tcpSocket,
                                  asio::buffer(&this->outgoingMessages.front().header, sizeof(message_header<MessageType>)),
                                  [this](std::error_code ec, std::size_t length) {
                                      (void)length;
                                      if (!ec) {
                                          if (this->outgoingMessages.front().body.size() > 0) {
                                              WriteBody();
                                          } else {
                                              this->outgoingMessages.pop_front();
                                              if (!this->outgoingMessages.empty()) {
                                                  WriteHeader();
                                              }
                                          }
                                      } else {
                                          std::cout << "[Error][" << this->id << "] Write header failed: " << ec.message() << std::endl;
                                          this->tcpSocket.close();
                                      }
                                  });
            }

            virtual void WriteBody() final {
                asio::async_write(this->tcpSocket,
                                  asio::buffer(this->outgoingMessages.front().body.data(), this->outgoingMessages.front().body.size()),
                                  [this](std::error_code ec, std::size_t length) {
                                      (void)length;
                                      if (!ec) {
                                          this->outgoingMessages.pop_front();
                                          if (!this->outgoingMessages.empty()) {
                                              WriteHeader();
                                          }
                                      } else {
                                          std::cout << "[Error][" << this->id << "] Write body failed: " << ec.message() << std::endl;
                                          this->tcpSocket.close();
                                      }
                                  });
            }

            virtual void ReadHeader() final {
                asio::async_read(this->tcpSocket,
                                 asio::buffer(&this->tmpIncomingMessage.header, sizeof(message_header<MessageType>)),
                                 [this](std::error_code ec, std::size_t length) {
                                     (void)length;
                                     if (!ec) {
                                         if (this->tmpIncomingMessage.header.size > 0) {
                                             this->tmpIncomingMessage.body.resize(this->tmpIncomingMessage.header.size);
                                             ReadBody();
                                         } else {
                                             this->AddToIncomingMessages();
                                         }
                                     } else {
                                         std::cout << "[Error][" << this->id << "] Read header failed: " << ec.message() << std::endl;
                                         this->tcpSocket.close();
                                     }
                                 });
            }

            virtual void ReadBody() final {
                asio::async_read(this->tcpSocket,
                                 asio::buffer(this->tmpIncomingMessage.body.data(), this->tmpIncomingMessage.body.size()),
                                 [this](std::error_code ec, std::size_t length) {
                                     (void)length;
                                     if (!ec) {
                                         this->AddToIncomingMessages();
                                     } else {
                                         std::cout << "[Error][" << this->id << "] Read body failed: " << ec.message() << std::endl;
                                         this->tcpSocket.close();
                                     }
                                 });
            }

            virtual void WriteValidation() final {
                asio::async_write(this->tcpSocket,
                                  asio::buffer(&this->handshakeOut, sizeof(uint64_t)),
                                  [this](std::error_code ec, std::size_t length) {
                                      (void)length;
                                      if (!ec) {
                                          if (this->connectionOwner == owner::client) {
                                              this->ReadHeader();
                                          }
                                      } else {
                                          std::cout << "[Error][" << this->id << "] Write validation failed: " << ec.message() << std::endl;
                                          this->tcpSocket.close();
                                      }
                                  });
            }

            virtual void ReadValidation(const RType::net::ServerInterface<MessageType>* server) final {
                if (this->connectionOwner == owner::server) {
                    AsyncTimer::GetInstance()->addTimer(this->id, 1000, [this, server]() {
                        std::cout << "Client Timed out while reading validation" << std::endl;
                        this->tcpSocket.close();
                    });
                }

                asio::async_read(this->tcpSocket, asio::buffer(&this->handshakeIn, sizeof(uint64_t)),
                                 [this, server](std::error_code ec, std::size_t length) {
                                     (void)length;
                                     if (!ec) {
                                         if (this->connectionOwner == owner::server) {
                                             // Connection is a server, so check response from client

                                             // Compare sent data to actual solution
                                             if (this->handshakeIn == this->handshakeCheck) {
                                                 AsyncTimer::GetInstance()->removeTimer(this->id);
                                                 // Client has provided valid solution, so allow it to connect properly
                                                 std::cout << "Client Validated" << std::endl;
                                                 server->OnClientValidated(this->shared_from_this());

                                                 // Sit waiting to receive data now
                                                 ReadHeader();
                                             } else {
                                                 // Client gave incorrect data, so disconnect
                                                 std::cout << "Client Disconnected (Fail Validation)" << std::endl;
                                                 this->tcpSocket.close();
                                             }
                                         } else {
                                             // Connection is a client, so solve puzzle
                                             this->handshakeOut = this->scramble(this->handshakeIn);

                                             // Write the result
                                             WriteValidation();
                                         }
                                     } else {
                                         // Some bigger failure occurred
                                         std::cout << "Client Disconnected (ReadValidation)" << std::endl;
                                         this->tcpSocket.close();
                                     }
                                 });
            }
        };

    }  // namespace net
}  // namespace RType
