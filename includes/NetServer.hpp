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

namespace RType {
    namespace net {
        template <typename T>
        class TcpConnection;

        template <typename MessageType>
        class ServerInterface {
           public:
            /*
                @brief Construct the server interface
                @param port The port to listen on
            */
            explicit ServerInterface(uint16_t port) : asioAcceptor_(asioContext_, asio::ip::tcp::endpoint(asio::ip::tcp::v4(), port_)), port_(port) {
                std::cout << "[SERVER] Listening on: " << getIp() << ":" << port_ << std::endl;
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
                    threadContext_ = std::thread([this]() { asioContext_.run(); });
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
                asioContext_.stop();

                if (threadContext_.joinable()) threadContext_.join();

                std::cout << "[SERVER] Stopped!\n";
            }

            /*
                @brief Wait for a client to connect, this function is Asynchronous.
                Prime context with an instruction to wait until a socket connects. This
                is the purpose of an "acceptor" object. It will provide a unique socket
                for each incoming connection attempt
            */
            void WaitForClientConnection() {
                asioAcceptor_.async_accept(
                    [this](std::error_code ec, asio::ip::tcp::socket _socket) {
                        if (!ec) {
                            std::cout << "[SERVER] New Connection: " << _socket.remote_endpoint() << "\n";

                            std::shared_ptr<TcpConnection<MessageType>> newConnection =
                                std::make_shared<TcpConnection<MessageType>>(owner::server,
                                                                             asioContext_, std::move(_socket), incomingTcpMessages_);

                            if (OnClientConnect(newConnection)) {
                                activeTcpConnections_.push_back(std::move(newConnection));

                                activeTcpConnections_.back()->ConnectToClient(this, IDCounter_++);

                                std::cout << "[" << activeTcpConnections_.back()->GetID() << "] Connection Approved\n";
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
                    activeTcpConnections_.erase(
                        std::remove(activeTcpConnections_.begin(), activeTcpConnections_.end(), client), activeTcpConnections_.end());
                }
            }

            void MessageClient(uint32_t id, const message<MessageType>& msg) {
                auto client = GetClientById(id);
                if (client != nullptr) {
                    MessageClient(client, msg);
                }
            }

            /*
                @brief Send a message to all clients
                @param msg The message to send
                @param ignoreClient A client to ignore
            */
            void MessageAllClients(const message<MessageType>& msg, std::shared_ptr<TcpConnection<MessageType>> ignoreClient = nullptr) {
                bool invalidClientExists = false;

                for (auto& client : activeTcpConnections_) {
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
                    activeTcpConnections_.erase(
                        std::remove(activeTcpConnections_.begin(), activeTcpConnections_.end(), nullptr), activeTcpConnections_.end());
            }

            /*
                @brief Force update the server
                @param maxMessages The maximum number of messages to process
                @param wait Whether to wait for a message
            */
            void Update(size_t maxMessages = -1, bool wait = false) {
                if (wait) incomingTcpMessages_.wait();

                size_t messageCount = 0;
                while (messageCount < maxMessages && !incomingTcpMessages_.empty()) {
                    auto msg = incomingTcpMessages_.pop_front();

                    OnMessage(msg.remote, msg.msg);

                    messageCount++;
                }
            }

            std::shared_ptr<TcpConnection<MessageType>> GetClientById(uint32_t id) {
                for (auto& client : activeTcpConnections_) {
                    if (client->GetID() == id)
                        return client;
                }
                return nullptr;
            }

            std::deque<std::shared_ptr<TcpConnection<MessageType>>> GetClients() {
                return activeTcpConnections_;
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
            uint16_t port_;

            TsQueue<owned_message<MessageType, TcpConnection<MessageType>>> incomingTcpMessages_;

            std::deque<std::shared_ptr<TcpConnection<MessageType>>> activeTcpConnections_;

            asio::io_context asioContext_;
            std::thread threadContext_;

            asio::ip::tcp::acceptor asioAcceptor_;

            uint32_t IDCounter_ = 10000;
        };
    }  // namespace net
}  // namespace RType
