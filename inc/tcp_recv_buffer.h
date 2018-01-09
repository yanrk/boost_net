/********************************************************
 * Description : tcp recv buffer
 * Data        : 2018-01-07 16:06:00
 * Author      : yanrk
 * Email       : yanrkchina@163.com
 * Blog        : blog.csdn.net/cxxmaker
 * Version     : 2.0
 * Copyright(C): 2018
 ********************************************************/

#ifndef BOOST_NET_TCP_RECV_BUFFER_H
#define BOOST_NET_TCP_RECV_BUFFER_H


#include <boost/asio.hpp>

namespace BoostNet { // namespace BoostNet begin

class TcpRecvBuffer
{
public:
    typedef std::size_t                             size_type;
    typedef boost::asio::streambuf                  streambuf_type;
    typedef streambuf_type::mutable_buffers_type    mutable_buffers_type;

public:
    mutable_buffers_type prepare(size_type size = 512);
    void commit(size_type size);
    size_type size() const;
    const char * c_str() const;
    void consume(size_type size);

private:
    streambuf_type                                  m_buffer;
};

} // namespace BoostNet end


#endif // BOOST_NET_TCP_RECV_BUFFER_H
