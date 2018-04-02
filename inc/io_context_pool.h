/********************************************************
 * Description : io context pool
 * Data        : 2018-01-02 10:30:00
 * Author      : yanrk
 * Email       : yanrkchina@163.com
 * Blog        : blog.csdn.net/cxxmaker
 * Version     : 2.0
 * Copyright(C): 2018
 ********************************************************/

#ifndef BOOST_NET_IO_CONTEXT_POOL_H
#define BOOST_NET_IO_CONTEXT_POOL_H


#include <boost/noncopyable.hpp>
#include <boost/ptr_container/ptr_vector.hpp>
#include <boost/asio.hpp>
#include <boost/thread.hpp>

namespace BoostNet { // namespace BoostNet begin

class IOServicePool : private boost::noncopyable
{
public:
    typedef boost::asio::io_context                 io_context_type;
    typedef boost::ptr_vector<io_context_type>      io_contexts_type;
    typedef boost::asio::io_context::work           work_type;
    typedef boost::ptr_vector<work_type>            works_type;
    typedef boost::system::error_code               error_code_type;
    typedef boost::ptr_vector<error_code_type>      error_codes_type;
    typedef boost::thread_group                     thread_group_type;

public:
    explicit IOServicePool();

public:
    bool init(std::size_t pool_size);
    void exit();

public:
    void run(bool blocking = false);
    io_context_type & get();
    std::size_t size();

private:
    io_contexts_type                                m_io_contexts;
    works_type                                      m_works;
    error_codes_type                                m_error_codes;
    thread_group_type                               m_thread_group;
    std::size_t                                     m_next_io_context;
};

} // namespace BoostNet end


#endif // BOOST_NET_IO_CONTEXT_POOL_H
