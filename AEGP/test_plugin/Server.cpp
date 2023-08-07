#include "Server.hpp"
#include "test.h"

class ConnectionExists : public std::exception {};

using namespace nlohmann;

Connection::Connection() {
    // This should be zero when its created but one after being set
    if (setup) {
        throw ConnectionExists();
    }
}

void Connection::connect() {
    setup = true;

    std::cout << "Setting up connection" << std::endl;

    socket = client.socket();

    socket->on("msg", ([&](std::string const& name, sio::message::ptr const& data, bool isAck, sio::message::list &ack_resp){
        onMsg(data->get_string());
        std::cout << "Received: " << data->get_string() << std::endl;
    }));

    std::cout << "validated connection" << std::endl;
    ensureConnection();
}

void Connection::updatePaths(std::string &path) {
    savedFilePath = path;
}

void Connection::onMsg(const std::string &msg) {
    if (msg == "pong") {
        connected = true;
        std::cout << "Connected!" << std::endl;
        return;
    }

    json j = json::parse(msg);
    std::string command = j["command"];
    std::string queryId = j["id"];

    if (command == COMMAND_GET_PATH) {
        std::string path = "off to oofland";
        sendString(formatPath(&queryId, savedFilePath));
    }
    if (command == COMMAND_CAMERA_DATA) {
        std::cout << "Got new camea data";
        // Lets pretend for a moment that this this a vector of XYZ data
        std::vector<std::vector<float>> vect = {
            {1.0,1.0,1.0},
            {2.0,2.0,2.0},
            {3.0,3.0,3.0}
        };
        
        addTrackKeyFrames(vect);
    }
}

bool Connection::ensureConnection() {
    if (!connected) {
        // Wait for connection message to be received in other thread
        for (int i = 0; i < 100; i++) {
            if (connected) {
                return true;
            }
            client.connect(SOCKET_URL);
            sendString("ping");
            std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        }
        std::cout << "Failed to connect" << std::endl;
        return false;
    }
    return true;
}

void Connection::sendString(const std::string &msg) const {
    std::cout << "Sending from C++: " << msg << std::endl;
    socket->emit("msg", msg);
}

bool Connection::setup = false;
