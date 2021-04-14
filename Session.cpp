#include "Session.h"
#include "Sensors.h"

#include <boost/asio.hpp>

#include <boost/algorithm/string.hpp>
#include <boost/array.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/format.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/scoped_ptr.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/thread.hpp>
#include <boost/unordered_map.hpp>

#include <fstream>
#include <map>
#include <queue>
#include <set>
#include <string>
#include <vector>

std::mutex mutex;

#include "Logger.h"
#include "Sensors.h"
#include "Server.h"

Session::Session(boost::asio::io_service& io_service) : heartbeatTimer(io_service), sessionSocket(io_service)
{

}

void Session::HandleRead(const boost::system::error_code& error, std::size_t bytesTransferred)
{
	std::scoped_lock lock(mutex);
	if(!error)
	{
		receivedData[bytesTransferred] = 0;
		Sensors::Get()->ProcessIncommingData(receivedData, sessionSocket.remote_endpoint().address().to_string().c_str());
		sessionSocket.async_read_some(boost::asio::buffer(receivedData), 
			boost::bind(&Session::HandleRead, shared_from_this(), boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred));
		SendAsync(receivedData);
		DBG(receivedData);
		DBG("\n");
	}
}

void Session::HandleTransferTimer(const boost::system::error_code& error)
{
	std::scoped_lock lock(mutex);
	if(!error)
	{
		if(!pendingMessages.empty())
		{
			SendAsync(pendingMessages.front());
			pendingMessages.pop();
		}
	}
}

void Session::HandleWrite(const boost::system::error_code& error)
{
	writeInProgress = false;
	if(!error)
	{
		if(!pendingMessages.empty())
		{
			SendAsync(pendingMessages.front());
			pendingMessages.pop();
		}
	}
	else
	{
		pendingMessages = std::queue<std::string>();
	}
}

void Session::SendAsync(const std::string& buffer)
{
	if(writeInProgress)
	{
		pendingMessages.push(buffer);
	}
	else
	{
		sentData = buffer;
		writeInProgress = true;
		boost::asio::async_write(sessionSocket, boost::asio::buffer(sentData, sentData.length()), 
			boost::bind(&Session::HandleWrite, shared_from_this(), boost::asio::placeholders::error));
	}
}

void Session::StartAsync()
{
	boost::system::error_code error;
	boost::asio::ip::tcp::endpoint remoteEndpoint = sessionSocket.remote_endpoint(error);
	if(error)
	{
		StopAsync();
		return;
	}
	/* for some reason the debugger freeze here.. I have no ide why */
	//sessionAddress = remoteEndpoint.address().to_string(); 
	//sessionPort = remoteEndpoint.port();
	/*
	for(std::set<SharedSession>::iterator c = core->getServer()->sessions.begin(); c != core->getServer()->sessions.end(); ++c)
	{
		if(boost::algorithm::equals((*c)->sessionAddress, sessionAddress))
		{
			stopAsync();
			return;
		}
	}
	*/
	
	sessionSocket.async_read_some(boost::asio::buffer(receivedData, sizeof(receivedData)), 
		boost::bind(&Session::HandleRead, shared_from_this(), boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred));
	Server::Get()->sessions.insert(shared_from_this());
}

void Session::StopAsync()
{
	if(sessionSocket.is_open())
	{
		boost::system::error_code error;
		sessionSocket.shutdown(boost::asio::ip::tcp::socket::shutdown_both, error);
		sessionSocket.close(error);
	}
}