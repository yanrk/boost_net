/********************************************************
 * Description : tcp manager implement
 * Data        : 2018-01-02 11:38:00
 * Author      : yanrk
 * Email       : yanrkchina@163.com
 * Blog        : blog.csdn.net/cxxmaker
 * Version     : 2.0
 * Copyright(C): 2018 - 2020
 ********************************************************/

#include <cstring>
#include <boost/functional/factory.hpp>
#include <boost/lexical_cast.hpp>
#include "tcp_manager_impl.h"

namespace BoostNet { // namespace BoostNet begin

TcpManagerImpl::TcpManagerImpl()
    : m_io_context_pool()
    , m_acceptors()
    , m_server_ssl_context(boost::asio::ssl::context::sslv23_server)
    , m_client_ssl_context(boost::asio::ssl::context::sslv23_client)
    , m_server_ssl_password()
    , m_client_ssl_password()
    , m_server_ssl_enable(false)
    , m_client_ssl_enable(false)
    , m_tcp_service(nullptr)
{

}

TcpManagerImpl::~TcpManagerImpl()
{

}

const std::string & TcpManagerImpl::get_server_ssl_password() const
{
    return (m_server_ssl_password);
}

const std::string & TcpManagerImpl::get_client_ssl_password() const
{
    return (m_client_ssl_password);
}

bool TcpManagerImpl::set_server_certificate(const Certificate * certificate)
{
    if (nullptr == certificate)
    {
        return (true);
    }

    if (nullptr == certificate->cert_file_or_buffer || nullptr == certificate->key_file_or_buffer)
    {
        return (false);
    }

    m_server_ssl_enable = true;

    m_server_ssl_context.set_options(boost::asio::ssl::context::default_workarounds | boost::asio::ssl::context::no_sslv2 | (nullptr == certificate->dh_file_or_buffer ? 0 : boost::asio::ssl::context::single_dh_use));

    if (nullptr != certificate->password)
    {
        m_server_ssl_password = certificate->password;
        m_server_ssl_context.set_password_callback(boost::bind(&TcpManagerImpl::get_server_ssl_password, this));
    }

    if (certificate->pass_file_not_buffer)
    {
        m_server_ssl_context.use_certificate_chain_file(certificate->cert_file_or_buffer);
        m_server_ssl_context.use_private_key_file(certificate->key_file_or_buffer, boost::asio::ssl::context::pem);

        if (nullptr != certificate->dh_file_or_buffer)
        {
            m_server_ssl_context.use_tmp_dh_file(certificate->dh_file_or_buffer);
        }
    }
    else
    {
        m_server_ssl_context.use_certificate_chain(boost::asio::buffer(certificate->cert_file_or_buffer, strlen(certificate->cert_file_or_buffer)));
        m_server_ssl_context.use_private_key(boost::asio::buffer(certificate->key_file_or_buffer, strlen(certificate->key_file_or_buffer)), boost::asio::ssl::context::pem);

        if (nullptr != certificate->dh_file_or_buffer)
        {
            m_server_ssl_context.use_tmp_dh(boost::asio::buffer(certificate->dh_file_or_buffer, strlen(certificate->dh_file_or_buffer)));
        }
    }

    return (true);
}

bool TcpManagerImpl::set_client_certificate(const Certificate * certificate)
{
    if (nullptr == certificate)
    {
        return (true);
    }

    if (nullptr == certificate->cert_file_or_buffer)
    {
        return (false);
    }

    m_client_ssl_enable = true;

    if (nullptr != certificate->password)
    {
        m_client_ssl_password = certificate->password;
        m_client_ssl_context.set_password_callback(boost::bind(&TcpManagerImpl::get_client_ssl_password, this));
    }

    if (certificate->pass_file_not_buffer)
    {
        m_client_ssl_context.load_verify_file(certificate->cert_file_or_buffer);
        if (nullptr != certificate->key_file_or_buffer)
        {
            m_client_ssl_context.use_certificate_file(certificate->cert_file_or_buffer, boost::asio::ssl::context::pem);
            m_client_ssl_context.use_private_key_file(certificate->key_file_or_buffer, boost::asio::ssl::context::pem);
        }
    }
    else
    {
        m_client_ssl_context.add_certificate_authority(boost::asio::buffer(certificate->cert_file_or_buffer, strlen(certificate->cert_file_or_buffer)));
        if (nullptr != certificate->key_file_or_buffer)
        {
            m_client_ssl_context.use_certificate(boost::asio::buffer(certificate->cert_file_or_buffer, strlen(certificate->cert_file_or_buffer)), boost::asio::ssl::context::pem);
            m_client_ssl_context.use_private_key(boost::asio::buffer(certificate->key_file_or_buffer, strlen(certificate->key_file_or_buffer)), boost::asio::ssl::context::pem);
        }
    }

    return (true);
}

bool TcpManagerImpl::init(TcpServiceBase * tcp_service, std::size_t thread_count, const char * host, unsigned short port_array[], std::size_t port_count, bool port_any_valid, const Certificate * server_certificate, const Certificate * client_certificate)
{
    if (nullptr == tcp_service)
    {
        return (false);
    }

    if (0 == thread_count)
    {
        return (false);
    }

    if (nullptr == port_array && 0 != port_count)
    {
        return (false);
    }

    set_server_certificate(server_certificate);
    set_client_certificate(client_certificate);

    if (m_io_context_pool.size() > 0)
    {
        return (false);
    }

    if (!m_io_context_pool.init(thread_count))
    {
        return (false);
    }

    m_tcp_service = tcp_service;

    if (port_any_valid)
    {
        bool exception = false;
        boost::system::error_code err;
        std::size_t index = 0;
        while (index < port_count)
        {
            unsigned short port = port_array[index];
            if (0 == port)
            {
                m_tcp_service->on_error(TcpConnectionSharedPtr(), "listener", "init", 1, "port is invalid");
            }
            else
            {
                try
                {
                    endpoint_type endpoint(boost::asio::ip::address_v4::from_string(nullptr == host ? "0.0.0.0" : host), port);
                    bool reuse_address = true;
                    m_acceptors.push_back(boost::factory<acceptor_type *>()(m_io_context_pool.get(), endpoint, reuse_address));
                    start_accept(m_acceptors.back(), port);
                    break;
                }
                catch (boost::system::error_code & ec)
                {
                    err = ec;
                }
                catch (...)
                {
                    exception = true;
                }
            }
            ++index;
        }
        if (index == port_count)
        {
            if (err)
            {
                m_tcp_service->on_error(TcpConnectionSharedPtr(), "listener", "init", err.value(), err.message().c_str());
            }
            else if (exception)
            {
                m_tcp_service->on_error(TcpConnectionSharedPtr(), "listener", "init", 1, "start exception");
            }
            return (false);
        }
    }
    else
    {
        try
        {
            for (std::size_t index = 0; index < port_count; ++index)
            {
                unsigned short port = port_array[index];
                if (0 == port)
                {
                    m_tcp_service->on_error(TcpConnectionSharedPtr(), "listener", "init", 1, "port is invalid");
                    return (false);
                }
                else
                {
                    endpoint_type endpoint(boost::asio::ip::address_v4::from_string(nullptr == host ? "0.0.0.0" : host), port);
                    bool reuse_address = true;
                    m_acceptors.push_back(boost::factory<acceptor_type *>()(m_io_context_pool.get(), endpoint, reuse_address));
                    start_accept(m_acceptors.back(), port);
                }
            }
        }
        catch (boost::system::error_code & ec)
        {
            m_tcp_service->on_error(TcpConnectionSharedPtr(), "listener", "init", ec.value(), ec.message().c_str());
            return (false);
        }
        catch (...)
        {
            m_tcp_service->on_error(TcpConnectionSharedPtr(), "listener", "init", 1, "start exception");
            return (false);
        }
    }

    return (true);
}

void TcpManagerImpl::exit()
{
    m_io_context_pool.exit();
    m_acceptors.clear();
    m_tcp_service = nullptr;
}

void TcpManagerImpl::run(bool blocking)
{
    m_io_context_pool.run(blocking);
}

void TcpManagerImpl::start_accept(acceptor_type & acceptor, unsigned short port)
{
    bool passive = true;
    const void * identity = reinterpret_cast<const void *>(port);
    if (m_server_ssl_enable)
    {
        ssl_session_ptr ssl_session = boost::factory<ssl_session_ptr>()(m_io_context_pool.get(), m_server_ssl_context, m_tcp_service, passive, identity);
        acceptor.async_accept(ssl_session->socket_lowest(), boost::bind(&TcpManagerImpl::handle_accept<ssl_session_type, ssl_session_ptr>, this, boost::ref(acceptor), port, ssl_session, boost::asio::placeholders::error));
    }
    else
    {
        tcp_session_ptr tcp_session = boost::factory<tcp_session_ptr>()(m_io_context_pool.get(), m_server_ssl_context, m_tcp_service, passive, identity);
        acceptor.async_accept(tcp_session->socket_lowest(), boost::bind(&TcpManagerImpl::handle_accept<tcp_session_type, tcp_session_ptr>, this, boost::ref(acceptor), port, tcp_session, boost::asio::placeholders::error));
    }
}

bool TcpManagerImpl::create_connection(const std::string & host, const std::string & service, bool sync_connect, const void * identity, const char * bind_ip, unsigned short bind_port)
{
    if (m_client_ssl_enable)
    {
        if (sync_connect)
        {
            return (sync_create_tcp_connection<ssl_session_type, ssl_session_ptr>(host, service, identity, bind_ip, bind_port));
        }
        else
        {
            return (async_create_tcp_connection<ssl_session_type, ssl_session_ptr>(host, service, identity, bind_ip, bind_port));
        }
    }
    else
    {
        if (sync_connect)
        {
            return (sync_create_tcp_connection<tcp_session_type, tcp_session_ptr>(host, service, identity, bind_ip, bind_port));
        }
        else
        {
            return (async_create_tcp_connection<tcp_session_type, tcp_session_ptr>(host, service, identity, bind_ip, bind_port));
        }
    }
}

bool TcpManagerImpl::create_connection(const std::string & host, unsigned short port, bool sync_connect, const void * identity, const char * bind_ip, unsigned short bind_port)
{
    return (create_connection(host, boost::lexical_cast<std::string>(port), sync_connect, identity, bind_ip, bind_port));
}

} // namespace BoostNet end
