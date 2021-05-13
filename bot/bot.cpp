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

/* find
* finds the value between two strings because regex was acting horrible 
*/
std::string find(std::string input, std::string start, std::string end) {
    int start_index = input.find(start) + start.length();

    int end_index = input.substr(start_index).find(end) + start_index;

    return input.substr(start_index, end_index - start_index);
}

/* find and replace
* finds each occurance of a certain value and replaces it
*/
std::string find_and_replace(std::string input, std::string replace_target, std::string replace_value) {
    std::string ret = input;

    int index = ret.find(replace_target);

    while (index != std::string::npos) {
        ret = ret.substr(0, index) + replace_value + ret.substr(index + replace_target.length(), input.length() - index);

        index = ret.find(replace_target);
    }

    return ret;
}

static const std::string base64_chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/"; //character array

std::vector<BYTE> base64_decode(std::string const& encoded_string) {
    int in_len = encoded_string.size();
    int i = 0;
    int j = 0;
    int in_ = 0;
    BYTE char_array_4[4], char_array_3[3];
    std::vector<BYTE> ret;

    while (in_len-- && (encoded_string[in_] != '=')) {
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

/* send_message
*  sends a message. wowzers
*/
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
/* response
*  simple response handler
*/
std::string response(web::websockets::client::websocket_client client)
{
    try {
        auto clienta = client.receive();
        clienta.wait();

        auto t = clienta.get().extract_string();
        auto text = t.get();

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

/* bot::get_socket_url
* generates a socket url from a kahoot gamesession
*/
std::wstring bot::get_socket_url(std::string game_code) {
    const auto time_since_epoch = []() -> uint64_t {
        return std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count();
    };

    std::string result;

    //initialize our url
    std::wstring url = L"https://kahoot.it/reserve/session/" + std::wstring(game_code.begin(), game_code.end()) + L"/?" + std::to_wstring(time_since_epoch()); //fuck unicode strings, all my homies hate unicode strings

    //create our client
    web::http::client::http_client http_client(url);

    //GET request to the pge
    auto page_request = http_client.request(L"GET");
    page_request.wait();
    
    //get the string
    auto body_request = page_request.get().extract_string();
    body_request.wait();

    auto z = body_request.get();

    result = std::string(z.begin(), z.end()); //death to unicode strings

    //find the x-kahoot-session-token header
    auto header_request = page_request.get().headers();
    auto token_result = header_request.find(L"x-kahoot-session-token");

    if (token_result == header_request.end())
        return L""; //headers got messed up

    auto value = *token_result;

    std::string session_header = std::string(value.second.begin(), value.second.end()); //no unicode

    //decode the session header so it gives us random gibberish. don't like it? blame kahoot not me.
    auto session_header_b64 = base64_decode(session_header);

    //x-kahoot-session-token

    std::string challenge_string = find(result, "challenge\":\"", "}");
    //purge the impure unicode and special symbols
    challenge_string = find_and_replace(challenge_string, "\\t", "");
    challenge_string = find_and_replace(challenge_string, " ", "");
    challenge_string = find_and_replace(challenge_string, "â€ƒ", "");
    challenge_string = find_and_replace(challenge_string, "", "");
    challenge_string = find(challenge_string, "=", ";");

    /* given input (3*4)+1
    * return 13
    * this is basically just a javascript interpreter on a budget
    */

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

    std::string call_string = find(result, "decode.call(this, \'", "\');"); //find the paramter in the call_string function

    std::string decoded_challenge_string;

    for (int i = 0; i < call_string.length(); i++)
    {
        char decoded_char = (char)((((call_string[i] * i) + code_offset) % 77) + 48); //kahoot has some really interesting techniques
        decoded_challenge_string.push_back(decoded_char);
    }

    std::string socket_token{};
    for (int i = 0; i < session_header_b64.size(); i++) //a little more kahoot voodoo;
    {
        char decoded_char = (char)(session_header_b64[i] ^ decoded_challenge_string[i % decoded_challenge_string.length()]); //like really interesting
        socket_token.push_back(decoded_char);
    }
    //the holy grail!

    std::wstring socket_url = L"wss://kahoot.it/cometd/" + std::wstring(game_code.begin(), game_code.end()) + L"/" + std::wstring(socket_token.begin(), socket_token.end()); //i hate the unicode string! i hate the unicode string! i hate the unicode string!

    return socket_url;
}

/* bot::run
* runs a bot
*/
void bot::run(std::string game_code, std::string name, int waitfor) {
    std::this_thread::sleep_for(std::chrono::milliseconds(waitfor)); //delay, my program will literally break the kahoot api if i send 100 bots at it all at once.
    try {
        const auto time_since_epoch = []() -> uint64_t {
            return std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count();
        };

        bool valid_token = false; //my javascript interpreter isn't perfect, it only works 90% of the time.

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

        //we got a good code
        //it’s all a matter of rhythm till we finally get our turn
        //i have 0 clue why the breaks are needed, on Google they are not. but it will refuse to function without them. they couldn't handle the neutron style I guess lol
        std::string message = "[{\"id\":\"1\",\"version\":\"1.0\",\"minimumVersion\":\"1.0\",\"channel\":\"/meta/handshake\",\"supportedConnectionTypes\":[\"websocket\",\"long-polling\",\"callback-polling\"],\"advice\":{\"timeout\":60000,\"interval\":0},\"ext\":{\"ack\":true,\"timesync\":{\"tc\":" + std::to_string(time_since_epoch()) + ",\"l\":0,\"o\":0}}}]";

        send_message(message, client);

        std::string client_id = find(response(client), "\"clientId\":\"", "\"");

        message = "[{\"id\":\"2\",\"channel\":\"/meta/connect\",\"connectionType\":\"websocket\",\"advice\":{\"timeout\":0},\"clientId\":\"" + client_id + "\",\"ext\":{\"ack\":0,\"timesync\":{\"tc\":" + std::to_string(time_since_epoch()) + ",\"l\":47,\"o\":-189}}}]";

        send_message(message, client);
        std::this_thread::sleep_for(std::chrono::milliseconds(250));

        response(client);
        std::this_thread::sleep_for(std::chrono::milliseconds(250));

        bots_connected++;

        while (go_time > time_since_epoch()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(250)); //wait for the others
        }

        message = "[{\"id\":\"4\",\"channel\":\"/service/controller\",\"data\":{\"type\":\"login\",\"gameid\":\"" + game_code + "\",\"host\":\"kahoot.it\",\"name\":\"" + name + "\",\"content\":\"{\\\"device\\\":{\\\"userAgent\\\":\\\"Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/87.0.4280.88 Safari/537.36\\\",\\\"screen\\\":{\\\"width\\\":2560,\\\"height\\\":1440}}}\"},\"clientId\":\"" + client_id + "\",\"ext\":{ }}]";
        std::string message2 = "[{\"id\":\"3\",\"channel\":\"/meta/connect\",\"connectionType\":\"websocket\",\"clientId\":\"" + client_id + "\",\"ext\":{\"ack\":0,\"timesync\":{\"tc\":" + std::to_string(time_since_epoch()) + ",\"l\":47,\"o\":-189}}}]";

        send_message(message2, client);
        std::this_thread::sleep_for(std::chrono::milliseconds(250));

        send_message(message, client);
        std::this_thread::sleep_for(std::chrono::milliseconds(250));

        /* kahoot will send 3 responses back */
        response(client);
        response(client);
        response(client);

        message = "[{\"id\":\"5\",\"channel\":\"/meta/connect\",\"connectionType\":\"websocket\",\"clientId\":\"" + client_id + "\",\"ext\":{\"ack\":2,\"timesync\":{\"tc\":" + std::to_string(time_since_epoch()) + ",\"l\":47,\"o\":-189}}}]";
        send_message(message, client);
\

        /*that is all to connect, now we will simply respond "uhuhuh yea i'm here"*/
        /* if they are kicked or time out (WIP), they will reconnect */
        int id = 6;
        while (true) {
            std::string res = response(client);

            if (res.find("kickCode") != std::string::npos || res == "error") { //dropped by client or kicked
                run(game_code, name+"1", 0);
                return;
            }
            //not working, fix this
            message = "[{\"id\":\""+std::to_string(id)+"\", \"channel\":\"/meta/connect\", \"connectionType\" : \"websocket\", \"clientId\":\""+client_id+"\", \"ext\" : {\"ack\":"+std::to_string(id-1)+", \"timesync\" : {\"tc\":"+std::to_string(time_since_epoch()) +", \"l\" : 42, \"o\" : 111}}}]";
            send_message(message, client);
            id++;
        }

        client.close();
    }
    catch (...) {
        return;
    }
}