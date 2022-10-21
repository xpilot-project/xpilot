#ifndef DTO_H
#define DTO_H

#include <msgpack.hpp>
#include <string>
#include <optional>

namespace xpilot {

    namespace dto {
        const std::string ADD_AIRCRAFT = "ADD";
        const std::string AIRCRAFT_ADDED = "ADDED";
        const std::string AIRCRAFT_DELETED = "DELETED";
        const std::string DELETE_AIRCRAFT = "DEL";
        const std::string DELETE_ALL_AIRCRAFT = "DELALL";
        const std::string AIRCRAFT_CONFIG = "ACCONF";
        const std::string FAST_POSITION_UPDATE = "FSTPOS";
        const std::string HEARTBEAT = "HB";
        const std::string PLUGIN_VER = "VER";
        const std::string VALIDATE_CSL = "CSL";
        const std::string RADIO_MESSAGE_SENT = "RDIOSENT";
        const std::string RADIO_MESSAGE_RECEIVED = "RDIORCVD";
        const std::string NOTIFICATION_POSTED = "NOTIF";
        const std::string PRIVATE_MESSAGE_SENT = "PRIVSENT";
        const std::string PRIVATE_MESSAGE_RECEIVED = "PRIVRCVD";
        const std::string NEARBY_ATC = "ATC";
        const std::string REQUEST_METAR = "REQMETAR";
        const std::string REQUEST_STATION_INFO = "REQSTATION";
        const std::string WALLOP_SENT = "WALLOP";
        const std::string FORCE_DISCONNECT = "FORCEDISC";
        const std::string CONNECTED = "CONN";
        const std::string DISCONNECTED = "DISCON";
        const std::string SHUTDOWN = "SHUTDOWN";
        const std::string STATION_CALLSIGN = "STATION_CALLSIGN";
    }

    using namespace dto;

    struct BaseDto {
        std::string type;
        msgpack::object dto;
        MSGPACK_DEFINE(type, dto)
    };

    struct AddAircraftDto {
        std::string callsign;
        std::string airline;
        std::string typeCode;
        double latitude;
        double longitude;
        double altitudeTrue;
        double heading;
        double bank;
        double pitch;
        MSGPACK_DEFINE(callsign, airline, typeCode, latitude, longitude, altitudeTrue, heading, bank, pitch);

        static std::string getName() {
            return ADD_AIRCRAFT;
        }
    };

    struct AircraftAddedDto {
        std::string callsign;
        MSGPACK_DEFINE(callsign);

        static std::string getName() {
            return AIRCRAFT_ADDED;
        }
    };

    struct AircraftDeletedDto {
        std::string callsign;
        MSGPACK_DEFINE(callsign);

        static std::string getName() {
            return AIRCRAFT_DELETED;
        }
    };

    struct DeleteAircraftDto {
        std::string callsign;
        std::string reason;
        MSGPACK_DEFINE(callsign, reason);

        static std::string getName() {
            return DELETE_AIRCRAFT;
        }
    };

    struct DeleteAllAircraftDto {
        MSGPACK_DEFINE();
        static std::string getName() {
            return DELETE_ALL_AIRCRAFT;
        }
    };

    struct AircraftConfigDto {
        std::string callsign;
        std::optional<bool> fullConfig;
        std::optional<bool> enginesOn;
        std::optional<bool> enginesReversing;
        std::optional<bool> onGround;
        std::optional<float> flaps;
        std::optional<bool> spoilersDeployed;
        std::optional<bool> gearDown;
        std::optional<bool> beaconLightsOn;
        std::optional<bool> landingLightsOn;
        std::optional<bool> navLightsOn;
        std::optional<bool> strobeLightsOn;
        std::optional<bool> taxiLightsOn;
        MSGPACK_DEFINE(callsign, fullConfig, enginesOn, enginesReversing, onGround, flaps, gearDown, beaconLightsOn, landingLightsOn, navLightsOn, strobeLightsOn, taxiLightsOn);

        static std::string getName() {
            return AIRCRAFT_CONFIG;
        }
    };

    struct FastPositionUpdateDto {
        std::string callsign;
        double latitude;
        double longitude;
        double altitudeTrue;
        double altitudeAgl;
        double heading;
        double bank;
        double pitch;
        double vx;
        double vy;
        double vz;
        double vp;
        double vh;
        double vb;
        double noseWheelAngle;
        double speed;
        MSGPACK_DEFINE(callsign, latitude, longitude, altitudeTrue, altitudeAgl, heading, bank, pitch, vx, vy, vz, vp, vh, vb, noseWheelAngle, speed);

        static std::string getName() {
            return FAST_POSITION_UPDATE;
        }
    };

    struct HeartbeatDto {
        std::string callsign;
        MSGPACK_DEFINE(callsign);

        static std::string getName() {
            return HEARTBEAT;
        }
    };

    struct PluginVersionDto {
        int version;
        MSGPACK_DEFINE(version);

        static std::string getName() {
            return PLUGIN_VER;
        }
    };

    struct ValidateCslDto {
        bool isValid;
        MSGPACK_DEFINE(isValid);

        static std::string getName() {
            return VALIDATE_CSL;
        }
    };

    struct RadioMessageSentDto {
        std::string message;
        MSGPACK_DEFINE(message);

        static std::string getName() {
            return RADIO_MESSAGE_SENT;
        }
    };

    struct RadioMessageReceivedDto {
        std::string from;
        std::string message;
        bool isDirect;
        MSGPACK_DEFINE(from, message, isDirect);

        static std::string getName() {
            return RADIO_MESSAGE_RECEIVED;
        }
    };

    struct NotificationPostedDto {
        std::string message;
        int64_t color;
        MSGPACK_DEFINE(message, color);

        static std::string getName() {
            return NOTIFICATION_POSTED;
        }
    };

    struct PrivateMessageSentDto {
        std::string to;
        std::string message;
        MSGPACK_DEFINE(to, message);

        static std::string getName() {
            return PRIVATE_MESSAGE_SENT;
        }
    };

    struct PrivateMessageReceivedDto {
        std::string from;
        std::string message;
        MSGPACK_DEFINE(from, message);

        static std::string getName() {
            return PRIVATE_MESSAGE_RECEIVED;
        }
    };

    struct NearbyAtcStationDto {
        std::string callsign;
        std::string name;
        std::string frequency;
        int xplaneFrequency;
        MSGPACK_DEFINE(callsign, name, frequency, xplaneFrequency);
    };

    struct NearbyAtcDto {
        std::vector<NearbyAtcStationDto> stations;
        MSGPACK_DEFINE(stations);

        static std::string getName() {
            return NEARBY_ATC;
        }
    };

    struct RequestMetarDto {
        std::string station;
        MSGPACK_DEFINE(station);

        static std::string getName() {
            return REQUEST_METAR;
        }
    };

    struct RequestStationInfoDto {
        std::string station;
        MSGPACK_DEFINE(station);

        static std::string getName() {
            return REQUEST_STATION_INFO;
        }
    };

    struct WallopSentDto {
        std::string message;
        MSGPACK_DEFINE(message);

        static std::string getName() {
            return WALLOP_SENT;
        }
    };

    struct ForcedDisconnectDto {
        std::string reason;
        MSGPACK_DEFINE(reason);

        static std::string getName() {
            return FORCE_DISCONNECT;
        }
    };

    struct ConnectedDto {
        std::string callsign;
        std::string selcal;
        MSGPACK_DEFINE(callsign, selcal);

        static std::string getName() {
            return CONNECTED;
        }
    };

    struct DisconnectedDto {
        MSGPACK_DEFINE();

        static std::string getName() {
            return DISCONNECTED;
        }
    };

    struct ShutdownDto {
        MSGPACK_DEFINE();

        static std::string getName() {
            return SHUTDOWN;
        }
    };

    struct ComStationCallsign {
        int com;
        std::string callsign;
        MSGPACK_DEFINE(com, callsign);

        static std::string getName() {
            return STATION_CALLSIGN;
        }
    };

    // -------------------------------------------------------------

    template<class T>
    static bool encodeDto(msgpack::sbuffer &dtoBuf, const T &dto)
    {
        msgpack::zone z;
        BaseDto base{dto.getName(),msgpack::object(dto, z)};

        // because we don't know the dto size in advance, pack it into it's own
        // temp buffer, then copy it into the actual payload buffer.
        msgpack::sbuffer dtoTempBuf;
        msgpack::pack(dtoTempBuf, base);
        if (dtoTempBuf.size() > UINT16_MAX) {
            return false;
        }

        dtoBuf.write(dtoTempBuf.data(), dtoTempBuf.size());

        assert(dtoBuf.size() == dtoTempBuf.size());
        return true;
    }
}

#endif // DTO_H
