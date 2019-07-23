/********************************************************
 * Description : tcp send buffer
 * Data        : 2018-01-07 16:28:00
 * Author      : yanrk
 * Email       : yanrkchina@163.com
 * Blog        : blog.csdn.net/cxxmaker
 * Version     : 2.0
 * Copyright(C): 2018 - 2020
 ********************************************************/

#ifndef BOOST_NET_TCP_SEND_BUFFER_H
#define BOOST_NET_TCP_SEND_BUFFER_H


#include <deque>
#include <vector>
#include <boost/asio.hpp>

namespace BoostNet { // namespace BoostNet begin

class TcpSendBuffer
{
public:
    typedef std::vector<char>                       buffer_type;
    typedef std::deque<buffer_type>                 buffer_deque_type;
    typedef const buffer_type &                     const_buffers_type;

public:
    bool empty() const;
    void commit(std::vector<char> && data);
    const_buffers_type data() const;
    void consume();

private:
    buffer_deque_type                               m_buffer_deque;
};

} // namespace BoostNet end


#endif // BOOST_NET_TCP_SEND_BUFFER_H
