#pragma once

#include "utils/CSingleton.hpp"

#include "Settings.hpp"
#include <boost/asio.hpp>

#include <inttypes.h>
#include <map>
#include <string>
#include <deque>

#include "Session.hpp"
#include <thread>

typedef std::shared_ptr<boost::asio::ip::tcp::acceptor> SharedAcceptor;
typedef std::shared_ptr<Session> SharedSession;

class Server : public CSingleton < Server >
{
    friend class CSingleton < Server >;
    friend class Session;

public:
    Server();
    ~Server();

    // !\brief Initialize TCP backend server for sensors
    void Init();

    // !\brief Start async operations
    void StartAsync(std::stop_token token);

    // !\brief Broadcast message to every session
    // !\param msg [in] TCP Server port
    void BroadcastMessage(const std::string& msg);
    
    // !\brief Set Forward TCP Server IP & Port
    void SetForwardIpAddress(const std::string& ip);    
    
    // !\brief Set Forward 2 TCP Server IP & Port
    void SetForwardIpAddress2(const std::string& ip);

    // !\brief Get Forward TCP Server IP & Port
    const std::string GetForwardIpAddress();    
    
    // !\brief Get Forward TCP Server 2 IP & Port
    const std::string GetForwardIpAddress2();

    // !\brief Is TCP backend server for sensors enabled?
    bool is_enabled = true;

    // !\brief Is server setup correctly and run without errors?
    bool is_ok = false;

    // !\brief TCP Server port
    uint16_t tcp_port = 2005;

    std::string forward_ip_address = "null";

    uint32_t forward_port = 0;    
    
    std::string forward_ip_address2 = "null";

    uint32_t forward_port2 = 0;

    // !\brief IP addresses used by connected sensor(s)
    std::set<uint32_t> used_ip_addresses;

private:
    
    // !\brief Create TCP acceptor
    // !\param port [in] TCP Server port
    bool CreateAcceptor(unsigned short port);

    // !\brief Stop async operations
    void StopAsync();

    // !\brief Start async accept
    void StartAccept();

    // !\brief Handle async accept
    // !\param error [in] Boost error code
    // !\param session [in] Shared pointer to current session
    void HandleAccept(const boost::system::error_code& error, SharedSession session);

    // !\brief Set of active sessions
    std::set<SharedSession> sessions;

    // !\brief Worker thread
    std::unique_ptr<std::jthread> m_worker = nullptr;

    // !\brief ASIO Acceptor
    SharedAcceptor acceptor;

    // !\brief Mutex for IO operations
    std::mutex m_IoMutex;

    // !\brief IO Service
    boost::asio::io_service io_service;
};