#pragma once

#include "http_response.hpp"
#include <string>
#include <sstream>

namespace coro_http {

inline HttpResponse parse_response(const std::string& response_data) {
    HttpResponse response;
    std::istringstream stream(response_data);
    std::string line;

    if (std::getline(stream, line)) {
        if (line.back() == '\r') line.pop_back();
        
        std::istringstream status_line(line);
        std::string http_version;
        int status_code;
        std::string reason;
        
        status_line >> http_version >> status_code;
        std::getline(status_line, reason);
        if (!reason.empty() && reason[0] == ' ') reason = reason.substr(1);
        
        response.set_status_code(status_code);
        response.set_reason(reason);
    }

    while (std::getline(stream, line) && line != "\r") {
        if (line.back() == '\r') line.pop_back();
        
        auto colon_pos = line.find(':');
        if (colon_pos != std::string::npos) {
            std::string key = line.substr(0, colon_pos);
            std::string value = line.substr(colon_pos + 1);
            
            while (!value.empty() && value[0] == ' ') value = value.substr(1);
            while (!value.empty() && value.back() == ' ') value.pop_back();
            
            response.add_header(key, value);
        }
    }

    std::string body;
    std::string remaining((std::istreambuf_iterator<char>(stream)), std::istreambuf_iterator<char>());
    response.set_body(remaining);

    return response;
}

inline std::string build_request(const HttpRequest& request, const UrlInfo& url_info) {
    std::ostringstream req;
    
    req << method_to_string(request.method()) << " " << url_info.path << " HTTP/1.1\r\n";
    req << "Host: " << url_info.host << "\r\n";
    
    for (const auto& [key, value] : request.headers()) {
        req << key << ": " << value << "\r\n";
    }
    
    if (!request.body().empty()) {
        req << "Content-Length: " << request.body().size() << "\r\n";
    }
    
    req << "Connection: close\r\n";
    req << "\r\n";
    
    if (!request.body().empty()) {
        req << request.body();
    }
    
    return req.str();
}

}
