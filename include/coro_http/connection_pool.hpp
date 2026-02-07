#pragma once

#include <asio.hpp>
#include <asio/ssl.hpp>
#include <memory>
#include <map>
#include <deque>
#include <mutex>
#include <chrono>

namespace coro_http {

struct PooledConnection {
    std::shared_ptr<asio::ip::tcp::socket> socket;
    std::chrono::steady_clock::time_point last_used;
    bool in_use{false};
    
    PooledConnection(std::shared_ptr<asio::ip::tcp::socket> sock)
        : socket(sock), last_used(std::chrono::steady_clock::now()) {}
};

struct PooledSSLConnection {
    std::shared_ptr<asio::ssl::stream<asio::ip::tcp::socket>> ssl_stream;
    std::chrono::steady_clock::time_point last_used;
    bool in_use{false};
    
    PooledSSLConnection(std::shared_ptr<asio::ssl::stream<asio::ip::tcp::socket>> stream)
        : ssl_stream(stream), last_used(std::chrono::steady_clock::now()) {}
};

class ConnectionPool {
public:
    ConnectionPool(int max_per_host, std::chrono::seconds idle_timeout)
        : max_connections_per_host_(max_per_host),
          idle_timeout_(idle_timeout) {}
    
    // Get or create HTTP connection
    std::shared_ptr<asio::ip::tcp::socket> get_connection(
        asio::io_context& io_context,
        const std::string& host,
        const std::string& port) {
        
        std::lock_guard<std::mutex> lock(mutex_);
        
        std::string key = host + ":" + port;
        auto& connections = http_pool_[key];
        
        // Find available connection
        auto now = std::chrono::steady_clock::now();
        for (auto it = connections.begin(); it != connections.end(); ) {
            // Remove timed out connections
            if (now - it->last_used > idle_timeout_) {
                it = connections.erase(it);
                continue;
            }
            
            // Check if connection is available and valid
            if (!it->in_use && is_socket_valid(it->socket)) {
                it->in_use = true;
                it->last_used = now;
                return it->socket;
            }
            
            // Remove invalid connections
            if (!is_socket_valid(it->socket)) {
                it = connections.erase(it);
                continue;
            }
            
            ++it;
        }
        
        // Create new connection if under limit
        if (static_cast<int>(connections.size()) < max_connections_per_host_) {
            auto socket = std::make_shared<asio::ip::tcp::socket>(io_context);
            connections.emplace_back(socket);
            connections.back().in_use = true;
            return socket;
        }
        
        // Pool is full, create temporary connection
        return std::make_shared<asio::ip::tcp::socket>(io_context);
    }
    
    // Get or create HTTPS connection
    std::shared_ptr<asio::ssl::stream<asio::ip::tcp::socket>> get_ssl_connection(
        asio::io_context& io_context,
        asio::ssl::context& ssl_context,
        const std::string& host,
        const std::string& port) {
        
        std::lock_guard<std::mutex> lock(mutex_);
        
        std::string key = host + ":" + port;
        auto& connections = ssl_pool_[key];
        
        // Find available connection
        auto now = std::chrono::steady_clock::now();
        for (auto it = connections.begin(); it != connections.end(); ) {
            // Remove timed out connections
            if (now - it->last_used > idle_timeout_) {
                it = connections.erase(it);
                continue;
            }
            
            // Check if connection is available and valid
            if (!it->in_use && is_ssl_socket_valid(it->ssl_stream)) {
                it->in_use = true;
                it->last_used = now;
                return it->ssl_stream;
            }
            
            // Remove invalid connections
            if (!is_ssl_socket_valid(it->ssl_stream)) {
                it = connections.erase(it);
                continue;
            }
            
            ++it;
        }
        
        // Create new connection if under limit
        if (static_cast<int>(connections.size()) < max_connections_per_host_) {
            auto ssl_stream = std::make_shared<asio::ssl::stream<asio::ip::tcp::socket>>(
                io_context, ssl_context);
            connections.emplace_back(ssl_stream);
            connections.back().in_use = true;
            return ssl_stream;
        }
        
        // Pool is full, create temporary connection
        return std::make_shared<asio::ssl::stream<asio::ip::tcp::socket>>(
            io_context, ssl_context);
    }
    
    // Release HTTP connection back to pool
    // Returns true if connection was released, false if it should be closed
    void release_connection(
        const std::shared_ptr<asio::ip::tcp::socket>& socket,
        const std::string& host,
        const std::string& port,
        bool should_keep_alive = true) {
        
        std::lock_guard<std::mutex> lock(mutex_);
        
        std::string key = host + ":" + port;
        auto& connections = http_pool_[key];
        
        // If server sent Connection: close, remove from pool
        if (!should_keep_alive) {
            for (auto it = connections.begin(); it != connections.end(); ++it) {
                if (it->socket == socket) {
                    connections.erase(it);
                    return;
                }
            }
            return;
        }
        
        // Normal release - mark as available
        for (auto& conn : connections) {
            if (conn.socket == socket) {
                conn.in_use = false;
                conn.last_used = std::chrono::steady_clock::now();
                return;
            }
        }
    }
    
    // Release HTTPS connection back to pool
    // Returns true if connection was released, false if it should be closed
    void release_ssl_connection(
        const std::shared_ptr<asio::ssl::stream<asio::ip::tcp::socket>>& ssl_stream,
        const std::string& host,
        const std::string& port,
        bool should_keep_alive = true) {
        
        std::lock_guard<std::mutex> lock(mutex_);
        
        std::string key = host + ":" + port;
        auto& connections = ssl_pool_[key];
        
        // If server sent Connection: close, remove from pool
        if (!should_keep_alive) {
            for (auto it = connections.begin(); it != connections.end(); ++it) {
                if (it->ssl_stream == ssl_stream) {
                    connections.erase(it);
                    return;
                }
            }
            return;
        }
        
        // Normal release - mark as available
        for (auto& conn : connections) {
            if (conn.ssl_stream == ssl_stream) {
                conn.in_use = false;
                conn.last_used = std::chrono::steady_clock::now();
                return;
            }
        }
    }
    
    // Clear all connections
    void clear() {
        std::lock_guard<std::mutex> lock(mutex_);
        http_pool_.clear();
        ssl_pool_.clear();
    }
    
    // Get pool statistics
    struct Stats {
        int total_http_connections{0};
        int active_http_connections{0};
        int total_ssl_connections{0};
        int active_ssl_connections{0};
    };
    
    Stats get_stats() const {
        std::lock_guard<std::mutex> lock(mutex_);
        Stats stats;
        
        for (const auto& [key, connections] : http_pool_) {
            stats.total_http_connections += connections.size();
            for (const auto& conn : connections) {
                if (conn.in_use) {
                    stats.active_http_connections++;
                }
            }
        }
        
        for (const auto& [key, connections] : ssl_pool_) {
            stats.total_ssl_connections += connections.size();
            for (const auto& conn : connections) {
                if (conn.in_use) {
                    stats.active_ssl_connections++;
                }
            }
        }
        
        return stats;
    }

private:
    bool is_socket_valid(const std::shared_ptr<asio::ip::tcp::socket>& socket) {
        if (!socket || !socket->is_open()) {
            return false;
        }
        
        // Check if remote endpoint is still valid
        asio::error_code ec;
        socket->lowest_layer().remote_endpoint(ec);
        if (ec) {
            return false;
        }
        
        // Detect server-initiated close by attempting non-blocking read
        // If server closed the connection, we'll get immediate EOF or error
        socket->non_blocking(true, ec);
        if (ec) {
            return false;
        }
        
        char test_byte;
        size_t bytes_read = socket->read_some(asio::buffer(&test_byte, 1), ec);
        
        // Restore blocking mode
        socket->non_blocking(false);
        
        // If we got data, this is unexpected (server shouldn't send unsolicited data)
        // If we got EOF, connection was closed
        // If we got would_block, connection is still alive
        if (ec == asio::error::would_block) {
            return true;  // Connection is alive
        }
        
        return false;  // Connection closed or error
    }
    
    bool is_ssl_socket_valid(
        const std::shared_ptr<asio::ssl::stream<asio::ip::tcp::socket>>& ssl_stream) {
        if (!ssl_stream) {
            return false;
        }
        
        auto& socket = ssl_stream->lowest_layer();
        if (!socket.is_open()) {
            return false;
        }
        
        // Check if remote endpoint is still valid
        asio::error_code ec;
        socket.remote_endpoint(ec);
        if (ec) {
            return false;
        }
        
        // Verify SSL session state
        SSL* ssl = ssl_stream->native_handle();
        if (!ssl) {
            return false;
        }
        
        // Check if SSL handshake is complete and session is valid
        if (!SSL_is_init_finished(ssl)) {
            return false;
        }
        
        // Check if session is still active
        SSL_SESSION* session = SSL_get_session(ssl);
        if (!session) {
            return false;
        }
        
        // Detect server-initiated close by attempting non-blocking read
        socket.non_blocking(true, ec);
        if (ec) {
            return false;
        }
        
        char test_byte;
        // For SSL, we need to try reading through the SSL layer
        asio::error_code read_ec;
        size_t bytes_read = ssl_stream->read_some(asio::buffer(&test_byte, 1), read_ec);
        
        // Restore blocking mode
        socket.non_blocking(false);
        
        // Check read result
        // For SSL, would_block indicates connection is alive
        if (read_ec == asio::error::would_block) {
            return true;  // Connection is alive
        }
        
        return false;  // Connection closed or error
    }
    
    int max_connections_per_host_;
    std::chrono::seconds idle_timeout_;
    std::map<std::string, std::deque<PooledConnection>> http_pool_;
    std::map<std::string, std::deque<PooledSSLConnection>> ssl_pool_;
    mutable std::mutex mutex_;
};

}
