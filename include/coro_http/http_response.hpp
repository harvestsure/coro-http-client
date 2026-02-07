#pragma once

#include <string>
#include <map>

namespace coro_http {

class HttpResponse {
public:
    HttpResponse() : status_code_(0) {}

    void set_status_code(int code) { status_code_ = code; }
    void set_reason(const std::string& reason) { reason_ = reason; }
    void add_header(const std::string& key, const std::string& value) {
        headers_[key] = value;
    }
    void set_body(const std::string& body) { body_ = body; }

    int status_code() const { return status_code_; }
    const std::string& reason() const { return reason_; }
    const std::map<std::string, std::string>& headers() const { return headers_; }
    const std::string& body() const { return body_; }

private:
    int status_code_;
    std::string reason_;
    std::map<std::string, std::string> headers_;
    std::string body_;
};

}
