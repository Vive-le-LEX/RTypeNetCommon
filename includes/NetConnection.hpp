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
            connection(owner parent, asio::io_context& asioContext, asio::ip::tcp::socket socket, TsQueue<owned_message<T>>& qIn)
                : asioContext(asioContext), socket(std::move(socket)), incomingMessages(qIn) {
                ownerType = parent;
            }

            virtual ~connection() {}

            /*
                @brief Get the ID of this connection
                @return uint32_t ID of this connection
            */
            uint32_t GetID() const {
                return id;
            }

            /*
                @brief Connect to a client
                @param uid ID of the client
            */
            void ConnectToClient(uint32_t uid = 0) {
                if (ownerType == owner::server) {
                    if (socket.is_open()) {
                        id = uid;
                        ReadHeader();
                    }
                }
            }

            /*
                @brief Connect to a server
                @param endpoints Endpoints of the server
            */
            void ConnectToServer(const asio::ip::tcp::resolver::results_type& endpoints) {
                if (ownerType == owner::client) {
                    asio::async_connect(socket, endpoints, [this](std::error_code ec, asio::ip::tcp::endpoint endpoint) {
                        if (!ec) {
                            ReadHeader();
                        }
                    });
                }
            }

            /*
                @brief Disconnect from server
            */
            void Disconnect() {
                if (IsConnected())
                    asio::post(asioContext, [this]() { socket.close(); });
            }

            /*
                @brief Check if this connection is still active
                @return true if this connection is still active, false otherwise
            */
            bool IsConnected() const {
                return socket.is_open();
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
            void Send(const message<T>& msg) {
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
                asio::async_write(socket, asio::buffer(&outgoingMessages.front().header, sizeof(message_header<T>)),
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
                                          socket.close();
                                      }
                                  });
            }

            /*
                @brief Write a message body, this function is Asynchronous
            */
            void WriteBody() {
                asio::async_write(socket, asio::buffer(outgoingMessages.front().body.data(), outgoingMessages.front().body.size()),
                                  [this](std::error_code ec, std::size_t length) {
                                      if (!ec) {
                                          outgoingMessages.pop_front();

                                          if (!outgoingMessages.empty()) {
                                              WriteHeader();
                                          }
                                      } else {
                                          std::cout << "[" << id << "] Write Body Fail.\n";
                                          socket.close();
                                      }
                                  });
            }

            /*
                @brief Read a message header, this function is Asynchronous.
                If this function is called, we are expecting asio to wait until it receives
                enough bytes to form a header of a message.
            */
            void ReadHeader() {
                asio::async_read(socket, asio::buffer(&tempIncomingMessage.header, sizeof(message_header<T>)),
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
                                         socket.close();
                                     }
                                 });
            }

            /*
                @brief Read a message body, this function is Asynchronous.
                If this function is called, a header has already been read, and in that header
                request we read a body, of the specified size, into the message buffer.
            */
            void ReadBody() {
                asio::async_read(socket, asio::buffer(tempIncomingMessage.body.data(), tempIncomingMessage.body.size()),
                                 [this](std::error_code ec, std::size_t length) {
                                     if (!ec) {
                                         AddToIncomingMessageQueue();
                                     } else {
                                         std::cout << "[" << id << "] Read Body Fail.\n";
                                         socket.close();
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
            asio::ip::tcp::socket socket;

            /*
                @brief Reference to ASIO context object this connection is attached to
            */
            asio::io_context& asioContext;

            /*
                @brief Queue of outgoing messages
            */
            TsQueue<message<T>> outgoingMessages;

            /*
                @brief Reference to incoming message queue
            */
            TsQueue<owned_message<T>>& incomingMessages;

            /*
                @brief Temporary message
            */
            message<T> tempIncomingMessage;

            /*
                @brief Owner type
            */
            owner ownerType = owner::server;

            uint32_t id = 0;
        };
    }  // namespace net
}  // namespace RType