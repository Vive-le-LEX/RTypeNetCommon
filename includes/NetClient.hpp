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
#include "NetTcpConnection.hpp"
#include "NetMessage.hpp"
#include "NetTsqueue.hpp"

namespace RType {
    namespace net {
        template <typename MessageType>
        class ClientInterface {
        public:
            ClientInterface() = default;

            virtual ~ClientInterface() {
                Disconnect();
            }

            /*
                @brief Connect to a server
                @param host The hostname/ip-address of the server
                @param port The port to connect with
                @return True if the connection succeeded, false otherwise
            */
            bool ConnectToServer(const std::string& host, const uint16_t port) {
                try {
                    _host = host;
                    _port = port;
                    asio::ip::tcp::resolver resolver(context);
                    asio::ip::tcp::resolver::results_type endpoints = resolver.resolve(host, std::to_string(port));

                    currentTcpConnection = std::make_unique<TcpConnection<MessageType>>(owner::client, context, asio::ip::tcp::socket(context), incomingTcpMessages);

                    currentTcpConnection->ConnectToServer(endpoints);

                    contextThread = std::thread([this]() { context.run(); });
                } catch (std::exception& e) {
                    std::cerr << "Client Exception: " << e.what() << "\n";
                    return false;
                }
                return true;
            }

            bool ConnectToRoom() {
                try {
                    currentUdpConnection = std::make_unique<UdpConnection<MessageType>>(owner::client, context, asio::ip::udp::endpoint(asio::ip::udp::v4(), _port), incomingUdpMessages);

                    std::cout << "Connecting to room" << std::endl;
                } catch (std::exception& e) {
                    std::cerr << "Client Exception: " << e.what() << "\n";
                    return false;
                }
                return true;
            }

            /*
                @brief Disconnect from the server
            */
            void Disconnect() {
                if (IsConnected()) {
                    currentTcpConnection->Disconnect();
                    currentUdpConnection->Disconnect();
                }

                context.stop();
                if (contextThread.joinable())
                    contextThread.join();

                currentTcpConnection.release();
                currentUdpConnection.release();
            }

            /*
                @brief Check if the client is connected to a server
                @return True if the client is connected to a server, false otherwise
            */
            bool IsConnected() {
                if (currentTcpConnection)
                    return currentTcpConnection->IsConnected();
                else
                    return false;
            }

            /*
                @brief Send a message to the server
                @param msg The message to send
            */
            void Send(const message<MessageType>& msg) {
                if (IsConnected())
                    currentTcpConnection->Send(msg);
            }

            /*
                @brief Retrieve the queue of messages from the server
                @return The queue of messages from the server
            */
            TsQueue<owned_message<MessageType, TcpConnection<MessageType>>>& IncomingTcpMessages() {
                return incomingTcpMessages;
            }

            TsQueue<owned_message<MessageType, UdpConnection<MessageType>>>& IncomingUdpMessages() {
                return incomingUdpMessages;
            }

        protected:
            std::string _host;
            uint16_t _port;

            asio::io_context context;
            std::thread contextThread;
            std::unique_ptr<TcpConnection<MessageType>> currentTcpConnection;
            std::unique_ptr<UdpConnection<MessageType>> currentUdpConnection;

        private:
            // This is the thread safe queue of incoming messages from server
            TsQueue<owned_message<MessageType, TcpConnection<MessageType>>> incomingTcpMessages;
            TsQueue<owned_message<MessageType, UdpConnection<MessageType>>> incomingUdpMessages;
        };
    }  // namespace net
}  // namespace RType
