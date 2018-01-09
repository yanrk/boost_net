/********************************************************
 * Description : tcp send buffer
 * Data        : 2018-01-07 16:28:00
 * Author      : yanrk
 * Email       : yanrkchina@163.com
 * Blog        : blog.csdn.net/cxxmaker
 * Version     : 2.0
 * Copyright(C): 2018
 ********************************************************/

#include "tcp_send_buffer.h"

namespace BoostNet { // namespace BoostNet begin

bool TcpSendBuffer::empty() const
{
    return(m_buffer_deque.empty());
}

void TcpSendBuffer::commit(std::vector<char> && data)
{
    m_buffer_deque.push_back(data);
}

TcpSendBuffer::const_buffers_type TcpSendBuffer::data() const
{
    return(m_buffer_deque.front());
}

void TcpSendBuffer::consume()
{
    m_buffer_deque.pop_front();
}

} // namespace BoostNet end
