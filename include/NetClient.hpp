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
                    host_ = host;
                    port_ = port;
                    asio::ip::tcp::resolver resolver(context_);
                    asio::ip::tcp::resolver::results_type endpoints = resolver.resolve(host, std::to_string(port));

                    currentTcpConnection_ = std::make_unique<TcpConnection<MessageType>>(owner::client, context_, asio::ip::tcp::socket(context_), incomingTcpMessages_);

                    currentTcpConnection_->ConnectToServer(endpoints);

                    contextThread_ = std::thread([this]() { context_.run(); });
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
                if (this->IsConnected()) {
                    currentTcpConnection_->Disconnect();
                }

                context_.stop();
                if (contextThread_.joinable()) {
                    contextThread_.join();
                }

                currentTcpConnection_.release();
            }

            /*
                @brief Check if the client is connected to a server
                @return True if the client is connected to a server, false otherwise
            */
            bool IsConnected() {
                if (currentTcpConnection_)
                    return currentTcpConnection_->IsConnected();
                else
                    return false;
            }

            /*
                @brief Send a message to the server
                @param msg The message to send
            */
            void Send(const message<MessageType>& msg) {
                if (this->IsConnected())
                    currentTcpConnection_->Send(msg);
            }

            /*
                @brief Retrieve the queue of messages from the server
                @return The queue of messages from the server
            */
            TsQueue<owned_message<MessageType, TcpConnection<MessageType>>>& IncomingTcpMessages() {
                return incomingTcpMessages_;
            }

        protected:
            std::string host_;
            uint16_t port_;

            asio::io_context context_;
            std::thread contextThread_;
            std::unique_ptr<TcpConnection<MessageType>> currentTcpConnection_;

        private:
            // This is the thread safe queue of incoming messages from server
            TsQueue<owned_message<MessageType, TcpConnection<MessageType>>> incomingTcpMessages_;
        };
    }  // namespace net
}  // namespace RType
