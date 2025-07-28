/********************************************************
 * Description : tcp connection
 * Data        : 2018-01-07 18:18:00
 * Author      : yanrk
 * Email       : yanrkchina@163.com
 * Blog        : blog.csdn.net/cxxmaker
 * Version     : 2.0
 * Copyright(C): 2018 - 2020
 ********************************************************/

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
    return m_user_data;
}

TcpSession::TcpSession(io_context_type & io_context, ssl_context_type & ssl_context, TcpServiceBase * tcp_service, bool passive, const void * identity)
    : TcpConnection(io_context, ssl_context, tcp_service, passive, identity, false)
    , m_socket(io_context)
{

}

TcpSession::socket_type & TcpSession::socket()
{
    return m_socket;
}

TcpSession::lowest_type & TcpSession::socket_lowest()
{
    return m_socket;
}

void TcpSession::handshake(bool passive)
{

}

void TcpSession::shutdown()
{
    boost::system::error_code ignore_error_code;
    m_socket.shutdown(socket_type::shutdown_both, ignore_error_code);
    m_socket.close(ignore_error_code);
}

SslSession::SslSession(io_context_type & io_context, ssl_context_type & ssl_context, TcpServiceBase * tcp_service, bool passive, const void * identity)
    : TcpConnection(io_context, ssl_context, tcp_service, passive, identity, true)
    , m_socket(io_context, ssl_context)
{
    if (!passive)
    {
        m_socket.set_verify_mode(boost::asio::ssl::verify_peer);
    }
}

SslSession::socket_type & SslSession::socket()
{
    return m_socket;
}

SslSession::lowest_type & SslSession::socket_lowest()
{
    return m_socket.lowest_layer();
}

void SslSession::handshake(bool passive)
{
    m_socket.async_handshake(
        passive ? boost::asio::ssl::stream_base::server : boost::asio::ssl::stream_base::client,
        [self = shared_from_this()](const boost::system::error_code & error) {
            self->handle_handshake(error);
        }
    );
}

void SslSession::shutdown()
{
    boost::system::error_code ignore_error_code;
    m_socket.shutdown(ignore_error_code);
}

} // namespace BoostNet end
