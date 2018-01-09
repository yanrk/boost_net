/********************************************************
 * Description : io service pool
 * Data        : 2018-01-02 10:30:00
 * Author      : yanrk
 * Email       : yanrkchina@163.com
 * Blog        : blog.csdn.net/cxxmaker
 * Version     : 2.0
 * Copyright(C): 2018
 ********************************************************/

#include <boost/bind.hpp>
#include <boost/functional/factory.hpp>
#include "io_service_pool.h"

namespace BoostNet { // namespace BoostNet begin

IOServicePool::IOServicePool()
    : m_io_services()
    , m_works()
    , m_error_codes()
    , m_thread_group()
    , m_next_io_service(0)
{

}

bool IOServicePool::init(std::size_t pool_size)
{
    if (0 == pool_size)
    {
        return(false);
    }

    if (m_thread_group.size() > 0)
    {
        return(false);
    }

    for (std::size_t index = 0; index < pool_size; ++index)
    {
        m_io_services.push_back(boost::factory<io_service_type *>()());
        m_works.push_back(boost::factory<work_type *>()(m_io_services.back()));
        m_error_codes.push_back(boost::factory<boost::system::error_code *>()());
        if (nullptr == m_thread_group.create_thread(boost::bind(&io_service_type::run, boost::ref(m_io_services.back()), boost::ref(m_error_codes.back()))))
        {
            return(false);
        }
    }

    return(true);
}

void IOServicePool::exit()
{
    m_works.clear();

    for (std::size_t index = 0; index < m_io_services.size(); ++index)
    {
        m_io_services[index].stop();
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

IOServicePool::io_service_type & IOServicePool::get()
{
    return(m_io_services[m_next_io_service++ % m_io_services.size()]);
}

std::size_t IOServicePool::size()
{
    return(m_io_services.size());
}

} // namespace BoostNet end
