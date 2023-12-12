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

#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <deque>
#include <iostream>
#include <memory>
#include <mutex>
#include <optional>
#include <thread>
#include <vector>

#ifdef _WIN32
#define _WIN32_WINNT 0x0A00
#elif __linux__
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#endif

#define ASIO_STANDALONE
#include <asio.hpp>
#include <asio/ts/buffer.hpp>
#include <asio/ts/internet.hpp>

//----------------------------------------------------------------
// Utilities
//----------------------------------------------------------------

inline std::string getIp(void) {
    int fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_IP);

    struct ifreq ifr {};
    strcpy(ifr.ifr_name, "wlo1");
    ioctl(fd, SIOCGIFADDR, &ifr);
    close(fd);

    char ip[INET_ADDRSTRLEN];
    strcpy(ip, inet_ntoa(((sockaddr_in*)&ifr.ifr_addr)->sin_addr));

    return std::string(ip);
}

template <typename SyncReadStream, typename MutableBufferSequence>
inline void readWithTimeout(SyncReadStream& s, const MutableBufferSequence& buffers, std::chrono::seconds& timeoutDuration = 1) {
    asio::steady_timer timer(s.get_io_service());

    timer.expires_from_now(timeoutDuration);
    timer.async_wait([&](const std::error_code& ec) {
        if (!ec) {
            // Handle timeout here
            std::cout << "Timeout occurred!" << std::endl;
            // You may want to close the socket or perform other actions
        }
    });

    asio::async_read(s, buffers, asio::transfer_at_least(1),
                            [&](const std::error_code& ec, std::size_t bytes_transferred) {
                                // Cancel the timer as the read operation completed before timeout
                                timer.cancel();

                                if (!ec) {
                                    // Handle read completion here
                                    std::cout << "Read completed, received " << bytes_transferred << " bytes." << std::endl;
                                } else {
                                    // Handle read error here
                                    std::cout << "Read error: " << ec.message() << std::endl;
                                }
                            });
}
