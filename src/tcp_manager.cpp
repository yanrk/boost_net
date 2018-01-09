/********************************************************
 * Description : tcp manager class
 * Data        : 2018-01-02 11:42:00
 * Author      : yanrk
 * Email       : yanrkchina@163.com
 * Blog        : blog.csdn.net/cxxmaker
 * Version     : 2.0
 * Copyright(C): 2018
 ********************************************************/

#include <boost/functional/factory.hpp>
#include <boost/checked_delete.hpp>
#include "boost_net.h"
#include "tcp_manager_impl.h"

namespace BoostNet { // namespace BoostNet begin

TcpManager::TcpManager()
    : m_manager_impl(nullptr)
{

}

TcpManager::~TcpManager()
{
    exit();
}

bool TcpManager::init(TcpServiceBase * tcp_service, std::size_t thread_count, unsigned short * port_array, std::size_t port_count)
{
    if (nullptr == tcp_service)
    {
        return(false);
    }

    if (nullptr != m_manager_impl)
    {
        return(false);
    }

    m_manager_impl = boost::factory<TcpManagerImpl *>()();
    if (nullptr == m_manager_impl)
    {
        return(false);
    }

    if (m_manager_impl->init(tcp_service, thread_count, port_array, port_count))
    {
        return(true);
    }

    boost::checked_delete(m_manager_impl);
    m_manager_impl = nullptr;

    return(false);
}

void TcpManager::exit()
{
    if (nullptr != m_manager_impl)
    {
        m_manager_impl->exit();
        boost::checked_delete(m_manager_impl);
        m_manager_impl = nullptr;
    }
}

bool TcpManager::create_connection(const std::string & host, const std::string & service, std::size_t identity, const char * bind_ip, unsigned short bind_port)
{
    return(nullptr != m_manager_impl && m_manager_impl->create_connection(host, service, identity, bind_ip, bind_port));
}

bool TcpManager::create_connection(const std::string & host, unsigned short port, std::size_t identity, const char * bind_ip, unsigned short bind_port)
{
    return(nullptr != m_manager_impl && m_manager_impl->create_connection(host, port, identity, bind_ip, bind_port));
}

} // namespace BoostNet end
