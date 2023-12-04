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

namespace RType {

    namespace net {

        /*
            @brief Message Header is sent at start of all messages. The template allows us
            to use "enum class" to ensure that the messages are valid at compile time

            @tparam T Message type
        */
        template <typename T>
        struct message_header {
            T id{};
            uint32_t size = 0;
        };

        /*
            @struct message
            @brief Message Body contains a header and a std::vector, containing raw bytes
            of infomation. This way the message can be variable length, but the size
            in the header must be updated.

            @tparam T Message type
        */
        template <typename T>
        struct message {
            message_header<T> header{};
            std::vector<uint8_t> body;

            /*
                @brief Returns size of entire message packet in bytes
            */
            size_t size() const {
                return body.size();
            }

            /*
                @brief Returns a friendly description of the message
            */
            friend std::ostream& operator<<(std::ostream& os, const message<T>& msg) {
                os << "ID:" << int(msg.header.id) << " Size:" << msg.header.size;
                return os;
            }

            /*
                @brief Pushes any POD-like data into the message buffer

                @tparam DataType POD-like data type

                @param data POD-like data to push into buffer

                @return message<T>&
            */
            template <typename DataType>
            friend message<T>& operator<<(message<T>& msg, const DataType& data) {
                static_assert(std::is_standard_layout<DataType>::value, "Data is too complex to be pushed into vector");

                size_t i = msg.body.size();

                msg.body.resize(msg.body.size() + sizeof(DataType));
                std::memcpy(msg.body.data() + i, &data, sizeof(DataType));
                msg.header.size = msg.size();

                return msg;
            }

            /*
                @brief Pulls any POD-like data form the message buffer

                @tparam DataType POD-like data type

                @param data POD-like data to pull from buffer

                @return message<T>&
            */
            template <typename DataType>
            friend message<T>& operator>>(message<T>& msg, DataType& data) {
                static_assert(std::is_standard_layout<DataType>::value, "Data is too complex to be pulled from vector");

                size_t i = msg.body.size() - sizeof(DataType);

                std::memcpy(&data, msg.body.data() + i, sizeof(DataType));
                msg.body.resize(i);
                msg.header.size = msg.size();

                return msg;
            }
        };

        template <typename T>
        class connection;

        /*
            @struct owned_message
            @brief An "owned" message is identical to a regular message, but it is associated with
            a connection. On a server, the owner would be the client that sent the message,
            on a client the owner would be the server.

            @tparam T Message type
        */
        template <typename T>
        struct owned_message {
            std::shared_ptr<connection<T>> remote = nullptr;
            message<T> msg;

            friend std::ostream& operator<<(std::ostream& os, const owned_message<T>& msg) {
                os << msg.msg;
                return os;
            }
        };
    }  // namespace net
}  // namespace RType
