/********************************************************
 * Description : udp manager class
 * Data        : 2019-07-23 10:00:00
 * Author      : yanrk
 * Email       : yanrkchina@163.com
 * Blog        : blog.csdn.net/cxxmaker
 * Version     : 2.0
 * Copyright(C): 2018 - 2020
 ********************************************************/

#include <boost/functional/factory.hpp>
#include <boost/checked_delete.hpp>
#include "udp_manager_impl.h"

namespace BoostNet { // namespace BoostNet begin

UdpManager::UdpManager()
    : m_manager_impl(nullptr)
{

}

UdpManager::~UdpManager()
{
    exit();
}

bool UdpManager::init(UdpServiceBase * udp_service, std::size_t thread_count, const char * host, unsigned short * port_array, std::size_t port_count, bool port_any_valid)
{
    if (nullptr == udp_service)
    {
        return false;
    }

    if (nullptr != m_manager_impl)
    {
        return false;
    }

    m_manager_impl = boost::factory<UdpManagerImpl *>()();
    if (nullptr == m_manager_impl)
    {
        return false;
    }

    if (m_manager_impl->init(udp_service, thread_count, host, port_array, port_count, port_any_valid))
    {
        return true;
    }

    boost::checked_delete(m_manager_impl);
    m_manager_impl = nullptr;

    return false;
}

void UdpManager::exit()
{
    if (nullptr != m_manager_impl)
    {
        m_manager_impl->exit();
        boost::checked_delete(m_manager_impl);
        m_manager_impl = nullptr;
    }
}

void UdpManager::get_ports(std::vector<unsigned short> & ports)
{
    if (nullptr != m_manager_impl)
    {
        m_manager_impl->get_ports(ports);
    }
}

bool UdpManager::create_connection(const std::string & host, const std::string & service, bool sync_connect, const void * identity, const char * bind_ip, unsigned short bind_port)
{
    return nullptr != m_manager_impl && m_manager_impl->create_connection(host, service, sync_connect, identity, bind_ip, bind_port);
}

bool UdpManager::create_connection(const std::string & host, unsigned short port, bool sync_connect, const void * identity, const char * bind_ip, unsigned short bind_port)
{
    return nullptr != m_manager_impl && m_manager_impl->create_connection(host, port, sync_connect, identity, bind_ip, bind_port);
}

} // namespace BoostNet end
