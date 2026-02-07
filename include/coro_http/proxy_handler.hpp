#pragma once

#include <string>
#include <regex>
#include <stdexcept>

namespace coro_http {

enum class ProxyType {
    NONE,
    HTTP,
    HTTPS,
    SOCKS5
};

struct ProxyInfo {
    ProxyType type{ProxyType::NONE};
    std::string host;
    std::string port;
    std::string username;
    std::string password;
};

inline ProxyInfo parse_proxy_url(const std::string& proxy_url) {
    ProxyInfo info;
    
    if (proxy_url.empty()) {
        return info;
    }
    
    std::regex proxy_regex(R"(^(https?|socks5):\/\/([^:\/\s]+)(?::(\d+))?$)");
    std::smatch matches;
    
    if (std::regex_search(proxy_url, matches, proxy_regex)) {
        std::string scheme = matches[1].str();
        info.host = matches[2].str();
        info.port = matches[3].matched ? matches[3].str() : "1080";
        
        if (scheme == "http") {
            info.type = ProxyType::HTTP;
            if (!matches[3].matched) info.port = "8080";
        } else if (scheme == "https") {
            info.type = ProxyType::HTTPS;
            if (!matches[3].matched) info.port = "8080";
        } else if (scheme == "socks5") {
            info.type = ProxyType::SOCKS5;
        }
    } else {
        throw std::runtime_error("Invalid proxy URL format");
    }
    
    return info;
}

inline std::string build_connect_request(const std::string& host, const std::string& port, 
                                         const std::string& proxy_username = "", 
                                         const std::string& proxy_password = "") {
    std::ostringstream req;
    req << "CONNECT " << host << ":" << port << " HTTP/1.1\r\n";
    req << "Host: " << host << ":" << port << "\r\n";
    
    if (!proxy_username.empty()) {
        std::string credentials = proxy_username + ":" + proxy_password;
        std::string encoded = "Basic " + credentials;
        req << "Proxy-Authorization: " << encoded << "\r\n";
    }
    
    req << "\r\n";
    return req.str();
}

inline bool parse_connect_response(const std::string& response) {
    std::istringstream stream(response);
    std::string line;
    
    if (std::getline(stream, line)) {
        if (line.back() == '\r') line.pop_back();
        
        std::istringstream status_line(line);
        std::string http_version;
        int status_code;
        
        status_line >> http_version >> status_code;
        
        return status_code == 200;
    }
    
    return false;
}

inline std::string build_socks5_handshake(bool use_auth) {
    std::string handshake;
    handshake.push_back(0x05);
    if (use_auth) {
        handshake.push_back(0x02);
        handshake.push_back(0x00);
        handshake.push_back(0x02);
    } else {
        handshake.push_back(0x01);
        handshake.push_back(0x00);
    }
    return handshake;
}

inline std::string build_socks5_auth(const std::string& username, const std::string& password) {
    std::string auth;
    auth.push_back(0x01);
    auth.push_back(static_cast<char>(username.size()));
    auth.append(username);
    auth.push_back(static_cast<char>(password.size()));
    auth.append(password);
    return auth;
}

inline std::string build_socks5_connect(const std::string& host, const std::string& port) {
    std::string request;
    request.push_back(0x05);
    request.push_back(0x01);
    request.push_back(0x00);
    request.push_back(0x03);
    request.push_back(static_cast<char>(host.size()));
    request.append(host);
    
    int port_num = std::stoi(port);
    request.push_back(static_cast<char>((port_num >> 8) & 0xFF));
    request.push_back(static_cast<char>(port_num & 0xFF));
    
    return request;
}

inline bool parse_socks5_response(const std::string& response, size_t min_size = 2) {
    if (response.size() < min_size) return false;
    return response[1] == 0x00;
}

}
