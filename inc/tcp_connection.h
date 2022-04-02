/********************************************************
 * Description : tcp connection
 * Data        : 2018-01-07 18:18:00
 * Author      : yanrk
 * Email       : yanrkchina@163.com
 * Blog        : blog.csdn.net/cxxmaker
 * Version     : 2.0
 * Copyright(C): 2018 - 2020
 ********************************************************/

#ifndef BOOST_NET_TCP_CONNECTION_H
#define BOOST_NET_TCP_CONNECTION_H


#include <string>
#include <vector>
#include <boost/bind.hpp>
#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>
#include "boost_net.h"
#include "tcp_recv_buffer.h"
#include "tcp_send_buffer.h"

namespace BoostNet { // namespace BoostNet begin

template <class Derived, class SocketType>
class TcpConnection : public TcpConnectionBase
{
public:
    typedef boost::asio::io_context                             io_context_type;
    typedef boost::asio::ssl::context                           ssl_context_type;
    typedef TcpRecvBuffer                                       tcp_recv_buffer_type;
    typedef TcpSendBuffer                                       tcp_send_buffer_type;
    typedef std::shared_ptr<boost::asio::ip::tcp::resolver>     resolver_ptr;

public:
    TcpConnection(io_context_type & io_context, ssl_context_type & ssl_context, TcpServiceBase * tcp_service, bool passive, const void * identity, bool use_ssl);
    virtual ~TcpConnection() override;

public:
    TcpConnection(const TcpConnection &) = delete;
    TcpConnection(TcpConnection &&) = delete;
    TcpConnection & operator = (const TcpConnection &) = delete;
    TcpConnection & operator = (TcpConnection &&) = delete;

public:
    virtual void get_host_address(std::string & ip, unsigned short & port) override;
    virtual void get_peer_address(std::string & ip, unsigned short & port) override;

public:
    virtual const void * recv_buffer_data() override;
    virtual std::size_t recv_buffer_size() override;
    virtual bool recv_buffer_copy(void * buf, std::size_t len) override;
    virtual bool recv_buffer_move(void * buf, std::size_t len) override;
    virtual bool recv_buffer_drop(std::size_t len) override;
    virtual void recv_buffer_water_mark(std::size_t len) override;
    virtual bool send_buffer_fill(const void * data, std::size_t len) override;

public:
    virtual void close() override;

public:
    io_context_type & io_context();
    tcp_recv_buffer_type & recv_buffer();
    tcp_send_buffer_type & send_buffer();
    void start();

public:
    void handle_resolve(const boost::system::error_code & error, boost::asio::ip::tcp::resolver::iterator iterator, boost::asio::ip::tcp::endpoint host_endpoint, resolver_ptr resolver);
    void handle_connect(const boost::system::error_code & error, boost::asio::ip::tcp::resolver::iterator iterator, boost::asio::ip::tcp::endpoint host_endpoint, resolver_ptr resolver);
    void handle_handshake(const boost::system::error_code & error);

private:
    void send();
    void recv();
    void stop();
    void post_send_data(const void * data, std::size_t len);
    void push_send_data(std::vector<char> data);

private:
    void handle_send(const boost::system::error_code & error);
    void handle_recv(const boost::system::error_code & error, std::size_t bytes_transferred);

private:
    Derived & derived();

private:
    io_context_type                               & m_io_context;
    ssl_context_type                              & m_ssl_context;
    TcpServiceBase                                * m_tcp_service;
    const bool                                      m_use_ssl;
    bool                                            m_running;
    bool                                            m_passive;
    const void                                    * m_identity;
    std::string                                     m_host_ip;
    unsigned short                                  m_host_port;
    std::string                                     m_peer_ip;
    unsigned short                                  m_peer_port;
    tcp_recv_buffer_type                            m_recv_buffer;
    tcp_send_buffer_type                            m_send_buffer;
    std::size_t                                     m_recv_water_mark;
};

template <class Derived, class SocketType>
TcpConnection<Derived, SocketType>::TcpConnection(io_context_type & io_context, ssl_context_type & ssl_context, TcpServiceBase * tcp_service, bool passive, const void * identity, bool use_ssl)
    : m_io_context(io_context)
    , m_ssl_context(ssl_context)
    , m_tcp_service(tcp_service)
    , m_use_ssl(use_ssl)
    , m_running(false)
    , m_passive(passive)
    , m_identity(identity)
    , m_host_ip()
    , m_host_port(0)
    , m_peer_ip()
    , m_peer_port(0)
    , m_recv_buffer()
    , m_send_buffer()
    , m_recv_water_mark(1)
{

}

template <class Derived, class SocketType>
TcpConnection<Derived, SocketType>::~TcpConnection()
{

}

template <class Derived, class SocketType>
Derived & TcpConnection<Derived, SocketType>::derived()
{
    return (static_cast<Derived &>(*this));
}

template <class Derived, class SocketType>
typename TcpConnection<Derived, SocketType>::io_context_type & TcpConnection<Derived, SocketType>::io_context()
{
    return (m_io_context);
}

template <class Derived, class SocketType>
typename TcpConnection<Derived, SocketType>::tcp_recv_buffer_type & TcpConnection<Derived, SocketType>::recv_buffer()
{
    return (m_recv_buffer);
}

template <class Derived, class SocketType>
typename TcpConnection<Derived, SocketType>::tcp_send_buffer_type & TcpConnection<Derived, SocketType>::send_buffer()
{
    return (m_send_buffer);
}

template <class Derived, class SocketType>
void TcpConnection<Derived, SocketType>::start()
{
    if (m_use_ssl)
    {
        derived().handshake(m_passive);
    }
    else
    {
        handle_handshake(boost::system::error_code());
    }
}

template <class Derived, class SocketType>
void TcpConnection<Derived, SocketType>::stop()
{
    if (m_running)
    {
        if (nullptr != m_tcp_service)
        {
            m_tcp_service->on_close(derived().shared_from_this());
        }
        m_running = false;
    }
}

template <class Derived, class SocketType>
void TcpConnection<Derived, SocketType>::handle_resolve(const boost::system::error_code & error, boost::asio::ip::tcp::resolver::iterator iterator, boost::asio::ip::tcp::endpoint host_endpoint, resolver_ptr resolver)
{
    boost::asio::ip::tcp::resolver::iterator iter_end;
    if (iter_end != iterator)
    {
        boost::system::error_code connect_error_code = boost::asio::error::host_not_found;
        derived().socket_lowest().close(connect_error_code);
        if (0 != host_endpoint.port() || !host_endpoint.address().is_unspecified())
        {
            try
            {
                derived().socket_lowest().open(host_endpoint.protocol());
                derived().socket_lowest().set_option(boost::asio::ip::tcp::socket::reuse_address(true));
                derived().socket_lowest().bind(host_endpoint);
            }
            catch (boost::system::error_code &)
            {
                if (nullptr != m_tcp_service)
                {
                    m_tcp_service->on_connect(nullptr, m_identity);
                }
                return;
            }
        }
        boost::asio::ip::tcp::endpoint peer_endpoint(*iterator++);
        derived().socket_lowest().async_connect(peer_endpoint, boost::bind(&TcpConnection::handle_connect, derived().shared_from_this(), boost::asio::placeholders::error, iterator, host_endpoint, resolver));
    }
    else
    {
        if (nullptr != m_tcp_service)
        {
            m_tcp_service->on_connect(nullptr, m_identity);
        }
    }
}

template <class Derived, class SocketType>
void TcpConnection<Derived, SocketType>::handle_connect(const boost::system::error_code & error, boost::asio::ip::tcp::resolver::iterator iterator, boost::asio::ip::tcp::endpoint host_endpoint, resolver_ptr resolver)
{
    if (error)
    {
        handle_resolve(error, iterator, host_endpoint, resolver);
    }
    else
    {
        io_context().post(boost::bind(&TcpConnection::start, derived().shared_from_this()));
    }
}

template <class Derived, class SocketType>
void TcpConnection<Derived, SocketType>::handle_handshake(const boost::system::error_code & error)
{
    boost::system::error_code ignore_error_code;
    m_host_ip = derived().socket_lowest().local_endpoint(ignore_error_code).address().to_string(ignore_error_code);
    m_host_port = derived().socket_lowest().local_endpoint(ignore_error_code).port();
    m_peer_ip = derived().socket_lowest().remote_endpoint(ignore_error_code).address().to_string(ignore_error_code);
    m_peer_port = derived().socket_lowest().remote_endpoint(ignore_error_code).port();

    if (!error)
    {
        m_running = true;

        if (nullptr != m_tcp_service)
        {
            if (m_passive)
            {
                if (!m_tcp_service->on_accept(derived().shared_from_this(), static_cast<unsigned short>(reinterpret_cast<uint64_t>(m_identity))))
                {
                    close();
                    return;
                }
            }
            else
            {
                if (!m_tcp_service->on_connect(derived().shared_from_this(), m_identity))
                {
                    close();
                    return;
                }
            }
        }

        recv();
    }
}

template <class Derived, class SocketType>
void TcpConnection<Derived, SocketType>::close()
{
    derived().shutdown();
    m_io_context.post(boost::bind(&TcpConnection::stop, derived().shared_from_this()));
}

template <class Derived, class SocketType>
void TcpConnection<Derived, SocketType>::recv()
{
    derived().socket().async_read_some(m_recv_buffer.prepare(), boost::bind(&TcpConnection::handle_recv, derived().shared_from_this(), boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred));
}

template <class Derived, class SocketType>
void TcpConnection<Derived, SocketType>::send()
{
    boost::asio::async_write(derived().socket(), boost::asio::buffer(m_send_buffer.data()), boost::bind(&TcpConnection::handle_send, derived().shared_from_this(), boost::asio::placeholders::error));
}

template <class Derived, class SocketType>
void TcpConnection<Derived, SocketType>::post_send_data(const void * data, std::size_t len)
{
    io_context().post(boost::bind(&TcpConnection::push_send_data, derived().shared_from_this(), std::vector<char>(reinterpret_cast<const char *>(data), reinterpret_cast<const char *>(data) + len)));
}

template <class Derived, class SocketType>
void TcpConnection<Derived, SocketType>::push_send_data(std::vector<char> data)
{
    bool need_send = m_send_buffer.empty();
    m_send_buffer.commit(std::move(data));
    if (need_send)
    {
        send();
    }
}

template <class Derived, class SocketType>
void TcpConnection<Derived, SocketType>::handle_recv(const boost::system::error_code & error, std::size_t bytes_transferred)
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
            if (!m_tcp_service->on_recv(derived().shared_from_this()))
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

template <class Derived, class SocketType>
void TcpConnection<Derived, SocketType>::handle_send(const boost::system::error_code & error)
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
            if (!m_tcp_service->on_send(derived().shared_from_this()))
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

template <class Derived, class SocketType>
void TcpConnection<Derived, SocketType>::get_host_address(std::string & ip, unsigned short & port)
{
    ip = m_host_ip;
    port = m_host_port;
}

template <class Derived, class SocketType>
void TcpConnection<Derived, SocketType>::get_peer_address(std::string & ip, unsigned short & port)
{
    ip = m_peer_ip;
    port = m_peer_port;
}

template <class Derived, class SocketType>
const void * TcpConnection<Derived, SocketType>::recv_buffer_data()
{
    return (reinterpret_cast<const void *>(m_recv_buffer.c_str()));
}

template <class Derived, class SocketType>
std::size_t TcpConnection<Derived, SocketType>::recv_buffer_size()
{
    return (m_recv_buffer.size());
}

template <class Derived, class SocketType>
bool TcpConnection<Derived, SocketType>::recv_buffer_copy(void * buf, std::size_t len)
{
    if (nullptr == buf || m_recv_buffer.size() < len)
    {
        return (false);
    }
    memcpy(buf, m_recv_buffer.c_str(), len);
    return (true);
}

template <class Derived, class SocketType>
bool TcpConnection<Derived, SocketType>::recv_buffer_move(void * buf, std::size_t len)
{
    if (nullptr == buf || m_recv_buffer.size() < len)
    {
        return (false);
    }
    memcpy(buf, m_recv_buffer.c_str(), len);
    m_recv_buffer.consume(len);
    return (true);
}

template <class Derived, class SocketType>
bool TcpConnection<Derived, SocketType>::recv_buffer_drop(std::size_t len)
{
    if (m_recv_buffer.size() < len)
    {
        return (false);
    }
    m_recv_buffer.consume(len);
    return (true);
}

template <class Derived, class SocketType>
void TcpConnection<Derived, SocketType>::recv_buffer_water_mark(std::size_t len)
{
    m_recv_water_mark = len;
}

template <class Derived, class SocketType>
bool TcpConnection<Derived, SocketType>::send_buffer_fill(const void * data, std::size_t len)
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

class TcpSession : public TcpConnection<TcpSession, boost::asio::ip::tcp::socket>, public std::enable_shared_from_this<TcpSession>
{
public:
    typedef boost::asio::ip::tcp::socket                            socket_type;
    typedef socket_type                                             lowest_type;

public:
    TcpSession(io_context_type & io_context, ssl_context_type & ssl_context, TcpServiceBase * tcp_service, bool passive, const void * identity);

public:
    socket_type & socket();
    lowest_type & socket_lowest();
    void handshake(bool passive);
    void shutdown();

private:
    socket_type                                                     m_socket;
}; 

class SslSession : public TcpConnection<SslSession, boost::asio::ssl::stream<boost::asio::ip::tcp::socket>>, public std::enable_shared_from_this<SslSession>
{
public:
    typedef boost::asio::ssl::stream<boost::asio::ip::tcp::socket>  socket_type;
    typedef socket_type::lowest_layer_type                          lowest_type;

public:
    SslSession(io_context_type & io_context, ssl_context_type & ssl_context, TcpServiceBase * tcp_service, bool passive, const void * identity);

public:
    socket_type & socket();
    lowest_type & socket_lowest();
    void handshake(bool passive);
    void shutdown();

private:
    boost::asio::ssl::stream<boost::asio::ip::tcp::socket>          m_socket;
};

} // namespace BoostNet end


#endif // BOOST_NET_TCP_CONNECTION_H
