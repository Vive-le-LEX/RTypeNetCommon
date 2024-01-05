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

#include "NetClient.hpp"
#include "NetCommon.hpp"
#include "NetMessage.hpp"
#include "NetServer.hpp"
#include "NetTcpConnection.hpp"
#include "NetTsqueue.hpp"
#include "NetUdpServer.hpp"
#include "RTypeServerMessages.hpp"

#define UUID_SYSTEM_GENERATOR
#include <uuid.h>

#include <glm/glm.hpp>

namespace RType {

    static uuids::uuid GenerateUuid() {
        std::random_device rd;
        auto seed_data = std::array<int, std::mt19937::state_size>{};
        std::generate(std::begin(seed_data), std::end(seed_data), std::ref(rd));
        std::seed_seq seq(std::begin(seed_data), std::end(seed_data));
        std::mt19937 generator(seq);
        uuids::uuid_random_generator gen{generator};
        return gen();
    }

    typedef char Username_t[32];
    typedef char ErrorMessage_t[128];

    ////////////////////////////////////////////////////////////////
    //                          Payloads                          //
    // Struct that are named InfinitiveVerbEvent_t are sent by    //
    // the client to the server.(CreateLobby_t)                   //
    // Struct that are named EventPastVerb_t are sent by the      //
    // server to the client.(LobbyCreated_t)                      //
    ////////////////////////////////////////////////////////////////

    namespace tcp {
        typedef struct CreateLobby_s {
            uint16_t maxPlayers;
        } CreateLobby_t;

        typedef struct LobbyCreated_s {
            uuids::uuid uuid;
            uint16_t port;
            uint16_t maxPlayers;
        } LobbyCreated_t;

        typedef struct DeleteLobby_s {
            uuids::uuid uuid;
        } DeleteLobby_t;

        inline std::ostream& operator<<(std::ostream& os, const DeleteLobby_t& deleteLobby) {
            return os << "DeleteLobby {\n"
                      << "\tlobbyUuid:  " << deleteLobby.uuid << "\n}\n";
        }

        typedef struct LobbyDeleted_s {
            uuids::uuid uuid;
        } LobbyDeleted_t;

        typedef struct JoinLobby_s {
            uuids::uuid lobbyUuid;
            uuids::uuid clientUuid;
            Username_t username;
        } JoinLobby_t;

        inline std::ostream& operator<<(std::ostream& os, const JoinLobby_t& joinLobby) {
            return os << "JoinLobby {\n"
                      << "\tclientUuid: " << joinLobby.clientUuid << ",\n"
                      << "\tusername:   " << joinLobby.username << ",\n"
                      << "\tlobbyUuid:  " << joinLobby.lobbyUuid << "\n}\n";
        }

        typedef struct LobbyJoined_s {
            uuids::uuid lobbyUuid;
            uuids::uuid clientUuid;
            Username_t username;
        } LobbyJoined_t;

        typedef struct LeaveLobby_s {
            uuids::uuid clientUuid;
            uuids::uuid lobbyUuid;
        } LeaveLobby_t;

        inline std::ostream& operator<<(std::ostream& os, const LeaveLobby_t& leaveLobby) {
            return os << "LeaveLobby {\n"
                      << "\tclientUuid: " << leaveLobby.clientUuid << ",\n"
                      << "\tlobbyUuid:  " << leaveLobby.lobbyUuid << "\n}\n";
        }

        typedef struct LobbyLeft_s {
            uuids::uuid clientUuid;
            uuids::uuid lobbyUuid;
        } LobbyLeft_t;

        typedef struct StartLobby_s {
            uuids::uuid uuid;
        } StartLobby_t;

        inline std::ostream& operator<<(std::ostream& os, const StartLobby_t& startLobby) {
            return os << "StartLobby {\n"
                      << "\tlobbyUuid:  " << startLobby.uuid << "\n}\n";
        }

        typedef struct LobbyStarted_s {
            uuids::uuid uuid;
        } LobbyStarted_t;

    }  // namespace tcp

    namespace udp {
        typedef struct {
            const ServerMessages messayeType = ServerMessages::ClientMove;
            uuids::uuid clientUuid;
            glm::vec2 position;
            glm::vec2 velocity;
        } Move_t;

        inline std::ostream& operator<<(std::ostream& os, const Move_t& move) {
            return os << "Move {\n"
                      << "\tclientUuid: " << move.clientUuid << ",\n"
                      << "\tposition:   " << move.position.x << ", " << move.position.y << ",\n"
                      << "\tvelocity:   " << move.velocity.x << ", " << move.velocity.y << "\n}\n";
        }
    }  // namespace udp
}  // namespace RType
