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

    /**
     * @brief Generate a uuid
     *
     * @return uuids::uuid
     */
    static uuids::uuid GenerateUuid() {
        std::random_device rd;
        auto seed_data = std::array<int, std::mt19937::state_size>{};
        std::generate(std::begin(seed_data), std::end(seed_data), std::ref(rd));
        std::seed_seq seq(std::begin(seed_data), std::end(seed_data));
        std::mt19937 generator(seq);
        uuids::uuid_random_generator gen{generator};
        return gen();
    }

    typedef char Username_t[32];       ///< The username type
    typedef char ErrorMessage_t[128];  ///< The error message type

#define MAX_PLAYERS 16
#define MAX_LOBBIES 16

    /**
     * @brief Struct that contains all the information about a lobby
     *
     */
    typedef struct Lobby_s {
        uuids::uuid uuid;          ///< The uuid of the lobby
        uint16_t port;             ///< The port of the lobby
        uint16_t maxPlayers;       ///< The max players of the lobby
        uint16_t playerCount = 0;  ///< The player count of the lobby

        /**
         * @brief Struct that contains all the information about a player
         *
         */
        struct connectedPlayers_s {
            uuids::uuid uuid;                  ///< The uuid of the player
            Username_t username;               ///< The username of the player
        } connectedPlayers[MAX_PLAYERS] = {};  ///< The connected players of the lobby
        bool started = false;                  ///< Is the lobby started
    } Lobby_t;

    /// @private
    inline std::ostream& operator<<(std::ostream& os, const Lobby_t& lobby) {
        os << "Lobby {\n"
           << "\tport: " << lobby.port << ",\n"
           << "\tuuid: " << lobby.uuid << ",\n"
           << "\tmaxPlayers: " << lobby.maxPlayers << ",\n"
           << "\tplayers (" << lobby.playerCount << "): \n";

        for (int i = 0; i < lobby.playerCount; i++) {
            os << "\t  - " << lobby.connectedPlayers[i].username << "&\n";
        }
        return os << "}";
    }

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
        BLUE = 0,
        MAGENTA = 1,
        GREEN = 2,
        RED = 3,

        COUNT
    };

    /**
     * @brief Struct that contains all the information about a ship
     *
     */
    typedef struct {
        ShipType type;      ///< The type of the ship
        WeaponType weapon;  ///< The weapon of the ship
        ShipColor color;    ///< The color of the ship
        union {
            uint16_t ammo;
            uint16_t overheat;
            uint16_t charge;
        } weaponInfo;  ///< The ammo, overheat or charge of the weapon
    } ShipInfo_t;

    /**
     * @brief Struct that contains all the information about a player
     *
     */
    typedef struct Player_s {
        uuids::uuid uuid;     ///< The uuid of the player
        Username_t username;  ///< The username of the player

        uint8_t health = 100;  ///< The health of the player
        uint8_t shield = 50;   ///< The shield of the player

        ShipInfo_t shipInfo;  ///< The ship info of the player

        uint16_t kills = 0;  ///< The kills of the player
        uint8_t lives = 5;   ///< The lives of the player
        uint64_t score = 0;  ///< The score of the player

        bool isAlive = true;  ///< Is the player alive

        glm::vec2 position = {0, 0};  ///< The position of the player
        glm::vec2 velocity = {0, 0};  ///< The velocity of the player

        /// @private
        bool operator==(const Player_s& rhs) const {
            return uuid == rhs.uuid;
        }

    } Player_t;

    /// @private
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

    /// @private
    namespace tcp {

        /**
         * @brief Payload for the CreateLobby event (client -> server)
         */
        typedef struct CreateLobby_s {
            uint16_t maxPlayers;  ///< The max players of the lobby
        } CreateLobby_t;

        /**
         * @brief Payload for the LobbyCreated event (server -> client)
         */
        typedef struct LobbyCreated_s {
            uuids::uuid uuid;     ///< The uuid of the lobby
            uint16_t port;        ///< The port of the lobby
            uint16_t maxPlayers;  ///< The max players of the lobby
        } LobbyCreated_t;

        /**
         * @brief Payload for the DeleteLobby event (client -> server)
         */
        typedef struct DeleteLobby_s {
            uuids::uuid uuid;  ///< The uuid of the lobby
        } DeleteLobby_t;

        /// @private
        inline std::ostream& operator<<(std::ostream& os, const DeleteLobby_t& deleteLobby) {
            return os << "DeleteLobby {\n"
                      << "\tlobbyUuid:  " << deleteLobby.uuid << "\n}\n";
        }

        /**
         * @brief Payload for the LobbyDeleted event (server -> client)
         */
        typedef struct LobbyDeleted_s {
            uuids::uuid uuid;  ///< The uuid of the lobby
        } LobbyDeleted_t;

        /**
         * @brief Payload for the JoinLobby event (client -> server)
         */
        typedef struct JoinLobby_s {
            uuids::uuid lobbyUuid;   ///< The uuid of the lobby
            uuids::uuid clientUuid;  ///< The uuid of the client
            Username_t username;     ///< The username of the client
            ShipColor color;         ///< The color of the ship
        } JoinLobby_t;

        /// @private
        inline std::ostream& operator<<(std::ostream& os, const JoinLobby_t& joinLobby) {
            return os << "JoinLobby {\n"
                      << "\tclientUuid: " << joinLobby.clientUuid << ",\n"
                      << "\tusername:   " << joinLobby.username << ",\n"
                      << "\tlobbyUuid:  " << joinLobby.lobbyUuid << "\n}\n";
        }

        /**
         * @brief Payload for the LobbyJoined event (server -> client)
         */
        typedef struct LobbyJoined_s {
            uuids::uuid lobbyUuid;   ///< The uuid of the lobby
            uuids::uuid clientUuid;  ///< The uuid of the client
            Username_t username;     ///< The username of the client
        } LobbyJoined_t;

        /**
         * @brief Payload for the LeaveLobby event (client -> server)
         */
        typedef struct LeaveLobby_s {
            uuids::uuid clientUuid;  ///< The uuid of the client
            uuids::uuid lobbyUuid;   ///< The uuid of the lobby
        } LeaveLobby_t;

        /// @private
        inline std::ostream& operator<<(std::ostream& os, const LeaveLobby_t& leaveLobby) {
            return os << "LeaveLobby {\n"
                      << "\tclientUuid: " << leaveLobby.clientUuid << ",\n"
                      << "\tlobbyUuid:  " << leaveLobby.lobbyUuid << "\n}\n";
        }

        /**
         * @brief Payload for the LobbyLeft event (server -> client)
         */
        typedef struct LobbyLeft_s {
            uuids::uuid clientUuid;  ///< The uuid of the client
            uuids::uuid lobbyUuid;   ///< The uuid of the lobby
        } LobbyLeft_t;

        /**
         * @brief Payload for the StartLobby event (client -> server)
         */
        typedef struct StartLobby_s {
            uuids::uuid uuid;  ///< The uuid of the lobby
        } StartLobby_t;

        /// @private
        inline std::ostream& operator<<(std::ostream& os, const StartLobby_t& startLobby) {
            return os << "StartLobby {\n"
                      << "\tlobbyUuid:  " << startLobby.uuid << "\n}\n";
        }

        /**
         * @brief Payload for the LobbyStarted event (server -> client)
         */
        typedef struct LobbyStarted_s {
            uuids::uuid uuid;  ///< The uuid of the lobby
        } LobbyStarted_t;

    }  // namespace tcp

    namespace udp {

        /**
         * @brief Payload for the ClientMove event
         */
        typedef struct Move_s {
            const ServerMessages messayeType = ServerMessages::ClientMove;  ///< The type of the message
            uuids::uuid clientUuid;                                         ///< The uuid of the client
            glm::vec2 position;                                             ///< The position of the client
            glm::vec2 velocity;                                             ///< The velocity of the client
        } Move_t;

        /// @private
        inline std::ostream& operator<<(std::ostream& os, const Move_t& move) {
            return os << "Move {\n"
                      << "\tclientUuid: " << move.clientUuid << ",\n"
                      << "\tposition:   " << move.position.x << ", " << move.position.y << ",\n"
                      << "\tvelocity:   " << move.velocity.x << ", " << move.velocity.y << "\n}\n";
        }
        
        typedef struct Shoot_s {
            const ServerMessages messayeType = ServerMessages::ClientShoot;  ///< The type of the message
            uuids::uuid clientUuid;                                         ///< The uuid of the client
            glm::vec2 position;                                             ///< The position of the shoot
            glm::vec2 target_velocity;                                      ///< The velocity of the shoot
            u_int8_t id;                                                    ///< The ID of the bullet
        } Shoot_t;
        
        typedef struct AddEnemy_s {
            const ServerMessages messayeType = ServerMessages::ClientShoot; ///< The type of the message
            uint8_t  id;                                                    ///< The ID of the enemy
            glm::vec2 position;                                             ///< The position of the enemy
            glm::vec2 velocity;                                             ///< The velocity of the enemy
            double time;                                                    ///< Time of the enemy slide animation
        } AddEnemy;

    }  // namespace udp
}  // namespace RType
