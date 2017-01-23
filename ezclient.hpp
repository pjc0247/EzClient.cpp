#pragma once

#include <string>

#include "json.hpp"
#include "easywsclient.hpp"

#ifdef _WIN32
#pragma comment( lib, "ws2_32" )
#include <WinSock2.h>
#endif

#define __EZ_BEGIN_PACKET(name) \
    struct __##name : public packet_base { \
        static const char *__type; \
    }; \
    const char *__##name::__type = "GSF.Ez.Packet."#name##", GSF.Ez.Packet"; \
    struct name : public __##name, public ez::ijsonconvertible { \
        name() { } 
#define __EZ_END_PACKET };

namespace ez {
    struct ijsonconvertible {
        virtual nlohmann::json to_json() const { throw std::exception("not implemented"); };
    };

    struct ezproperties : ijsonconvertible {
        template <typename T>
        void set(const std::string &key, T &&o) {
            data[key] = o;
        }

        template <typename T>
        typename std::enable_if<std::is_same<T, std::string>::value, const std::string>::type
            get(const std::string &key) {
            return data[key].get<std::string>();
        }
        template <typename T>
        typename std::enable_if<std::is_same<T, int>::value, int>::type
            get(const std::string &key) {
            return data[key].get<int>();
        }
        template <typename T>
        typename std::enable_if<std::is_same<T, long>::value, long>::type
            get(const std::string &key) {
            return data[key].get<long>();
        }
        template <typename T>
        typename std::enable_if<std::is_same<T, float>::value, float>::type
            get(const std::string &key) {
            return data[key].get<float>();
        }
        template <typename T>
        typename std::enable_if<std::is_same<T, float>::value, double>::type
            get(const std::string &key) {
            return data[key].get<double>();
        }

        template <typename T>
        typename std::enable_if<std::is_same<T, std::vector<T>>::value, std::vector<T>>::type
            get(const std::string &key) {

            std::vector<T> v;
            for (auto &item : data[key])
                v.push_back(item);
            return v;
        }

        virtual nlohmann::json to_json() const {
            nlohmann::json jobj = "{}"_json;

            for (auto &kv : data)
                jobj[kv.first] = kv.second;

            return jobj;
        }
        static ezproperties from_json(const nlohmann::json &json) {
            ezproperties properties;

            for (auto it = json.begin(); it != json.end(); ++it) {
                properties.data[it.key()] = it.value();
            }

            return properties;
        }

    private:
        std::map<std::string, nlohmann::json> data;
    };

    struct ezplayer : ijsonconvertible {
        std::string player_id;

        ezproperties property;

        virtual nlohmann::json to_json() const {
            nlohmann::json jobj;

            jobj["PlayerId"] = player_id;
            jobj["Property"] = property.to_json();

            return jobj;
        }

        static ezplayer from_json(const nlohmann::json &json) {
            ezplayer player;

            player.player_id = json["PlayerId"].get<std::string>();
            player.property = ezproperties::from_json(json["Property"]);

            return player;
        }
    };

    struct packet_base {
        int packet_id;

        packet_base() { }
    };

    __EZ_BEGIN_PACKET(JoinPlayer)
        ezplayer player;

        virtual nlohmann::json to_json() const {
            nlohmann::json jobj;

            jobj["__type"] = ez::JoinPlayer::__type;
            jobj["Player"] = player.to_json();

            return jobj;
        }

        static JoinPlayer from_json(const nlohmann::json &json) {
            JoinPlayer packet;

            packet.player = ezplayer::from_json(json["Player"]);

            return packet;
        }
    __EZ_END_PACKET
    __EZ_BEGIN_PACKET(LeavePlayer)
        ezplayer player;

        virtual nlohmann::json to_json() const {
            nlohmann::json jobj;

            jobj["__type"] = ez::LeavePlayer::__type;
            jobj["Player"] = player.to_json();

            return jobj;
        }

        static LeavePlayer from_json(const nlohmann::json &json) {
            LeavePlayer packet;

            packet.player = ezplayer::from_json(json["Player"]);

            return packet;
        }
    __EZ_END_PACKET

    __EZ_BEGIN_PACKET(WorldInfo)
        ezproperties property;

        ezplayer player;
        std::vector<ezplayer> other_players;

        static WorldInfo from_json(const nlohmann::json &json) {
            WorldInfo packet;

            packet.player = ezplayer::from_json(json["Player"]);
            packet.property = ezproperties::from_json(json["Property"]);
            for (auto &player : json["OtherPlayers"])
                packet.other_players.push_back(ezplayer::from_json(player));

            return packet;
        }
    __EZ_END_PACKET

    __EZ_BEGIN_PACKET(ModifyPlayerProperty)
        ezplayer player;

        ezproperties property;
        std::vector<std::string> removed_keys;

        virtual nlohmann::json to_json() const {
            nlohmann::json jobj;

            jobj["__type"] = ez::ModifyPlayerProperty::__type;
            jobj["Player"] = player.to_json();
            jobj["Property"] = property.to_json();
            jobj["RemovedKeys"] = removed_keys;

            return jobj;
        }

        static ModifyPlayerProperty from_json(const nlohmann::json &json) {
            ModifyPlayerProperty packet;

            packet.player = ezplayer::from_json(json["Player"]);
            packet.property = ezproperties::from_json(json["Property"]);
            for (auto &key : json["RemovedKeys"])
                packet.removed_keys.push_back(key);

            return packet;
        }
    __EZ_END_PACKET

    __EZ_BEGIN_PACKET(BroadcastPacket)
        ezplayer sender;

        int type;
        ezproperties data;

        static BroadcastPacket from_json(const nlohmann::json &json) {
            BroadcastPacket packet;

            packet.sender = ezplayer::from_json(json["Sender"]);
            packet.type = json["Type"].get<int>();
            packet.data = ezproperties::from_json(json["Data"]);

            return packet;
        }
    __EZ_END_PACKET
    __EZ_BEGIN_PACKET(RequestBroadcast)
        int type;
        ezproperties data;

        virtual nlohmann::json to_json() const {
            nlohmann::json jobj;

            jobj["__type"] = ez::RequestBroadcast::__type;
            jobj["Type"] = type;
            jobj["Data"] = data.to_json();

            return jobj;
        }
    __EZ_END_PACKET
}

class ezclient {
public:
    static ezclient *create(const std::string &uri) {
        return new ezclient(uri);
    }

    void dispatch() {
        client->poll();
        client->dispatch([this](const std::string &message) {
            on_message(message);
        });
    }

    template <typename T>
    void set_player_property(const std::string &key, T &&value) {
        ez::ezproperties p;
        p.set(key, value);
        set_player_property(p);
    }
    void set_player_property(const ez::ezproperties &properties) {
        ez::ModifyPlayerProperty packet;
        packet.property = properties;
        send_packet(packet.to_json().dump());
    }
    void send_packet(int packet_type, const ez::ezproperties &data) {
        ez::RequestBroadcast packet;
        packet.type = packet_type;
        packet.data = data;
        send_packet(packet.to_json().dump());
    }

private:
    ezclient(std::string const &uri) {
        client = easywsclient::WebSocket::from_url(uri);

        ez::JoinPlayer packet;
        packet.player.player_id = "asdf";

        send_packet(packet.to_json().dump());
    }

    void on_message(const std::string &message) {
        std::cout << "RECV : " << message << std::endl;

        auto jobj = nlohmann::json::parse(message.begin(), message.end());
        auto type = jobj["__type"].get<std::string>();

        if (type == ez::WorldInfo::__type) {
            auto packet = ez::WorldInfo::from_json(jobj);

			player = packet.player;
			other_players = packet.other_players;

            if (on_worldinfo != nullptr)
                on_worldinfo(packet);
        }
        else if (type == ez::JoinPlayer::__type) {
            auto packet = ez::JoinPlayer::from_json(jobj);

			other_players.push_back(packet.player);

            if (on_joinplayer != nullptr)
                on_joinplayer(packet);
        }
        else if (type == ez::LeavePlayer::__type) {
            auto packet = ez::LeavePlayer::from_json(jobj);

			other_players.erase(
				std::find_if(other_players.begin(), other_players.end(),
				[&packet](ez::ezplayer &p) {
					return p.player_id == packet.player.player_id;
				}));

            if (on_leaveplayer != nullptr)
                on_leaveplayer(packet);
        }   
        else if (type == ez::ModifyPlayerProperty::__type) {
            auto packet = ez::ModifyPlayerProperty::from_json(jobj);

            if (on_modifyplayerproperty != nullptr)
                on_modifyplayerproperty(packet);
        }
        else if (type == ez::BroadcastPacket::__type) {
            auto packet = ez::BroadcastPacket::from_json(jobj);

            if (on_custompacket != nullptr)
                on_custompacket(packet);
        }
    }

    void send_packet(const std::string &json) {
        std::cout << "SEND : " << json << std::endl;
        client->sendBinary(json);
    }

public:
    std::function<void(ez::WorldInfo)> on_worldinfo;
    std::function<void(ez::JoinPlayer)> on_joinplayer;
    std::function<void(ez::LeavePlayer)> on_leaveplayer;
    std::function<void(ez::ModifyPlayerProperty)> on_modifyplayerproperty;
    std::function<void(ez::BroadcastPacket)> on_custompacket;

	ez::ezplayer player;
	std::vector<ez::ezplayer> other_players;

private:
    easywsclient::WebSocket *client;
};
