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

#include <sys/ioctl.h>
#include <unistd.h>

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <cstring>
#include <deque>
#include <functional>
#include <iostream>
#include <memory>
#include <mutex>
#include <optional>
#include <thread>
#include <unordered_map>
#include <vector>

#include "Singleton.hpp"

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

    return ip;
}

class AsyncTimer : public Singleton<AsyncTimer> {
   public:
    void addTimer(uint32_t id, uint32_t ms, std::function<void()> callback) {
        _callbacks[id] = std::move(callback);
        std::thread([this, id, ms]() {
            std::this_thread::sleep_for(std::chrono::milliseconds(ms));
            if (_callbacks.find(id) != _callbacks.end()) {
                _callbacks[id]();
                _callbacks.erase(id);
            }
        }).detach();
    }

    void removeTimer(uint32_t id) {
        if (_callbacks.find(id) != _callbacks.end())
            _callbacks.erase(id);
    }

   private:
    AsyncTimer() = default;
    friend class Singleton<AsyncTimer>;

    std::unordered_map<uint32_t, std::function<void()> > _callbacks;
};
