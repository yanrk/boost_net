/********************************************************
 * Description : io service pool
 * Data        : 2018-01-02 10:30:00
 * Author      : yanrk
 * Email       : yanrkchina@163.com
 * Blog        : blog.csdn.net/cxxmaker
 * Version     : 2.0
 * Copyright(C): 2018 - 2020
 ********************************************************/

#include <boost/bind.hpp>
#include <boost/functional/factory.hpp>
#include "io_context_pool.h"

namespace BoostNet { // namespace BoostNet begin

IOServicePool::IOServicePool()
    : m_io_contexts()
    , m_works()
    , m_error_codes()
    , m_thread_group()
    , m_next_io_context(0)
{

}

bool IOServicePool::init(std::size_t pool_size)
{
    if (0 == pool_size)
    {
        return (false);
    }

    if (m_thread_group.size() > 0)
    {
        return (false);
    }

    for (std::size_t index = 0; index < pool_size; ++index)
    {
        m_io_contexts.push_back(boost::factory<io_context_type *>()());
        m_works.push_back(boost::factory<work_type *>()(m_io_contexts.back()));
        m_error_codes.push_back(boost::factory<boost::system::error_code *>()());
        if (nullptr == m_thread_group.create_thread(boost::bind(&io_context_type::run, boost::ref(m_io_contexts.back()), boost::ref(m_error_codes.back()))))
        {
            return (false);
        }
    }

    return (true);
}

void IOServicePool::exit()
{
    m_works.clear();

    for (std::size_t index = 0; index < m_io_contexts.size(); ++index)
    {
        m_io_contexts[index].stop();
    }

    m_error_codes.clear();
}

void IOServicePool::run(bool blocking)
{
    if (blocking)
    {
        m_thread_group.join_all();
    }
}

IOServicePool::io_context_type & IOServicePool::get()
{
    return (m_io_contexts[m_next_io_context++ % m_io_contexts.size()]);
}

std::size_t IOServicePool::size()
{
    return (m_io_contexts.size());
}

} // namespace BoostNet end
