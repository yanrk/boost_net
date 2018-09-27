/********************************************************
 * Description : tcp connection
 * Data        : 2018-01-07 18:18:00
 * Author      : yanrk
 * Email       : yanrkchina@163.com
 * Blog        : blog.csdn.net/cxxmaker
 * Version     : 2.0
 * Copyright(C): 2018
 ********************************************************/

#include <boost/bind.hpp>
#include "boost_net.h"
#include "tcp_connection.h"

namespace BoostNet { // namespace BoostNet begin

TcpConnectionBase::TcpConnectionBase()
    : m_user_data(nullptr)
{

}

TcpConnectionBase::~TcpConnectionBase()
{

}

void TcpConnectionBase::set_user_data(void * user_data)
{
    m_user_data = user_data;
}

void * TcpConnectionBase::get_user_data()
{
    return (m_user_data);
}

TcpConnection::TcpConnection(io_context_type & io_context, TcpServiceBase * tcp_service, bool passtive, std::size_t identity)
    : m_tcp_service(tcp_service)
    , m_running(false)
    , m_passtive(passtive)
    , m_identity(identity)
    , m_socket(io_context)
    , m_host_ip()
    , m_host_port(0)
    , m_peer_ip()
    , m_peer_port(0)
    , m_recv_buffer()
    , m_send_buffer()
    , m_recv_water_mark(1)
{

}

TcpConnection::~TcpConnection()
{

}

TcpConnection::socket_type & TcpConnection::socket()
{
    return (m_socket);
}

TcpConnection::io_context_type & TcpConnection::io_context()
{
    return (m_socket.get_io_context());
}

TcpConnection::tcp_recv_buffer_type & TcpConnection::recv_buffer()
{
    return (m_recv_buffer);
}

TcpConnection::tcp_send_buffer_type & TcpConnection::send_buffer()
{
    return (m_send_buffer);
}

void TcpConnection::start()
{
    boost::system::error_code ignore_error_code;
    m_host_ip = m_socket.local_endpoint(ignore_error_code).address().to_string(ignore_error_code);
    m_host_port = m_socket.local_endpoint(ignore_error_code).port();
    m_peer_ip = m_socket.remote_endpoint(ignore_error_code).address().to_string(ignore_error_code);
    m_peer_port = m_socket.remote_endpoint(ignore_error_code).port();

    m_running = true;

    if (m_passtive)
    {
        if (nullptr != m_tcp_service)
        {
            if (!m_tcp_service->on_accept(shared_from_this(), static_cast<unsigned short>(m_identity)))
            {
                close();
                return;
            }
        }
    }
    else
    {
        if (nullptr != m_tcp_service)
        {
            if (!m_tcp_service->on_connect(shared_from_this(), m_identity))
            {
                close();
                return;
            }
        }
    }

    recv();
}

void TcpConnection::stop()
{
    if (m_running)
    {
        if (nullptr != m_tcp_service)
        {
            m_tcp_service->on_close(shared_from_this());
        }
        m_running = false;
    }
}

void TcpConnection::handle_resolve(const boost::system::error_code & error, boost::asio::ip::tcp::resolver::iterator iterator, boost::asio::ip::tcp::endpoint host_endpoint, resolver_ptr resolver)
{
    boost::asio::ip::tcp::resolver::iterator iter_end;
    if (iter_end != iterator)
    {
        boost::system::error_code connect_error_code = boost::asio::error::host_not_found;
        m_socket.close(connect_error_code);
        if (0 != host_endpoint.port() || !host_endpoint.address().is_unspecified())
        {
            m_socket.open(host_endpoint.protocol(), connect_error_code);
            if (connect_error_code)
            {
                return;
            }
            m_socket.set_option(boost::asio::ip::tcp::socket::reuse_address(true), connect_error_code);
            if (connect_error_code)
            {
                return;
            }
            m_socket.bind(host_endpoint, connect_error_code);
            if (connect_error_code)
            {
                return;
            }
        }
        boost::asio::ip::tcp::endpoint peer_endpoint(*iterator++);
        m_socket.async_connect(peer_endpoint, boost::bind(&TcpConnection::handle_connect, shared_from_this(), boost::asio::placeholders::error, iterator, host_endpoint, resolver));
    }
}

void TcpConnection::handle_connect(const boost::system::error_code & error, boost::asio::ip::tcp::resolver::iterator iterator, boost::asio::ip::tcp::endpoint host_endpoint, resolver_ptr resolver)
{
    if (error)
    {
        handle_resolve(error, iterator, host_endpoint, resolver);
    }
    else
    {
        io_context().post(boost::bind(&TcpConnection::start, shared_from_this()));
    }
}

bool TcpConnection::send(const void * data, std::size_t len)
{
    return (send_buffer_fill_len(data, len));
}

void TcpConnection::close()
{
    boost::system::error_code ignore_error_code;
    m_socket.shutdown(socket_type::shutdown_both, ignore_error_code);
    m_socket.close(ignore_error_code);
    m_socket.get_io_context().post(boost::bind(&TcpConnection::stop, shared_from_this()));
}

void TcpConnection::recv()
{
    m_socket.async_read_some(m_recv_buffer.prepare(), boost::bind(&TcpConnection::handle_recv, shared_from_this(), boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred));
}

void TcpConnection::send()
{
    boost::asio::async_write(m_socket, boost::asio::buffer(m_send_buffer.data()), boost::bind(&TcpConnection::handle_send, shared_from_this(), boost::asio::placeholders::error));
}

void TcpConnection::post_send_data(const void * data, std::size_t len)
{
    io_context().post(boost::bind(&TcpConnection::push_send_data, shared_from_this(), std::vector<char>(reinterpret_cast<const char *>(data), reinterpret_cast<const char *>(data) + len)));
}

void TcpConnection::push_send_data(std::vector<char> data)
{
    bool need_send = m_send_buffer.empty();
    m_send_buffer.commit(std::move(data));
    if (need_send)
    {
        send();
    }
}

void TcpConnection::handle_recv(const boost::system::error_code & error, std::size_t bytes_transferred)
{
    if (error)
    {
        close();
        return;
    }

    m_recv_buffer.commit(bytes_transferred);

    if (nullptr != m_tcp_service)
    {
        if (m_recv_buffer.size() >= m_recv_water_mark)
        {
            if (!m_tcp_service->on_recv(shared_from_this()))
            {
                close();
                return;
            }
        }
    }
    else
    {
        m_recv_buffer.consume(bytes_transferred);
    }

    recv();
}

void TcpConnection::handle_send(const boost::system::error_code & error)
{
    if (error)
    {
        close();
        return;
    }

    m_send_buffer.consume();

    if (m_send_buffer.empty())
    {
        if (nullptr != m_tcp_service)
        {
            if (!m_tcp_service->on_send(shared_from_this()))
            {
                close();
                return;
            }
        }
    }
    else
    {
        send();
    }
}

void TcpConnection::get_host_address(std::string & ip, unsigned short & port)
{
    ip = m_host_ip;
    port = m_host_port;
}

void TcpConnection::get_peer_address(std::string & ip, unsigned short & port)
{
    ip = m_peer_ip;
    port = m_peer_port;
}

const void * TcpConnection::recv_buffer_data()
{
    return (reinterpret_cast<const void *>(m_recv_buffer.c_str()));
}

std::size_t TcpConnection::recv_buffer_size()
{
    return (m_recv_buffer.size());
}

bool TcpConnection::recv_buffer_copy_len(void * buf, std::size_t len)
{
    if (nullptr == buf || m_recv_buffer.size() < len)
    {
        return (false);
    }
    memcpy(buf, m_recv_buffer.c_str(), len);
    return (true);
}

bool TcpConnection::recv_buffer_move_len(void * buf, std::size_t len)
{
    if (nullptr == buf || m_recv_buffer.size() < len)
    {
        return (false);
    }
    memcpy(buf, m_recv_buffer.c_str(), len);
    m_recv_buffer.consume(len);
    return (true);
}

bool TcpConnection::recv_buffer_drop_len(std::size_t len)
{
    if (m_recv_buffer.size() < len)
    {
        return (false);
    }
    m_recv_buffer.consume(len);
    return (true);
}

void TcpConnection::recv_buffer_water_mark(std::size_t len)
{
    m_recv_water_mark = len;
}

bool TcpConnection::send_buffer_fill_len(const void * data, std::size_t len)
{
    if (nullptr == data)
    {
        return (0 == len);
    }
    if (0 != len)
    {
        post_send_data(data, len);
    }
    return (true);
}

} // namespace BoostNet end
