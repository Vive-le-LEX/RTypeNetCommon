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
#include "NetTcpConnection.hpp"
#include "NetTsqueue.hpp"
#include "NetUdpConnection.hpp"

namespace RType {
    namespace net {
        template <typename T>
        class TcpConnection;

        template <typename T>
        class UdpConnection;

        template <typename MessageType>
        class ServerInterface {
           public:
            /*
                @brief Construct the server interface
                @param port The port to listen on
            */
            explicit ServerInterface(uint16_t port) : asioAcceptor(asioContext, asio::ip::tcp::endpoint(asio::ip::tcp::v4(), port)), port(port) {
                std::cout << "[SERVER] Listening on: " << getIp() << ":" << port << std::endl;
            }

            virtual ~ServerInterface() {
                Stop();
            }

            /*
                @brief Start the server
                @return True if the server started successfully, false otherwise
             */
            bool Start() {
                try {
                    WaitForClientConnection();
                    udpConnection = std::make_shared<UdpConnection<MessageType>>(owner::server, asioContext, asio::ip::udp::endpoint(asio::ip::udp::v4(), port), incomingUdpMessages);
                    threadContext = std::thread([this]() { asioContext.run(); });
                } catch (std::exception& e) {
                    std::cerr << "[SERVER] Exception: " << e.what() << "\n";
                    return false;
                }

                std::cout << "[SERVER] Started!\n";
                return true;
            }

            /*
                @brief Stop the server
            */
            void Stop() {
                asioContext.stop();

                if (threadContext.joinable()) threadContext.join();

                std::cout << "[SERVER] Stopped!\n";
            }

            /*
                @brief Wait for a client to connect, this function is Asynchronous.
                Prime context with an instruction to wait until a socket connects. This
                is the purpose of an "acceptor" object. It will provide a unique socket
                for each incoming connection attempt
            */
            void WaitForClientConnection() {
                asioAcceptor.async_accept(
                    [this](std::error_code ec, asio::ip::tcp::socket _socket) {
                        if (!ec) {
                            std::cout << "[SERVER] New Connection: " << _socket.remote_endpoint() << "\n";

                            std::shared_ptr<TcpConnection<MessageType>> newConnection =
                                std::make_shared<TcpConnection<MessageType>>(owner::server,
                                                                             asioContext, std::move(_socket), incomingTcpMessages);

                            if (OnClientConnect(newConnection)) {
                                activeTcpConnections.push_back(std::move(newConnection));

                                activeTcpConnections.back()->ConnectToClient(this, nIDCounter++);

                                std::cout << "[" << activeTcpConnections.back()->GetID() << "] Connection Approved\n";
                            } else {
                                std::cout << "[-----] Connection Denied\n";
                            }
                        } else {
                            std::cout << "[SERVER] New Connection Error: " << ec.message() << "\n";
                        }

                        WaitForClientConnection();
                    });
            }

            /*
                @brief Send a message to a specific client
                @param client The client to send the message to
                @param msg The message to send
            */
            void MessageClient(std::shared_ptr<TcpConnection<MessageType>> client, const message<MessageType>& msg) {
                if (client && client->IsConnected()) {
                    client->Send(msg);
                } else {
                    OnClientDisconnect(client);

                    client.reset();

                    // Then physically remove it from the container
                    activeTcpConnections.erase(
                        std::remove(activeTcpConnections.begin(), activeTcpConnections.end(), client), activeTcpConnections.end());
                }
            }

            /*
                @brief Send a message to all clients
                @param msg The message to send
                @param ignoreClient A client to ignore
            */
            void MessageAllClients(const message<MessageType>& msg, std::shared_ptr<TcpConnection<MessageType>> ignoreClient = nullptr) {
                bool invalidClientExists = false;

                for (auto& client : activeTcpConnections) {
                    if (client && client->IsConnected()) {
                        if (client != ignoreClient)
                            client->Send(msg);
                    } else {
                        OnClientDisconnect(client);
                        client.reset();

                        invalidClientExists = true;
                    }
                }

                if (invalidClientExists)
                    activeTcpConnections.erase(
                        std::remove(activeTcpConnections.begin(), activeTcpConnections.end(), nullptr), activeTcpConnections.end());
            }

            void SendUdpMessage(const message<MessageType>& msg) {
                if (udpConnection && udpConnection->IsConnected()) {
                    udpConnection->Send(msg);
                } else {
                    std::cout << "[SERVER] UDP Connection is not connected" << std::endl;
                }
            }

            /*
                @brief Force update the server
                @param maxMessages The maximum number of messages to process
                @param wait Whether to wait for a message
            */
            void Update(size_t maxMessages = -1, bool wait = false) {
                if (wait) incomingTcpMessages.wait();

                size_t messageCount = 0;
                while (messageCount < maxMessages && !incomingTcpMessages.empty()) {
                    auto msg = incomingTcpMessages.pop_front();

                    OnMessage(msg.remote, msg.msg);

                    messageCount++;
                }
            }

           protected:
            /*
                @brief Called when a client connects
                @param client The client that connected
                @return True if the connection is accepted, false otherwise
            */
            virtual bool OnClientConnect(std::shared_ptr<TcpConnection<MessageType>> client) = 0;

            /*
                @brief Called when a client disconnects
                @param client The client that disconnected
            */
            virtual void OnClientDisconnect(std::shared_ptr<TcpConnection<MessageType>> client) = 0;

            /*
                @brief Called when a message arrives
                @param client The client that sent the message
                @param msg The message that was sent
            */
            virtual void OnMessage(std::shared_ptr<TcpConnection<MessageType>> client, message<MessageType>& msg) = 0;

           public:
            virtual void OnClientValidated(std::shared_ptr<TcpConnection<MessageType>> client) = 0;

           protected:
            uint16_t port;

            TsQueue<owned_message<MessageType, TcpConnection<MessageType>>> incomingTcpMessages;
            TsQueue<owned_message<MessageType, UdpConnection<MessageType>>> incomingUdpMessages;

            std::deque<std::shared_ptr<TcpConnection<MessageType>>> activeTcpConnections;
            std::shared_ptr<UdpConnection<MessageType>> udpConnection;

            asio::io_context asioContext;
            std::thread threadContext;

            asio::ip::tcp::acceptor asioAcceptor;

            uint32_t nIDCounter = 10000;
        };
    }  // namespace net
}  // namespace RType
