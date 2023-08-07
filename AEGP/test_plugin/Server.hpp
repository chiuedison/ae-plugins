#ifndef Server_hpp
#define Server_hpp

#include <cstdio>

#include "json.hpp"
#include <iostream>
#include <thread>
#include <chrono>
#include <vector>

#include <sio_client.h>
#include <sio_message.h>
#include <sio_socket.h>

#define PROTOCOL_VERSION 1
#define COMMAND_GET_PATH "path"
#define COMMAND_CAMERA_DATA "cameraData"

#define SOCKET_URL "ws://127.0.0.1:5555"

using namespace nlohmann;

class Connection {
private:
    static json addCommandHeader(json &body,
                                 const std::string &command,
                                 const std::string &commandId) {
        json fullMessage = "{}"_json;
        fullMessage["command"] = command;
        fullMessage["version"] = SHADE_PROTOCOL_VERSION;
        fullMessage["id"] = commandId;
        fullMessage["body"] = body;

        return fullMessage;
    }

    static std::string serialize(json &j) {
        return j.dump();
    }

    static json addToBody(json j,
                          const std::string &key,
                          const std::string &value) {
        (j)[key] = value;
        return j;
    }
public:
    sio::client client;
    sio::socket::ptr socket;
    bool connected = false;
    std::string savedFilePath;
    static bool setup;

    Connection();

    void onMsg(const std::string &msg);

    static nlohmann::json parseJson(const std::string &input) {
        return json::parse(input);
    }

    void updatePaths(std::string &path);

    bool ensureConnection();

    void sendString(const std::string &msg) const;

    void connect();

    static std::string formatPath(std::string *commandId, std::string &paths) {
        json body = "{}"_json;

        body["path"] = paths;

        json data = addCommandHeader(body, COMMAND_GET_PATH, *commandId);

        std::string serializedOutput = serialize(data);
        std::cout << serializedOutput << std::endl;

        return serializedOutput;
    }
};
#endif /* Server_hpp */
