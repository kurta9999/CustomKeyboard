#include "pch.hpp"

Server::Server()
{

}

Server::~Server()
{
    StopAsync();
    if(m_worker)
        m_worker.reset(nullptr);
}

void Server::Init(void)
{
    if(is_enabled)
    {
        std::scoped_lock lock(m_IoMutex);
        if(!CreateAcceptor(tcp_port))
        {
            LOG(LogLevel::Error, "createAcceptor fail!");
            return;
        }
        m_worker = std::make_unique<std::jthread>(std::bind_front(&Server::StartAsync, this));
        if(m_worker)
            utils::SetThreadName(*m_worker, "Server");
        DatabaseLogic::Get()->GenerateGraphs();
    }
}

void Server::StartAsync(std::stop_token token)
{
    unsigned short port = acceptor->local_endpoint().port();
    io_service.reset();
    boost::system::error_code error;
    io_service.run(error);
    LOG(LogLevel::Verbose, "tcp backend io_service finish");
}

void Server::BroadcastMessage(const std::string& msg)
{
    if(is_enabled)
    {
        std::scoped_lock lock(m_IoMutex);
        for(auto& i : sessions)
        {
            i->SendAsync(msg);
        }
    }
}

void Server::SetForwardIpAddress(const std::string& ip)
{
    if(ip != "null")
    {
        size_t pos = ip.find(':');
        if(pos != std::string::npos)
        {
			forward_ip_address = ip.substr(0, pos);
            try
            {
				forward_port = std::stoi(ip.substr(pos + 1));
			}
            catch(const std::exception& e)
            {
                LOG(LogLevel::Error, "stoi exception: {}", e.what());
            }
		}
    }
}

void Server::SetForwardIpAddress2(const std::string& ip)
{
    if(ip != "null")
    {
        size_t pos = ip.find(':');
        if(pos != std::string::npos)
        {
			forward_ip_address2 = ip.substr(0, pos);
            try
            {
				forward_port2 = std::stoi(ip.substr(pos + 1));
			}
            catch(const std::exception& e)
            {
                LOG(LogLevel::Error, "stoi exception: {}", e.what());
            }
		}
    }
}

const std::string Server::GetForwardIpAddress()
{
    if(forward_ip_address == "null")
		return forward_ip_address;

    return forward_ip_address + ":" + std::to_string(forward_port);
}

const std::string Server::GetForwardIpAddress2()
{
    if(forward_ip_address2 == "null")
		return forward_ip_address2;

    return forward_ip_address2 + ":" + std::to_string(forward_port2);
}

bool Server::CreateAcceptor(unsigned short port)
{
    boost::system::error_code error;
    boost::asio::ip::tcp::endpoint endpoint(boost::asio::ip::tcp::v4(), port);
    acceptor = std::make_shared<boost::asio::ip::tcp::acceptor>(io_service);
    acceptor->open(endpoint.protocol(), error);
    if(error)
    {
        LOG(LogLevel::Error, "open error {} - {}", port, error.message());
        acceptor.reset();
        return false;
    }

    acceptor->set_option(boost::asio::ip::tcp::acceptor::reuse_address(true), error);
    if(error)
    {
        LOG(LogLevel::Error, "reuse_address error - {}", error.message());
        acceptor.reset();
        return false;
    }
    acceptor->bind(endpoint, error);
    if(error)
    {
        LOG(LogLevel::Error, "bind error - {}", error.message());
        acceptor.reset();
        return false;
    }
    acceptor->listen(boost::asio::socket_base::max_connections, error);
    if(error)
    {
        LOG(LogLevel::Error, "listen error - {}", error.message());
        acceptor.reset();
        return false;
    }
    StartAccept();
    is_ok = true;
    return true;
}

void Server::StopAsync()
{
    if(is_enabled)
    {
        std::scoped_lock lock(m_IoMutex);
        if(acceptor)
        {
            acceptor->close();
            acceptor.reset();

            for(const auto& c : sessions)
            {
                c->StopAsync(false);
            }
            sessions.clear();
        }
        io_service.stop();
    }
}

void Server::StartAccept()
{
    SharedSession session = std::make_shared<Session>(io_service, m_IoMutex, std::make_unique<TcpMessageExecutor>());
    acceptor->async_accept(session->sessionSocket, std::bind(&Server::HandleAccept, this, std::placeholders::_1, session));
}

void Server::HandleAccept(const boost::system::error_code& error, SharedSession session)
{
    std::scoped_lock lock(m_IoMutex);
    if(acceptor)
    {
        if(!error)
        {
            session->StartAsync();
            sessions.emplace(session);
            if(Server::Get()->used_ip_addresses.emplace(session->sessionIntAddr).second)
            {
                LOG(LogLevel::Error, "New sensor connected, IP: {}:{}", session->sessionAddress, session->sessionPort);
            }
            StartAccept();
        }
    }
}
