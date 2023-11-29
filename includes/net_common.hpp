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
    strcpy(ip, inet_ntoa(((sockaddr_in *)&ifr.ifr_addr)->sin_addr));

    return std::string(ip);
}
