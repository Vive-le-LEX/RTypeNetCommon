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
#include "NetConnection.hpp"
#include "NetMessage.hpp"
#include "NetTsqueue.hpp"

namespace RType {
    namespace net {
        template <typename T>
        class ClientInterface {
            ClientInterface() {}

            virtual ~ClientInterface() {
                Disconnect();
            }

            /*
                @brief Connect to a server
                @param host The hostname/ip-address of the server
                @param port The port to connect with
                @return True if the connection succeeded, false otherwise
            */
            bool Connect(const std::string& host, const uint16_t port) {
                try {
                    asio::ip::tcp::resolver resolver(context);
                    asio::ip::tcp::resolver::results_type endpoints = resolver.resolve(host, std::to_string(port));

                    currentConnection = std::make_unique<connection<T>>(connection<T>::owner::client, context, asio::ip::tcp::socket(context), incomingMessages);

                    currentConnection->ConnectToServer(endpoints);

                    contextThread = std::thread([this]() { context.run(); });
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
                    currentConnection->Disconnect();
                }

                context.stop();
                if (contextThread.joinable())
                    contextThread.join();

                currentConnection.release();
            }

            /*
                @brief Check if the client is connected to a server
                @return True if the client is connected to a server, false otherwise
            */
            bool IsConnected() {
                if (currentConnection)
                    return currentConnection->IsConnected();
                else
                    return false;
            }

            /*
                @brief Send a message to the server
                @param msg The message to send
            */
            void Send(const message<T>& msg) {
                if (IsConnected())
                    currentConnection->Send(msg);
            }

            /*
                @brief Retrieve the queue of messages from the server
                @return The queue of messages from the server
            */
            TsQueue<owned_message<T>>& Incoming() {
                return incomingMessages;
            }

           protected:
            asio::io_context context;
            std::thread contextThread;
            std::unique_ptr<connection<T>> currentConnection;

           private:
            // This is the thread safe queue of incoming messages from server
            TsQueue<owned_message<T>> incomingMessages;
        };
    }  // namespace net
}  // namespace olc
