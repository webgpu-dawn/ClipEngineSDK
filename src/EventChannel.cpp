#include "EventChannel.h"

#include <websocketpp/config/asio_no_tls.hpp>
#include <websocketpp/server.hpp>

using namespace std;
typedef websocketpp::server<websocketpp::config::asio> server;

void EventChannel::init()
{
    init_websocket();
}

void EventChannel::init_websocket()
{
    std::thread t([this](){
        server echo_server;

        echo_server.init_asio();

        echo_server.set_open_handler([&echo_server](websocketpp::connection_hdl hdl) {
            std::cout << "Client connected\n";
            echo_server.send(hdl, "connect", websocketpp::frame::opcode::text);
        });

        echo_server.set_message_handler([&echo_server, this](websocketpp::connection_hdl hdl, server::message_ptr msg) {
            std::string payload = msg->get_payload();

            try {
                json j = json::parse(payload);

                const std::string event = j["event"];
                EventCallback cb = events_[event];
                if(cb) {
                   cb(j);
                }
            } catch (std::exception& e) {
                std::cerr << "JSON parse error: " << e.what() << std::endl;
            }
            // echo_server.send(hdl, msg->get_payload(), msg->get_opcode());
        });

        echo_server.listen(9002);
        echo_server.start_accept();

        echo_server.run();
    });
    t.detach();
}

void EventChannel::subscribe(std::string event, EventCallback callback)
{
    events_[event] = callback;
}