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

#include <iostream>

#include "RType.hpp"

namespace RType {

    enum class ServerMessages : uint32_t {
        ClientHandshake,

        ServerAccept,
        ServerDeny,

        ServerPing,

        ClientCreateLobby,
        ClientDeleteLobby,
        ClientJoinLobby,
        ClientLeaveLobby,
        ClientStartLobby,

        ServerLobbyCreated,
        ServerLobbyDeleted,
        ServerLobbyUpdated,
        ServerClientJoinedLobby,
        ServerClientLeftLobby,
        ServerLobbyStarted,

        ClientMove,
        ClientShoot,

        ClientDealDamage,
        ClientTakeDamage,
        ClientDie,

        ClientDisconnect,
    };

    inline std::ostream& operator<<(std::ostream& os, const ServerMessages& msg) {
        switch (msg) {
            case ServerMessages::ClientHandshake:
                os << "ClientHandshake";
                break;
            case ServerMessages::ServerAccept:
                os << "ServerAccept";
                break;
            case ServerMessages::ServerDeny:
                os << "ServerDeny";
                break;
            case ServerMessages::ServerPing:
                os << "ServerPing";
                break;
            case ServerMessages::ClientCreateLobby:
                os << "ClientCreateLobby";
                break;
            case ServerMessages::ClientDeleteLobby:
                os << "ClientDeleteLobby";
                break;
            case ServerMessages::ClientJoinLobby:
                os << "ClientJoinLobby";
                break;
            case ServerMessages::ClientLeaveLobby:
                os << "ClientLeaveLobby";
                break;
            case ServerMessages::ClientStartLobby:
                os << "ClientStartLobby";
                break;
            case ServerMessages::ServerLobbyCreated:
                os << "ServerLobbyCreated";
                break;
            case ServerMessages::ServerLobbyDeleted:
                os << "ServerLobbyDeleted";
                break;
            case ServerMessages::ServerLobbyUpdated:
                os << "ServerLobbyUpdated";
                break;
            case ServerMessages::ServerClientJoinedLobby:
                os << "ServerClientJoinedLobby";
                break;
            case ServerMessages::ServerClientLeftLobby:
                os << "ServerClientLeftLobby";
                break;
            case ServerMessages::ServerLobbyStarted:
                os << "ServerLobbyStarted";
                break;
            case ServerMessages::ClientMove:
                os << "ClientMove";
                break;
            case ServerMessages::ClientShoot:
                os << "ClientShoot";
                break;
            case ServerMessages::ClientDealDamage:
                os << "ClientDealDamage";
                break;
            case ServerMessages::ClientTakeDamage:
                os << "ClientTakeDamage";
                break;
            case ServerMessages::ClientDie:
                os << "ClientDie";
                break;
            case ServerMessages::ClientDisconnect:
                os << "ClientDisconnect";
                break;
            default:
                os << "Unknown";
                break;
        }
        return os;
    }

}  // namespace RType
