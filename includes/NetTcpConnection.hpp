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
        class TcpConnection : public AConnection<MessageType>, public std::enable_shared_from_this<TcpConnection<MessageType>> {
           public:
            TcpConnection(owner parent,
                          asio::io_context& context,
                          asio::ip::tcp::socket socket,
                          TsQueue<owned_message<MessageType, TcpConnection<MessageType>>>& incomingMessages) : AConnection<MessageType>(parent, context, std::move(socket), incomingMessages) {}

            virtual ~TcpConnection() = default;

            void ConnectToServer(const asio::ip::tcp::resolver::results_type& endpoints) {
                if (this->connectionOwner_ == owner::server) {
                    throw std::runtime_error("Cannot connect a server to a server");
                }

                asio::async_connect(this->tcpSocket, endpoints,
                                    [this](std::error_code ec, const asio::ip::tcp::endpoint& endpoint) {
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
                asio::post(this->asioContext_,
                           [this, msg]() {
                               bool writingMessage = !this->outgoingMessages_.empty();
                               this->outgoingMessages_.push_back(msg);
                               if (!writingMessage) {
                                   WriteHeader();
                               }
                           });
            }

           private:
            virtual void WriteHeader() final {
                asio::async_write(this->tcpSocket,
                                  asio::buffer(&this->outgoingMessages_.front().header, sizeof(message_header<MessageType>)),
                                  [this](std::error_code ec, std::size_t length) {
                                      (void)length;
                                      if (!ec) {
                                          if (this->outgoingMessages_.front().body.size() > 0) {
                                              WriteBody();
                                          } else {
                                              this->outgoingMessages_.pop_front();
                                              if (!this->outgoingMessages_.empty()) {
                                                  WriteHeader();
                                              }
                                          }
                                      } else {
                                          std::cout << "[Error][" << this->id_ << "] Write header failed: " << ec.message() << std::endl;
                                          this->tcpSocket.close();
                                      }
                                  });
            }

            virtual void WriteBody() final {
                asio::async_write(this->tcpSocket,
                                  asio::buffer(this->outgoingMessages_.front().body.data(), this->outgoingMessages_.front().body.size()),
                                  [this](std::error_code ec, std::size_t length) {
                                      (void)length;
                                      if (!ec) {
                                          this->outgoingMessages_.pop_front();
                                          if (!this->outgoingMessages_.empty()) {
                                              WriteHeader();
                                          }
                                      } else {
                                          std::cout << "[Error][" << this->id_ << "] Write body failed: " << ec.message() << std::endl;
                                          this->tcpSocket.close();
                                      }
                                  });
            }

            virtual void ReadHeader() final {
                asio::async_read(this->tcpSocket,
                                 asio::buffer(&this->tempIncomingMessage_.header, sizeof(message_header<MessageType>)),
                                 [this](std::error_code ec, std::size_t length) {
                                     (void)length;
                                     if (!ec) {
                                         if (this->tempIncomingMessage_.header.size > 0) {
                                             this->tempIncomingMessage_.body.resize(this->tempIncomingMessage_.header.size);
                                             ReadBody();
                                         } else {
                                             this->AddToIncomingMessageQueue();
                                         }
                                     } else if (ec != asio::error::eof) {
                                         std::cout << "[Error][" << this->id_ << "] Read header failed: " << ec.message() << std::endl;
                                         this->tcpSocket.close();
                                     }
                                 });
            }

            virtual void ReadBody() final {
                asio::async_read(this->tcpSocket,
                                 asio::buffer(this->tempIncomingMessage_.body.data(), this->tempIncomingMessage_.body.size()),
                                 [this](std::error_code ec, std::size_t length) {
                                     (void)length;
                                     if (!ec) {
                                         this->AddToIncomingMessageQueue();
                                     } else {
                                         std::cout << "[Error][" << this->id_ << "] Read body failed: " << ec.message() << std::endl;
                                         this->tcpSocket.close();
                                     }
                                 });
            }

            virtual void WriteValidation() final {
                asio::async_write(this->tcpSocket,
                                  asio::buffer(&this->handshakeOut_, sizeof(uint64_t)),
                                  [this](std::error_code ec, std::size_t length) {
                                      (void)length;
                                      if (!ec) {
                                          if (this->connectionOwner_ == owner::client) {
                                              this->ReadHeader();
                                          }
                                      } else {
                                          std::cout << "[Error][" << this->id_ << "] Write validation failed: " << ec.message() << std::endl;
                                          this->tcpSocket.close();
                                      }
                                  });
            }

            virtual void ReadValidation(RType::net::ServerInterface<MessageType>* server = nullptr) final {
                if (this->connectionOwner_ == owner::server) {
                    AsyncTimer::GetInstance()->addTimer(this->id_, 1000, [this, server]() {
                        std::cout << "Client Timed out while reading validation" << std::endl;
                        this->tcpSocket.close();
                    });
                }

                asio::async_read(this->tcpSocket, asio::buffer(&this->handshakeIn_, sizeof(uint64_t)),
                                 [this, server](std::error_code ec, std::size_t length) {
                                     (void)length;
                                     if (!ec) {
                                         if (this->connectionOwner_ == owner::server) {
                                             // Connection is a server, so check response from client

                                             // Compare sent data to actual solution
                                             if (this->handshakeIn_ == this->handshakeCheck_) {
                                                 AsyncTimer::GetInstance()->removeTimer(this->id_);
                                                 // Client has provided valid solution, so allow it to connect properly
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
                                             this->handshakeOut_ = this->scramble(this->handshakeIn_);

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

            virtual void AddToIncomingMessageQueue() final {
                if (this->connectionOwner_ == owner::server) {
                    owned_message<MessageType, TcpConnection<MessageType>> msg;
                    msg.remote = this->shared_from_this();
                    msg.msg = this->tempIncomingMessage_;
                    this->incomingTcpMessages_.push_back(msg);
                } else {
                    owned_message<MessageType, TcpConnection<MessageType>> msg{nullptr, this->tempIncomingMessage_};
                    this->incomingTcpMessages_.push_back(msg);
                }

                ReadHeader();
            }
        };

    }  // namespace net
}  // namespace RType
