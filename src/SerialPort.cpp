#include "pch.hpp"

using namespace std::chrono_literals;

SerialPort::~SerialPort()
{
    DestroyWorkingThread();
}

void SerialPort::Init()
{
    if(is_enabled)
    {
        if(!m_worker)
            m_worker = std::make_unique<std::thread>(&SerialPort::UartReceiveThread, this, std::ref(to_exit), std::ref(m_cv), std::ref(m_mutex));
    }
    else
    {
        DestroyWorkingThread();
    }
}

void SerialPort::DestroyWorkingThread()
{
    if(m_worker)
    {
        {
            std::lock_guard guard(m_mutex);
            to_exit = true;
            m_cv.notify_all();
        }
        if(m_worker->joinable())
            m_worker->join();
    }
}

void SerialPort::OnUartDataReceived(const char* data, unsigned int len)
{
    if(forward_serial_to_tcp)
    {
        SerialForwarder::Get()->Send(remote_tcp_ip, remote_tcp_port, data, len);
    }
    else
    {
        CustomMacro::Get()->ProcessReceivedData(data, len);
    }
}

// LUbuntu [Running] - Oracle VM VirtualBox
// \x00\x00\x00\x00\x00\x00\x00\x00\x00\x54\x00\x00\x00\x00\x00\x4C\x45
// \h(00 00 00 00 00 00 00 00 00 54 00 00 00 00 00 4C 45)
void SerialPort::UartReceiveThread(std::atomic<bool>& to_exit, std::condition_variable& cv, std::mutex& m)
{
    while(!to_exit)
    {
        try
        {
            CallbackAsyncSerial serial("\\\\.\\COM" + std::to_string(com_port), 921600); /* baud rate has no meaning here */
            serial.setCallback(std::bind(&SerialPort::OnUartDataReceived, this, std::placeholders::_1, std::placeholders::_2));

            while(!to_exit)
            {
                if(serial.errorStatus() || serial.isOpen() == false)
                {
                    LOGMSG(error, "Serial port unexpectedly closed");
                    break;
                }
                {
                    std::unique_lock lock(m_mutex);
                    m_cv.wait_for(lock, 1000ms);
                }
            }
            serial.close();
        }
        catch(std::exception& e)
        {
            LOGMSG(error, "Exception serial {}", e.what());
            {
                std::unique_lock lock(m_mutex);
                m_cv.wait_for(lock, 1000ms);
            }
        }
    }
}

void SerialPort::SetEnabled(bool enable)
{
    is_enabled = enable;
}

bool SerialPort::IsEnabled()
{
    return is_enabled;
}

void SerialPort::SetComPort(uint16_t port)
{
    com_port = port;
}

uint16_t SerialPort::GetComPort()
{
    return com_port;
}

void SerialPort::SetForwardToTcp(bool enable)
{
    forward_serial_to_tcp = enable;
}

bool SerialPort::IsForwardToTcp()
{
    return forward_serial_to_tcp;
}

void SerialPort::SetRemoteTcpIp(std::string& ip)
{
    remote_tcp_ip = ip;
}

std::string& SerialPort::GetRemoteTcpIp()
{
    return remote_tcp_ip;
}

void SerialPort::SetRemoteTcpPort(uint16_t remote_port)
{
    remote_tcp_port = remote_port;
}

uint16_t SerialPort::GetRemoteTcpPort()
{
    return remote_tcp_port;
}