#include <boost/asio.hpp>

#include "Server.h"
#include "Logger.h"

#include <iostream>
#include <memory>
#include <utility>
#include <boost/program_options.hpp>

#include <deque>

#include "Database.h"

using boost::asio::ip::tcp;
using namespace std::chrono_literals;

#include "Session.h"
#include <boost/thread.hpp>

extern std::mutex mutex;

void Server::HandleAccept(const boost::system::error_code& error, SharedSession session)
{
    std::scoped_lock lock(mutex);
    if(acceptor)
    {
        if(!error)
        {
            session->StartAsync();
            StartAccept();
        }
    }
}

void Server::StartAccept()
{
    SharedSession session(new Session(io_service));
    acceptor->async_accept(session->sessionSocket, boost::bind(&Server::HandleAccept, this, boost::asio::placeholders::error, session));
}

void Server::StartAsync()
{
    unsigned short port = acceptor->local_endpoint().port();
    io_service.reset();
    boost::system::error_code error;
    io_service.run(error);
}

void Server::StopAsync()
{
    if(acceptor)
    {
        acceptor->close();
        acceptor.reset();
        
        for(std::set<SharedSession>::iterator c = sessions.begin(); c != sessions.end(); ++c)
        {
            (*c)->StopAsync();
        }
        sessions.clear();
    }
    io_service.stop();
}

bool Server::CreateAcceptor(unsigned short port)
{
    boost::system::error_code error;
    boost::asio::ip::tcp::endpoint endpoint(boost::asio::ip::tcp::v4(), port);
    acceptor = SharedAcceptor(new boost::asio::ip::tcp::acceptor(io_service));
    acceptor->open(endpoint.protocol(), error);
    if(error)
    {
        acceptor.reset();
        return false;
    }
    
    acceptor->set_option(boost::asio::ip::tcp::acceptor::reuse_address(true), error);
    if(error)
    {
        acceptor.reset();
        return false;
    }
    acceptor->bind(endpoint, error);
    if(error)
    {
        acceptor.reset();
        return false;
    }
    acceptor->listen(boost::asio::socket_base::max_connections, error);
    if(error)
    {
        acceptor.reset();
        return false;
    }
    StartAccept();
    return true;
}

void Server::Init(void)
{
    std::scoped_lock lock(mutex);
    if(!CreateAcceptor(tcp_port))
    {
        DBG("createAcceptor fail!");
    }
    t = new std::thread(&Server::StartAsync, this);
}

void Server::BroadcastMessage(const std::string& msg)
{
    for(auto& i : sessions)
    {
        i->SendAsync(msg);
    }
}