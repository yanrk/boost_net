/********************************************************
 * Description : tcp recv buffer
 * Data        : 2018-01-07 16:06:00
 * Author      : yanrk
 * Email       : yanrkchina@163.com
 * Blog        : blog.csdn.net/cxxmaker
 * Version     : 2.0
 * Copyright(C): 2018 - 2020
 ********************************************************/

#include <boost/cast.hpp>
#include "tcp_recv_buffer.h"

namespace BoostNet { // namespace BoostNet begin

TcpRecvBuffer::mutable_buffers_type TcpRecvBuffer::prepare(size_type size)
{
    return (m_buffer.prepare(size));
}

void TcpRecvBuffer::commit(size_type size)
{
    m_buffer.commit(size);
}

TcpRecvBuffer::size_type TcpRecvBuffer::size() const
{
    return (m_buffer.size());
}

const char * TcpRecvBuffer::c_str() const
{
    return (boost::asio::buffer_cast<const char *>(m_buffer.data()));
}

void TcpRecvBuffer::consume(size_type size)
{
    m_buffer.consume(size);
}

} // namespace BoostNet end
