/********************************************************
 * Description : udp connection base
 * Data        : 2019-07-22 13:45:00
 * Author      : yanrk
 * Email       : yanrkchina@163.com
 * Blog        : blog.csdn.net/cxxmaker
 * Version     : 2.0
 * Copyright(C): 2018 - 2020
 ********************************************************/

#include "boost_net.h"

namespace BoostNet { // namespace BoostNet begin

UdpConnectionBase::UdpConnectionBase()
    : m_user_data(nullptr)
{

}

UdpConnectionBase::~UdpConnectionBase()
{

}

void UdpConnectionBase::set_user_data(void * user_data)
{
    m_user_data = user_data;
}

void * UdpConnectionBase::get_user_data()
{
    return m_user_data;
}

} // namespace BoostNet end
