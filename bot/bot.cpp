#include "bot.hpp"
#include <string>
#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <regex>
#include <chrono>
#include <cpprest/ws_client.h>
#include <cpprest/http_client.h>
#include <cpprest/producerconsumerstream.h>
#include <chrono>

std::string find(std::string input, std::string start, std::string end) {
    int start_index = input.find(start) + start.length();

    int end_index = input.substr(start_index).find(end) + start_index;

    return input.substr(start_index, end_index - start_index);
}

std::string find_and_replace(std::string input, std::string replace_target, std::string replace_value) {
    std::string ret = input;

    int index = ret.find(replace_target);

    while (index != std::string::npos) {
        ret = ret.substr(0, index) + replace_value + ret.substr(index + replace_target.length(), input.length() - index);

        index = ret.find(replace_target);
    }

    return ret;
}


static const std::string base64_chars =
"ABCDEFGHIJKLMNOPQRSTUVWXYZ"
"abcdefghijklmnopqrstuvwxyz"
"0123456789+/";


static inline bool is_base64(BYTE c) {
    return true;
}
std::vector<BYTE> base64_decode(std::string const& encoded_string) {
    int in_len = encoded_string.size();
    int i = 0;
    int j = 0;
    int in_ = 0;
    BYTE char_array_4[4], char_array_3[3];
    std::vector<BYTE> ret;

    while (in_len-- && (encoded_string[in_] != '=') && is_base64(encoded_string[in_])) {
        char_array_4[i++] = encoded_string[in_]; in_++;
        if (i == 4) {
            for (i = 0; i < 4; i++)
                char_array_4[i] = base64_chars.find(char_array_4[i]);

            char_array_3[0] = (char_array_4[0] << 2) + ((char_array_4[1] & 0x30) >> 4);
            char_array_3[1] = ((char_array_4[1] & 0xf) << 4) + ((char_array_4[2] & 0x3c) >> 2);
            char_array_3[2] = ((char_array_4[2] & 0x3) << 6) + char_array_4[3];

            for (i = 0; (i < 3); i++)
                ret.push_back(char_array_3[i]);
            i = 0;
        }
    }

    if (i) {
        for (j = i; j < 4; j++)
            char_array_4[j] = 0;

        for (j = 0; j < 4; j++)
            char_array_4[j] = base64_chars.find(char_array_4[j]);

        char_array_3[0] = (char_array_4[0] << 2) + ((char_array_4[1] & 0x30) >> 4);
        char_array_3[1] = ((char_array_4[1] & 0xf) << 4) + ((char_array_4[2] & 0x3c) >> 2);
        char_array_3[2] = ((char_array_4[2] & 0x3) << 6) + char_array_4[3];

        for (j = 0; (j < i - 1); j++) ret.push_back(char_array_3[j]);
    }

    return ret;
}

bool send_message(std::string body, web::websockets::client::websocket_client client)
{
    try {
        web::websockets::client::websocket_outgoing_message msg;
        msg.set_utf8_message(body);
        client.send(msg);
        return true;
    }
    catch (...) {
        return false;
    }
}
std::string response(web::websockets::client::websocket_client client)
{
    try {
        auto clienta = client.receive();
        clienta.wait();

        auto t = clienta.get().extract_string();
        auto text = t.get();

        std::cout << text << "\r\n\r\n";

        return text;
    }
    catch (...) {
        return "error";
    }
}
namespace bot {
    int bots_connected;
    uint64_t go_time;
}

std::wstring get_socket_url(std::string game_code) {
    const auto time_since_epoch = []() -> uint64_t {
        return std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count();
    };

    std::string result;
    std::string headers;

    std::wstring url = L"https://kahoot.it/reserve/session/" + std::wstring(game_code.begin(), game_code.end()) + L"/?" + std::to_wstring(time_since_epoch()); //fuck unicode strings, all my homies hate unicode strings

    web::http::client::http_client http_client(url);

    auto page_request = http_client.request(L"GET");
    page_request.wait();

    auto body_request = page_request.get().extract_string();
    body_request.wait();

    auto z = body_request.get();

    result = std::string(z.begin(), z.end()); //death to unicode strings

    auto header_request = page_request.get().headers();
    auto token_result = header_request.find(L"x-kahoot-session-token");

    if (token_result == header_request.end())
        return L"";

    auto value = *token_result;

    std::string session_header = std::string(value.second.begin(), value.second.end());

    auto session_header_b64 = base64_decode(session_header);

    //x-kahoot-session-token

    std::string challenge_string = find(result, "challenge\":\"", "}");

    challenge_string = find_and_replace(challenge_string, "\\t", "");
    challenge_string = find_and_replace(challenge_string, " ", "");
    challenge_string = find_and_replace(challenge_string, "â€ƒ", "");
    challenge_string = find_and_replace(challenge_string, "", "");
    challenge_string = find(challenge_string, "=", ";");

    std::regex mul_re("(\\d+)\\*(\\d+)");
    //add 
    std::regex add_re("(\\d+)\\+(\\d+)");
    //subtract
    std::regex sub_re("(\\d+)\\-(\\d+)");
    //whole
    std::regex whole_re("\\((\\d+)\\)");

    std::smatch match;

    while (true)
    {
        //order of operations kids! back to 7th grade PEMDAS
        std::regex_search(challenge_string, match, mul_re);
        while (!match.empty()) {
            int a = std::stoi(match[1].str()) * std::stoi(match[2].str());
            challenge_string = find_and_replace(challenge_string, match[0].str(), std::to_string(a));
            std::regex_search(challenge_string, match, mul_re);
        }

        std::regex_search(challenge_string, match, add_re);
        while (!match.empty()) {
            int a = std::stoi(match[1].str()) + std::stoi(match[2].str());
            challenge_string = find_and_replace(challenge_string, match[0].str(), std::to_string(a));
            std::regex_search(challenge_string, match, add_re);
        }

        std::regex_search(challenge_string, match, sub_re);
        while (!match.empty()) {
            int a = std::stoi(match[1].str()) - std::stoi(match[2].str());
            challenge_string = find_and_replace(challenge_string, match[0].str(), std::to_string(a));
            std::regex_search(challenge_string, match, sub_re);
        }

        bool triggered = false;
        std::regex_search(challenge_string, match, whole_re);
        if (!match.empty()) {
            challenge_string = find_and_replace(challenge_string, match[0].str(), match[1].str());
            std::regex_search(challenge_string, match, whole_re);
            triggered = true;
        }

        if (!triggered)
            break;
    }

    int code_offset = std::stoi(challenge_string);

    std::string call_string = find(result, "decode.call(this, \'", "\');"); //find the paramter in the call_stirng function

    std::string decoded_challenge_string{}; //empty string after we do the kahoot voodoo on it

    for (int i = 0; i < call_string.length(); i++)
    {
        char decoded_char = (char)((((call_string[i] * i) + code_offset) % 77) + 48);
        decoded_challenge_string.push_back(decoded_char);
    }

    std::string socket_token{};
    for (int i = 0; i < session_header_b64.size(); i++) //a little more kahoot voodoo;
    {
        char decoded_char = (char)(session_header_b64[i] ^ decoded_challenge_string[i % decoded_challenge_string.length()]);
        socket_token.push_back(decoded_char);
    }
    //the holy grail!

    //connect

    std::wstring socket_url = L"wss://kahoot.it/cometd/" + std::wstring(game_code.begin(), game_code.end()) + L"/" + std::wstring(socket_token.begin(), socket_token.end()); //i hate the unicode string! i hate the unicode string! i hate the unicode string!

    return socket_url;
}

void bot::run(std::string game_code, std::string name, int waitfor) {
    std::this_thread::sleep_for(std::chrono::milliseconds(waitfor));
    try {
        const auto time_since_epoch = []() -> uint64_t {
            return std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count();
        };

        bool valid_token = false;

        web::websockets::client::websocket_client client = web::websockets::client::websocket_client();
        while (!valid_token) {
            client = web::websockets::client::websocket_client();
            std::wstring socket_url = get_socket_url(game_code);
            try {
                auto con = client.connect(socket_url).then([]() { std::cout << "connected!"; });
                valid_token = true;
                con.wait();
            }
            catch (...) {
                //bad decode
                continue;
            }
        }

        //it’s all a matter of rhythm till we finally get our turn
        //i have 0 clue why the breaks are needed, on Google they are not. but it will refuse to function without them. they couldn't handle the neutron style I guess lol
        std::string ptr = "[{\"id\":\"1\",\"version\":\"1.0\",\"minimumVersion\":\"1.0\",\"channel\":\"/meta/handshake\",\"supportedConnectionTypes\":[\"websocket\",\"long-polling\",\"callback-polling\"],\"advice\":{\"timeout\":60000,\"interval\":0},\"ext\":{\"ack\":true,\"timesync\":{\"tc\":" + std::to_string(time_since_epoch()) + ",\"l\":0,\"o\":0}}}]";

        send_message(ptr, client);

        std::string client_id = find(response(client), "\"clientId\":\"", "\"");

        ptr = "[{\"id\":\"2\",\"channel\":\"/meta/connect\",\"connectionType\":\"websocket\",\"advice\":{\"timeout\":0},\"clientId\":\"" + client_id + "\",\"ext\":{\"ack\":0,\"timesync\":{\"tc\":" + std::to_string(time_since_epoch()) + ",\"l\":47,\"o\":-189}}}]";

        send_message(ptr, client);
        std::this_thread::sleep_for(std::chrono::milliseconds(250));
        response(client);
        std::this_thread::sleep_for(std::chrono::milliseconds(250));
        std::string ptr2 = "[{\"id\":\"3\",\"channel\":\"/meta/connect\",\"connectionType\":\"websocket\",\"clientId\":\"" + client_id + "\",\"ext\":{\"ack\":0,\"timesync\":{\"tc\":" + std::to_string(time_since_epoch()) + ",\"l\":47,\"o\":-189}}}]";
        bots_connected++;
        while (go_time > time_since_epoch()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(250));
        }

        ptr = "[{\"id\":\"4\",\"channel\":\"/service/controller\",\"data\":{\"type\":\"login\",\"gameid\":\"" + game_code + "\",\"host\":\"kahoot.it\",\"name\":\"" + name + "\",\"content\":\"{\\\"device\\\":{\\\"userAgent\\\":\\\"Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/87.0.4280.88 Safari/537.36\\\",\\\"screen\\\":{\\\"width\\\":2560,\\\"height\\\":1440}}}\"},\"clientId\":\"" + client_id + "\",\"ext\":{ }}]";
        send_message(ptr2, client);
        std::this_thread::sleep_for(std::chrono::milliseconds(250));
        send_message(ptr, client);
        std::this_thread::sleep_for(std::chrono::milliseconds(250));


        response(client);
        response(client);
        response(client);

        ptr = "[{\"id\":\"5\",\"channel\":\"/meta/connect\",\"connectionType\":\"websocket\",\"clientId\":\"" + client_id + "\",\"ext\":{\"ack\":2,\"timesync\":{\"tc\":" + std::to_string(time_since_epoch()) + ",\"l\":47,\"o\":-189}}}]";
        send_message(ptr, client);

        response(client);
        response(client);
        std::this_thread::sleep_for(std::chrono::milliseconds(100));

        client.close();
    }
    catch (...) {
        return;
    }
}