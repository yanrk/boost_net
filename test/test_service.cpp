#include <cstdio>
#include <cassert>
#include <cstring>
#include <string>
#include <algorithm>
#include "test_service.h"

static const char msg_blk[] = "this is a message\n";
static const std::size_t msg_len = sizeof(msg_blk) / sizeof(msg_blk[0]) - 1;

TestService::TestService(bool requester, std::size_t send_times, std::size_t connection_count)
    : m_requester(requester)
    , m_max_msg_cnt(std::min(send_times, (65536 - 2 - 1) / msg_len))
    , m_max_connection_cnt(connection_count)
    , m_connect_count(0)
    , m_disconnect_count(0)
    , m_send_finish_count(0)
    , m_tcp_manager()
{

}

TestService::~TestService()
{

}

bool TestService::on_connect(BoostNet::TcpConnectionSharedPtr connection, std::size_t identity)
{
    assert(m_requester);

    if (m_requester)
    {
        return(insert_connection(connection) && send_message(connection));
    }
    else
    {
        assert(false);
        return(false);
    }
}

bool TestService::on_accept(BoostNet::TcpConnectionSharedPtr connection, unsigned short listener_port)
{
    assert(!m_requester);

    if (!m_requester)
    {
        return(insert_connection(connection));
    }
    else
    {
        assert(false);
        return(false);
    }
}

bool TestService::on_recv(BoostNet::TcpConnectionSharedPtr connection)
{
    return(recv_message(connection));
}

bool TestService::on_send(BoostNet::TcpConnectionSharedPtr connection)
{
    return(true);
}

bool TestService::on_close(BoostNet::TcpConnectionSharedPtr connection)
{
    return(remove_connection(connection));
}

bool TestService::insert_connection(BoostNet::TcpConnectionSharedPtr connection)
{
    std::string host_ip;
    unsigned short host_port = 0;
    connection->get_host_address(host_ip, host_port);
    std::string peer_ip;
    unsigned short peer_port = 0;
    connection->get_peer_address(peer_ip, peer_port);
    printf("connect: %u, [%s:%u] -> [%s:%u]\n", static_cast<uint32_t>(++m_connect_count), host_ip.c_str(), host_port, peer_ip.c_str(), peer_port);
    connection->set_user_data(reinterpret_cast<void *>(0));
    return(true);
}

bool TestService::remove_connection(BoostNet::TcpConnectionSharedPtr connection)
{
    std::string host_ip;
    unsigned short host_port = 0;
    connection->get_host_address(host_ip, host_port);
    std::string peer_ip;
    unsigned short peer_port = 0;
    connection->get_peer_address(peer_ip, peer_port);
    printf("disconnect: %u, [%s:%u] -> [%s:%u]\n", static_cast<uint32_t>(++m_disconnect_count), host_ip.c_str(), host_port, peer_ip.c_str(), peer_port);
    std::size_t count = reinterpret_cast<std::size_t>(connection->get_user_data());
    if (count < m_max_msg_cnt)
    {
        assert(false);
    }
    return(true);
}

bool TestService::send_message(BoostNet::TcpConnectionSharedPtr connection)
{
    if (0 == m_max_msg_cnt)
    {
        return(true);
    }

    std::size_t count = reinterpret_cast<std::size_t>(connection->get_user_data());
    if (count >= m_max_msg_cnt)
    {
        std::string host_ip;
        unsigned short host_port = 0;
        connection->get_host_address(host_ip, host_port);
        std::string peer_ip;
        unsigned short peer_port = 0;
        connection->get_peer_address(peer_ip, peer_port);
        printf("send finish: %u, [%s:%u] -> [%s:%u]\n", static_cast<uint32_t>(++m_send_finish_count), host_ip.c_str(), host_port, peer_ip.c_str(), peer_port);
        return(false);
    }

    ++count;

    std::size_t data_len = 2 + msg_len * count + 1;

    char head[2] = { static_cast<char>(data_len / 256U), static_cast<char>(data_len % 256U) };
    if (!connection->send_buffer_fill_len(head, 2))
    {
        assert(false);
        return(false);
    }

    for (std::size_t index = 0; index < count; ++index)
    {
        if (!connection->send_buffer_fill_len(msg_blk, msg_len))
        {
            assert(false);
            return(false);
        }
    }

    char tail[1] = { 0x00 };
    if (!connection->send_buffer_fill_len(tail, 1))
    {
        assert(false);
        return(false);
    }

    connection->set_user_data(reinterpret_cast<void *>(count));

    return(true);
}

bool TestService::recv_message(BoostNet::TcpConnectionSharedPtr connection)
{
    assert(0 != m_max_msg_cnt);

    const char * data = reinterpret_cast<const char *>(connection->recv_buffer_data());
    std::size_t data_len = connection->recv_buffer_size();

    if (data_len < 2)
    {
        return(true);
    }

    std::size_t need_len = static_cast<unsigned char>(data[0]) * 256U + static_cast<unsigned char>(data[1]);
    if (data_len < need_len)
    {
        connection->recv_buffer_water_mark(need_len);
        return(true);
    }

    if (!check_message(connection, data, need_len))
    {
        assert(false);
        return(false);
    }

    if (!connection->recv_buffer_drop_len(need_len))
    {
        assert(false);
        return(false);
    }

    connection->recv_buffer_water_mark(2);

    if (!send_message(connection))
    {
        connection->close();
        return(false);
    }

    return(true);
}

bool TestService::check_message(BoostNet::TcpConnectionSharedPtr connection, const char * data, std::size_t data_len)
{
    std::size_t count = reinterpret_cast<std::size_t>(connection->get_user_data());
    if (count >= m_max_msg_cnt)
    {
        assert(false);
        return(false);
    }

    ++count;

    const std::size_t need_len = 2 + msg_len * count + 1;
    if (need_len != data_len)
    {
        assert(false);
        return(false);
    }

    data += 2;
    data_len -= 2;
    for (std::size_t index = 0; index < count; ++index)
    {
        if (0 != memcmp(data, msg_blk, msg_len))
        {
            assert(false);
        }
        data += msg_len;
        data_len -= msg_len;
    }
    if ('\0' != data[0] || 1 != data_len)
    {
        assert(false);
    }

    connection->set_user_data(reinterpret_cast<void *>(count));

    return(true);
}

bool TestService::init()
{
    if (m_requester)
    {
        if (!m_tcp_manager.init(this, 5, nullptr, 0))
        {
            return(false);
        }
        for (std::size_t index = 0; index < m_max_connection_cnt; ++index)
        {
            if (!m_tcp_manager.create_connection("127.0.0.1", 12345))
            {
                return(false);
            }
        }
        return(true);
    }
    else
    {
        unsigned short port[] = { 12345 };
        if (!m_tcp_manager.init(this, 5, port, sizeof(port) / sizeof(port[0])))
        {
            return(false);
        }
        return(true);
    }
}

void TestService::exit()
{
    m_tcp_manager.exit();
}
