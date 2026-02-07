#pragma once

#include <chrono>
#include <string>

namespace coro_http {

struct ClientConfig {
    std::chrono::milliseconds connect_timeout{30000};
    std::chrono::milliseconds read_timeout{30000};
    std::chrono::milliseconds request_timeout{60000};
    
    bool follow_redirects{true};
    int max_redirects{10};
    
    bool enable_compression{true};
    
    bool verify_ssl{false};
    std::string ca_cert_file;
    std::string ca_cert_path;
    
    std::string proxy_url;
    std::string proxy_username;
    std::string proxy_password;
};

}
