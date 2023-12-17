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
        class UdpConnection : public AConnection<MessageType>, public std::enable_shared_from_this<UdpConnection<MessageType>> {
           public:
            UdpConnection(owner parent,
                          asio::io_context& context,
                          asio::ip::udp::socket socket,
                          TsQueue<owned_message<MessageType, UdpConnection<MessageType>>>& incomingMessages) : AConnection<MessageType>(parent, context, std::move(socket), incomingMessages) {
                try {
                    std::cout <"[UDP] New connection from " << this->udpSocket.remote_endpoint() << std::endl;
                } catch (std::exception& e) {
                    std::cerr << "[Error] Failed to get remote endpoint: " << e.what() << std::endl;
                }
            }

            virtual ~UdpConnection() = default;

            void StartReceiving() {
                this->udpSocket.async_receive_from(asio::buffer(this->receiveBuffer), this->remoteEndpoint,
                                                   [this](std::error_code ec, std::size_t bytes_received) {
                                                       if (!ec) {
                                                           // Process the received message
                                                           std::cout << "Received message from " << this->remoteEndpoint << ": " << std::string(this->receiveBuffer.begin(), this->receiveBuffer.begin() + bytes_received) << std::endl;

                                                           // Continue listening for incoming messages
                                                           this->StartReceiving();
                                                       } else {
                                                           std::cerr << "[Error] Failed to receive message: " << ec.message() << std::endl;
                                                       }
                                                   });
            }
        };
    }  // namespace net
}  // namespace RType
