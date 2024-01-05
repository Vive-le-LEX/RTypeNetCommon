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

#include <bitset>
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

    enum class ShipType : uint8_t {
        R9A = 0,
        R9E3 = 1,
        R90 = 2,
        R100 = 3,
        UFCS05 = 4,
        UFDD02 = 5,
        UFHC007 = 6,
        POW = 7,

        COUNT
    };

    enum class WeaponType : uint8_t {
        BLASTER = 4,
        LASER = 2,
        MISSILE = 8,
        BURST = 16,

        COUNT
    };

    enum class ShipColor : uint8_t {
        RED = 0,
        BLUE = 1,
        GREEN = 2,
        YELLOW = 3,

        COUNT
    };

    typedef struct {
        ShipType type;
        WeaponType weapon;
        ShipColor color;
        union {
            uint16_t ammo;
            uint16_t overheat;
            uint16_t charge;
        } weaponInfo;
    } ShipInfo_t;

    typedef struct Player_s {
        uuids::uuid uuid;
        Username_t username;

        uint8_t health = 100;
        uint8_t shield = 50;

        ShipInfo_t shipInfo;

        uint16_t kills = 0;
        uint8_t lives = 5;
        uint64_t score = 0;

        bool isAlive = true;

        glm::vec2 position = {0, 0};
        glm::vec2 velocity = {0, 0};

        bool operator==(const Player_s& rhs) const {
            return uuid == rhs.uuid;
        }

    } Player_t;

    inline std::ostream& operator<<(std::ostream& os, const Player_t& player) {
        return os << "Player {"
                  << "uuid: " << player.uuid << ", "
                  << "username: " << player.username << ", "
                  << "health: " << player.health << ", "
                  << "kills: " << player.kills << ", "
                  << "lives: " << player.lives << ", "
                  << "score: " << player.score << ", "
                  << "position: " << player.position.x << ", " << player.position.y << ", "
                  << "velocity: " << player.velocity.x << ", " << player.velocity.y << "}";
    }

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
