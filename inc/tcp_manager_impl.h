/********************************************************
 * Description : tcp manager implement
 * Data        : 2018-01-02 11:38:00
 * Author      : yanrk
 * Email       : yanrkchina@163.com
 * Blog        : blog.csdn.net/cxxmaker
 * Version     : 2.0
 * Copyright(C): 2018 - 2020
 ********************************************************/

#ifndef BOOST_NET_TCP_MANAGER_IMPLEMENT_H
#define BOOST_NET_TCP_MANAGER_IMPLEMENT_H


#include <string>
#include <vector>
#include <memory>
#include <boost/bind.hpp>
#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>
#include <boost/ptr_container/ptr_vector.hpp>
#include "boost_net.h"
#include "tcp_connection.h"
#include "io_context_pool.h"

namespace BoostNet { // namespace BoostNet begin

class TcpManagerImpl
{
public:
    typedef boost::asio::io_context                             io_context_type;
    typedef boost::asio::ssl::context                           ssl_context_type;
    typedef boost::asio::ip::tcp::endpoint                      endpoint_type;
    typedef boost::asio::ip::tcp::acceptor                      acceptor_type;
    typedef boost::ptr_vector<acceptor_type>                    acceptors_type;
    typedef IOServicePool                                       io_context_pool_type;
    typedef TcpSession                                          tcp_session_type;
    typedef SslSession                                          ssl_session_type;
    typedef std::shared_ptr<tcp_session_type>                   tcp_session_ptr;
    typedef std::shared_ptr<ssl_session_type>                   ssl_session_ptr;
    typedef std::shared_ptr<boost::asio::ip::tcp::resolver>     resolver_ptr;

public:
    TcpManagerImpl();
    ~TcpManagerImpl();

public:
    TcpManagerImpl(const TcpManagerImpl &) = delete;
    TcpManagerImpl(TcpManagerImpl &&) = delete;
    TcpManagerImpl & operator = (const TcpManagerImpl &) = delete;
    TcpManagerImpl & operator = (TcpManagerImpl &&) = delete;

public:
    bool init(TcpServiceBase * tcp_service, std::size_t thread_count, const char * host, unsigned short port_array[], std::size_t port_count, bool port_any_valid, const Certificate * server_certificate, const Certificate * client_certificate);
    void exit();

public:
    void get_ports(std::vector<uint16_t> & ports);

public:
    void run(bool blocking = false);

public:
    bool create_connection(const std::string & host, const std::string & service, bool sync_connect = true, const void * identity = 0, const char * bind_ip = "0.0.0.0", unsigned short bind_port = 0);
    bool create_connection(const std::string & host, unsigned short port, bool sync_connect = true, const void * identity = 0, const char * bind_ip = "0.0.0.0", unsigned short bind_port = 0);

private:
    template<class SessionType, class SessionPtr> bool sync_create_tcp_connection(const std::string & host, const std::string & service, const void * identity, const char * bind_ip, unsigned short bind_port);
    template<class SessionType, class SessionPtr> bool async_create_tcp_connection(const std::string & host, const std::string & service, const void * identity, const char * bind_ip, unsigned short bind_port);

private:
    void start_accept(acceptor_type & acceptor, unsigned short port);

private:
    template<class SessionType, class SessionPtr> void handle_accept(acceptor_type & acceptor, unsigned short port, SessionPtr session, const boost::system::error_code & error);

private:
    bool set_server_certificate(const Certificate * certificate);
    bool set_client_certificate(const Certificate * certificate);

private:
    const std::string & get_server_ssl_password() const;
    const std::string & get_client_ssl_password() const;

private:
    io_context_pool_type                            m_io_context_pool;
    acceptors_type                                  m_acceptors;
    ssl_context_type                                m_server_ssl_context;
    ssl_context_type                                m_client_ssl_context;
    std::string                                     m_server_ssl_password;
    std::string                                     m_client_ssl_password;
    bool                                            m_server_ssl_enable;
    bool                                            m_client_ssl_enable;
    TcpServiceBase                                * m_tcp_service;
    std::vector<uint16_t>                           m_tcp_ports;
};

template<class SessionType, class SessionPtr>
bool TcpManagerImpl::sync_create_tcp_connection(const std::string & host, const std::string & service, const void * identity, const char * bind_ip, unsigned short bind_port)
{
    boost::asio::ip::tcp::endpoint endpoint;
    if (nullptr == bind_ip || '\0' == *bind_ip)
    {
        endpoint = boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::v4(), bind_port);
    }
    else
    {
        endpoint = boost::asio::ip::tcp::endpoint(boost::asio::ip::address_v4::from_string(bind_ip), bind_port);
    }

    bool passive = false;
    SessionPtr session = boost::factory<SessionPtr>()(m_io_context_pool.get(), m_client_ssl_context, m_tcp_service, passive, identity);
    typename SessionType::lowest_type & socket = session->socket_lowest();

    boost::asio::ip::tcp::resolver resolver(session->io_context());
    boost::asio::ip::tcp::resolver::query query(host, service);
    boost::asio::ip::tcp::resolver::iterator iter_end;
    boost::system::error_code error = boost::asio::error::host_not_found;
    for (boost::asio::ip::tcp::resolver::iterator iter = resolver.resolve(query); error && iter_end != iter; ++iter)
    {
        socket.close(error);
        if (0 != endpoint.port() || !endpoint.address().is_unspecified())
        {
            socket.open(endpoint.protocol(), error);
            if (error)
            {
                return (false);
            }
            socket.set_option(boost::asio::ip::tcp::socket::reuse_address(true), error);
            socket.set_option(boost::asio::ip::tcp::socket::keep_alive(true), error);
            if (error)
            {
                return (false);
            }
            socket.bind(endpoint, error);
            if (error)
            {
                return (false);
            }
        }
        socket.connect(*iter, error);
    }

    if (error)
    {
        return (false);
    }

    session->io_context().post(boost::bind(&SessionType::start, session));

    return (true);
}

template<class SessionType, class SessionPtr>
bool TcpManagerImpl::async_create_tcp_connection(const std::string & host, const std::string & service, const void * identity, const char * bind_ip, unsigned short bind_port)
{
    boost::asio::ip::tcp::endpoint endpoint;
    if (nullptr == bind_ip || '\0' == *bind_ip)
    {
        endpoint = boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::v4(), bind_port);
    }
    else
    {
        endpoint = boost::asio::ip::tcp::endpoint(boost::asio::ip::address_v4::from_string(bind_ip), bind_port);
    }

    bool passive = false;
    SessionPtr session = boost::factory<SessionPtr>()(m_io_context_pool.get(), m_client_ssl_context, m_tcp_service, passive, identity);

    boost::asio::ip::tcp::resolver::query query(host, service);
    resolver_ptr resolver = boost::factory<resolver_ptr>()(session->io_context());
    resolver->async_resolve(query, boost::bind(&SessionType::handle_resolve, session, boost::asio::placeholders::error, boost::asio::placeholders::iterator, endpoint, resolver));

    return (true);
}

template<class SessionType, class SessionPtr>
void TcpManagerImpl::handle_accept(acceptor_type & acceptor, unsigned short port, SessionPtr session, const boost::system::error_code & error)
{
    start_accept(acceptor, port);

    if (error)
    {
        session->close();
        return;
    }

    boost::system::error_code ignore_error_code;
    session->socket_lowest().set_option(boost::asio::ip::tcp::socket::keep_alive(true), ignore_error_code);
    session->io_context().post(boost::bind(&SessionType::start, session));
}

} // namespace BoostNet end


#endif // BOOST_NET_TCP_MANAGER_IMPLEMENT_H
