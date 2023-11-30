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

#include "net_common.hpp"
#include "net_connection.hpp"
#include "net_message.hpp"
#include "net_tsqueue.hpp"

namespace RType {
    namespace net {
        template <typename T>
        class server_interface {
           protected:
            /*
                @brief Construct the server interface
                @param port The port to listen on
            */
            server_interface(uint16_t port) : asioAcceptor(asioContext, asio::ip::tcp::endpoint(asio::ip::tcp::v4(), port)) {

                std::cout << "[SERVER] Listening on: " << getIp() << ":" << port << std::endl;

            }

            virtual ~server_interface() {
                Stop();
            }

            /*
                @brief Start the server
                @return True if the server started successfully, false otherwise
             */
            bool Start() {
                try {
                    WaitForClientConnection();

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
                    [this](std::error_code ec, asio::ip::tcp::socket socket) {
                        if (!ec) {
                            std::cout << "[SERVER] New Connection: " << socket.remote_endpoint() << "\n";

                            std::shared_ptr<connection<T>> newconn =
                                std::make_shared<connection<T>>(connection<T>::owner::server,
                                                                asioContext, std::move(socket), incomingMessages);

                            if (OnClientConnect(newconn)) {
                                activeConnections.push_back(std::move(newconn));

                                activeConnections.back()->ConnectToClient(nIDCounter++);

                                std::cout << "[" << activeConnections.back()->GetID() << "] Connection Approved\n";
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
            void MessageClient(std::shared_ptr<connection<T>> client, const message<T>& msg) {
                if (client && client->IsConnected()) {
                    client->Send(msg);
                } else {
                    OnClientDisconnect(client);

                    client.reset();

                    // Then physically remove it from the container
                    activeConnections.erase(
                        std::remove(activeConnections.begin(), activeConnections.end(), client), activeConnections.end());
                }
            }

            /*
                @brief Send a message to all clients
                @param msg The message to send
                @param ignoreClient A client to ignore
            */
            void MessageAllClients(const message<T>& msg, std::shared_ptr<connection<T>> ignoreClient = nullptr) {
                bool invalidClientExists = false;

                for (auto& client : activeConnections) {
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
                    activeConnections.erase(
                        std::remove(activeConnections.begin(), activeConnections.end(), nullptr), activeConnections.end());
            }

            /*
                @brief Force update the server
                @param maxMessages The maximum number of messages to process
                @param wait Whether or not to wait for a message
            */
            void Update(size_t maxMessages = -1, bool wait = false) {
                if (wait) incomingMessages.wait();

                size_t messageCount = 0;
                while (messageCount < maxMessages && !incomingMessages.empty()) {
                    auto msg = incomingMessages.pop_front();

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
            virtual bool OnClientConnect(std::shared_ptr<connection<T>> client) = 0;

            /*
                @brief Called when a client disconnects
                @param client The client that disconnected
            */
            virtual void OnClientDisconnect(std::shared_ptr<connection<T>> client) = 0;

            /*
                @brief Called when a message arrives
                @param client The client that sent the message
                @param msg The message that was sent
            */
            virtual void OnMessage(std::shared_ptr<connection<T>> client, message<T>& msg) = 0;

            tsqueue<owned_message<T>> incomingMessages;

            std::deque<std::shared_ptr<connection<T>>> activeConnections;

            asio::io_context asioContext;
            std::thread threadContext;

            asio::ip::tcp::acceptor asioAcceptor;

            uint32_t nIDCounter = 10000;
        };
    }  // namespace net
}  // namespace RType
