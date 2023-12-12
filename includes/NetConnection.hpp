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
        /*
            @brief The connection class is responsible for managing the socket connection,
            sending and receiving messages, and handling the asynchronous operations.
            On both sides of the connection, client and server, the interface is the same:
            ConnectToServer() and ConnectToClient() respectively. The template argument
            T is the type of message that this connection will be handling. It is used
            to ensure that the messages sent and received are valid at compile time.

            @tparam T Message type
        */
        template <typename T>
        class connection : public std::enable_shared_from_this<connection<T>> {
           public:
            /*
                @enum owner
                @brief Defines who owns the connection - a client or the server
            */
            enum class owner {
                server,
                client
            };

            /*
                @brief Constructor: Specify Owner, connect to context, transfer the socket
                Provide reference to incoming message queue

                @param parent Owner type
                @param asioContext Reference to ASIO context object this connection will be
                attached to
                @param socket Socket connection
                @param qIn Reference to incoming message queue
            */
            connection(owner parent, asio::io_context &asioContext, asio::ip::tcp::socket incomingSocket,
                       TsQueue<owned_message<T>> &qIn)
                : asioContext(asioContext), _socket(std::move(incomingSocket)), incomingMessages(qIn) {
                ownerType = parent;
                AsyncTimer::Construct();
                if (ownerType == owner::server) {
                    handshakeOut = uint64_t(std::chrono::system_clock::now().time_since_epoch().count());

                    handshakeCheck = scramble(handshakeOut);
                } else {
                    // Connection is Client -> Server, so we have nothing to define,
                    handshakeIn = 0;
                    handshakeOut = 0;
                }
            }

            virtual ~connection() = default;

            /*
                @brief Get the ID of this connection
                @return uint32_t ID of this connection
            */
            [[nodiscard]] uint32_t GetID() const {
                return id;
            }

            /*
                @brief Connect to a client
                @param uid ID of the client
            */
            void ConnectToClient(RType::net::ServerInterface<T> *server, uint32_t uid = 0) {
                if (ownerType == owner::server) {
                    if (_socket.is_open()) {
                        id = uid;

                        WriteValidation();

                        ReadValidation(server);
                    }
                }
            }

            /*
                @brief Connect to a server
                @param endpoints Endpoints of the server
            */
            void ConnectToServer(const asio::ip::tcp::resolver::results_type &endpoints) {
                if (ownerType == owner::client) {
                    asio::async_connect(_socket, endpoints,
                                        [this](std::error_code ec, asio::ip::tcp::endpoint endpoint) {
                                            if (!ec) {
                                                ReadValidation();
                                            }
                                        });
                }
            }

            /*
                @brief Disconnect from server
            */
            void Disconnect() {
                if (IsConnected())
                    asio::post(asioContext, [this]() { _socket.close(); });
            }

            /*
                @brief Check if this connection is still active
                @return true if this connection is still active, false otherwise
            */
            [[nodiscard]] bool IsConnected() const {
                return _socket.is_open();
            }

            /*
                @brief Start listening for incoming messages
            */
            void StartListening() {
            }

            /*
                @brief Send a message to either a client or a server depending on
                the owner type
                @param msg Message to send
            */
            void Send(const message<T> &msg) {
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
            /*
                @brief Write a message header, this function is Asynchronous
            */
            void WriteHeader() {
                asio::async_write(_socket, asio::buffer(&outgoingMessages.front().header, sizeof(message_header<T>)),
                                  [this](std::error_code ec, std::size_t length) {
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
                                          std::cout << "[" << id << "] Write Header Fail.\n";
                                          _socket.close();
                                      }
                                  });
            }

            /*
                @brief Write a message body, this function is Asynchronous
            */
            void WriteBody() {
                asio::async_write(_socket, asio::buffer(outgoingMessages.front().body.data(), outgoingMessages.front().body.size()),
                                  [this](std::error_code ec, std::size_t length) {
                                      if (!ec) {
                                          outgoingMessages.pop_front();

                                          if (!outgoingMessages.empty()) {
                                              WriteHeader();
                                          }
                                      } else {
                                          std::cout << "[" << id << "] Write Body Fail.\n";
                                          _socket.close();
                                      }
                                  });
            }

            /*
                @brief Read a message header, this function is Asynchronous.
                If this function is called, we are expecting asio to wait until it receives
                enough bytes to form a header of a message.
            */
            void ReadHeader() {
                asio::async_read(_socket, asio::buffer(&tempIncomingMessage.header, sizeof(message_header<T>)),
                                 [this](std::error_code ec, std::size_t length) {
                                     if (!ec) {
                                         if (tempIncomingMessage.header.size > 0) {
                                             tempIncomingMessage.body.resize(tempIncomingMessage.header.size);
                                             ReadBody();
                                         } else {
                                             AddToIncomingMessageQueue();
                                         }
                                     } else {
                                         std::cout << "[" << id << "] Read Header Fail.\n";
                                         _socket.close();
                                     }
                                 });
            }

            /*
                @brief Read a message body, this function is Asynchronous.
                If this function is called, a header has already been read, and in that header
                request we read a body, of the specified size, into the message buffer.
            */
            void ReadBody() {
                asio::async_read(_socket,
                                 asio::buffer(tempIncomingMessage.body.data(), tempIncomingMessage.body.size()),
                                 [this](std::error_code ec, std::size_t length) {
                                     if (!ec) {
                                         AddToIncomingMessageQueue();
                                     } else {
                                         std::cout << "[" << id << "] Read Body Fail.\n";
                                         _socket.close();
                                     }
                                 });
            }

            uint64_t scramble(uint64_t input) {
                uint64_t out = input ^ 0xDEADBEEFC0DECAFE;
                out = (out & 0xF0F0F0F0F0F0F0) >> 4 | (out & 0x0F0F0F0F0F0F0F) << 4;
                return out ^ 0xC0DEFACE12345678;
            }

            void WriteValidation() {
                asio::async_write(_socket, asio::buffer(&handshakeOut, sizeof(uint64_t)),
                                  [this](std::error_code ec, std::size_t length) {
                                      if (!ec) {
                                          // Validation data sent, clients should sit and wait
                                          // for a response (or a closure)
                                          if (ownerType == owner::client)
                                              ReadHeader();
                                      } else {
                                          _socket.close();
                                      }
                                  });
            }

            void ReadValidation(RType::net::ServerInterface<T> *server = nullptr) {
                if (ownerType == owner::server) {
                    AsyncTimer::GetInstance()->addTimer(id, 1000, [this, server]() {
                        std::cout << "Client Timed out while reading validation" << std::endl;
                        _socket.close();
                    });
                }
                asio::async_read(_socket, asio::buffer(&handshakeIn, sizeof(uint64_t)),
                                 [this, server](std::error_code ec, std::size_t length) {
                                     if (!ec) {
                                         if (ownerType == owner::server) {
                                             // Connection is a server, so check response from client

                                             // Compare sent data to actual solution
                                             if (handshakeIn == handshakeCheck) {
                                                 AsyncTimer::GetInstance()->removeTimer(id);
                                                 // Client has provided valid solution, so allow it to connect properly
                                                 std::cout << "Client Validated" << std::endl;
                                                 server->OnClientValidated(this->shared_from_this());

                                                 // Sit waiting to receive data now
                                                 ReadHeader();
                                             } else {
                                                 // Client gave incorrect data, so disconnect
                                                 std::cout << "Client Disconnected (Fail Validation)" << std::endl;
                                                 _socket.close();
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
                                         _socket.close();
                                     }
                                 });
            }

            /*
                @brief Add a message to the incoming queue when it is fully received
            */
            void AddToIncomingMessageQueue() {
                if (ownerType == owner::server)
                    incomingMessages.push_back({this->shared_from_this(), tempIncomingMessage});
                else
                    incomingMessages.push_back({nullptr, tempIncomingMessage});
                ReadHeader();
            }

           protected:
            /*
                @brief Socket to the remote
            */
            asio::ip::tcp::socket _socket;

            /*
                @brief Reference to ASIO context object this connection is attached to
            */
            asio::io_context &asioContext;

            /*
                @brief Queue of outgoing messages
            */
            TsQueue<message<T>> outgoingMessages;

            /*
                @brief Reference to incoming message queue
            */
            TsQueue<owned_message<T>> &incomingMessages;

            /*
                @brief Temporary message
            */
            message<T> tempIncomingMessage;

            /*
                @brief Owner type
            */
            owner ownerType = owner::server;

            uint64_t handshakeOut = 0;
            uint64_t handshakeIn = 0;
            uint64_t handshakeCheck = 0;

            bool validHandshake = false;
            bool connectionEstablished = false;

            uint32_t id = 0;
        };
    }  // namespace net
}  // namespace RType
