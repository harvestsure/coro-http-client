#pragma once

#include "http_request.hpp"
#include "http_response.hpp"
#include "url_parser.hpp"
#include "http_parser.hpp"
#include <asio.hpp>
#include <asio/ssl.hpp>
#include <system_error>

namespace coro_http {

class HttpClient {
public:
    HttpClient() : io_context_(), ssl_context_(asio::ssl::context::tlsv12_client) {
        ssl_context_.set_default_verify_paths();
        ssl_context_.set_verify_mode(asio::ssl::verify_none);
    }

    HttpResponse execute(const HttpRequest& request) {
        auto url_info = parse_url(request.url());
        
        if (url_info.is_https) {
            return execute_https(request, url_info);
        } else {
            return execute_http(request, url_info);
        }
    }

private:
    HttpResponse execute_http(const HttpRequest& request, const UrlInfo& url_info) {
        asio::ip::tcp::resolver resolver(io_context_);
        auto endpoints = resolver.resolve(url_info.host, url_info.port);
        
        asio::ip::tcp::socket socket(io_context_);
        asio::connect(socket, endpoints);
        
        std::string request_str = build_request(request, url_info);
        asio::write(socket, asio::buffer(request_str));
        
        std::string response_data;
        std::array<char, 8192> buffer;
        std::error_code ec;
        
        while (true) {
            size_t len = socket.read_some(asio::buffer(buffer), ec);
            if (len > 0) {
                response_data.append(buffer.data(), len);
            }
            if (ec == asio::error::eof) {
                break;
            } else if (ec) {
                throw std::system_error(ec);
            }
        }
        
        return parse_response(response_data);
    }

    HttpResponse execute_https(const HttpRequest& request, const UrlInfo& url_info) {
        asio::ip::tcp::resolver resolver(io_context_);
        auto endpoints = resolver.resolve(url_info.host, url_info.port);
        
        asio::ssl::stream<asio::ip::tcp::socket> ssl_socket(io_context_, ssl_context_);
        asio::connect(ssl_socket.next_layer(), endpoints);
        ssl_socket.handshake(asio::ssl::stream_base::client);
        
        std::string request_str = build_request(request, url_info);
        asio::write(ssl_socket, asio::buffer(request_str));
        
        std::string response_data;
        std::array<char, 8192> buffer;
        std::error_code ec;
        
        while (true) {
            size_t len = ssl_socket.read_some(asio::buffer(buffer), ec);
            if (len > 0) {
                response_data.append(buffer.data(), len);
            }
            if (ec == asio::error::eof || ec == asio::ssl::error::stream_truncated) {
                break;
            } else if (ec) {
                throw std::system_error(ec);
            }
        }
        
        return parse_response(response_data);
    }

public:

    HttpResponse get(const std::string& url) {
        return execute(HttpRequest(HttpMethod::GET, url));
    }

    HttpResponse post(const std::string& url, const std::string& body) {
        return execute(HttpRequest(HttpMethod::POST, url).set_body(body));
    }

    HttpResponse put(const std::string& url, const std::string& body) {
        return execute(HttpRequest(HttpMethod::PUT, url).set_body(body));
    }

    HttpResponse del(const std::string& url) {
        return execute(HttpRequest(HttpMethod::DELETE, url));
    }

    HttpResponse head(const std::string& url) {
        return execute(HttpRequest(HttpMethod::HEAD, url));
    }

    HttpResponse patch(const std::string& url, const std::string& body) {
        return execute(HttpRequest(HttpMethod::PATCH, url).set_body(body));
    }

    HttpResponse options(const std::string& url) {
        return execute(HttpRequest(HttpMethod::OPTIONS, url));
    }

private:
    asio::io_context io_context_;
    asio::ssl::context ssl_context_;
};

}
