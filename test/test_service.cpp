#include <cstdio>
#include <cassert>
#include <cstring>
#include <string>
#include <algorithm>
#include "test_service.h"

static const char msg_blk[] = "this is a message\n";
static const std::size_t msg_len = sizeof(msg_blk) / sizeof(msg_blk[0]) - 1;

TestService::TestService(bool use_tcp, bool requester, bool sync_connect, std::size_t send_times, std::size_t connect_count)
    : m_use_tcp(use_tcp)
    , m_requester(requester)
    , m_sync_connect(sync_connect)
    , m_max_message_count(std::min(use_tcp ? send_times : send_times + 1, (65536 - 2 - 1) / msg_len))
    , m_max_connect_count(connect_count)
    , m_connect_count(0)
    , m_disconnect_count(0)
    , m_send_finish_count(0)
    , m_tcp_manager()
{

}

TestService::~TestService()
{

}

bool TestService::on_connect(BoostNet::TcpConnectionSharedPtr connection, const void * identity)
{
    assert(m_use_tcp && m_requester);

    if (m_requester)
    {
        return insert_connection(connection) && send_message(connection);
    }
    else
    {
        return false;
    }
}

bool TestService::on_accept(BoostNet::TcpConnectionSharedPtr connection, unsigned short listener_port)
{
    assert(m_use_tcp && !m_requester);

    if (!m_requester)
    {
        return insert_connection(connection);
    }
    else
    {
        assert(false);
        return false;
    }
}

bool TestService::on_recv(BoostNet::TcpConnectionSharedPtr connection)
{
    return recv_message(connection);
}

bool TestService::on_send(BoostNet::TcpConnectionSharedPtr connection)
{
    return true;
}

void TestService::on_close(BoostNet::TcpConnectionSharedPtr connection)
{
    remove_connection(connection);
}

void TestService::on_error(BoostNet::TcpConnectionSharedPtr connection, const char * operater, const char * action, int error, const char * message)
{

}

bool TestService::insert_connection(BoostNet::TcpConnectionSharedPtr connection)
{
    if (!connection)
    {
        return false;
    }
    std::string host_ip;
    unsigned short host_port = 0;
    connection->get_host_address(host_ip, host_port);
    std::string peer_ip;
    unsigned short peer_port = 0;
    connection->get_peer_address(peer_ip, peer_port);
    printf("connect: %u, [%s:%u] -> [%s:%u]\n", static_cast<uint32_t>(++m_connect_count), host_ip.c_str(), host_port, peer_ip.c_str(), peer_port);
    connection->set_user_data(reinterpret_cast<void *>(0));
    return true;
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
    if (count < m_max_message_count)
    {
        assert(false);
    }
    return true;
}

bool TestService::send_message(BoostNet::TcpConnectionSharedPtr connection)
{
    if (0 == m_max_message_count)
    {
        return true;
    }

    std::size_t count = reinterpret_cast<std::size_t>(connection->get_user_data());
    if (count >= m_max_message_count)
    {
        std::string host_ip;
        unsigned short host_port = 0;
        connection->get_host_address(host_ip, host_port);
        std::string peer_ip;
        unsigned short peer_port = 0;
        connection->get_peer_address(peer_ip, peer_port);
        printf("send finish: %u, [%s:%u] -> [%s:%u]\n", static_cast<uint32_t>(++m_send_finish_count), host_ip.c_str(), host_port, peer_ip.c_str(), peer_port);
        return false;
    }

    ++count;

    std::size_t data_len = 2 + msg_len * count + 1;

    char head[2] = { static_cast<char>(data_len / 256U), static_cast<char>(data_len % 256U) };
    if (!connection->send_buffer_fill(head, 2))
    {
        assert(false);
        return false;
    }

    for (std::size_t index = 0; index < count; ++index)
    {
        if (!connection->send_buffer_fill(msg_blk, msg_len))
        {
            assert(false);
            return false;
        }
    }

    char tail[1] = { 0x00 };
    if (!connection->send_buffer_fill(tail, 1))
    {
        assert(false);
        return false;
    }

    connection->set_user_data(reinterpret_cast<void *>(count));

    return true;
}

bool TestService::recv_message(BoostNet::TcpConnectionSharedPtr connection)
{
    assert(0 != m_max_message_count);

    const char * data = reinterpret_cast<const char *>(connection->recv_buffer_data());
    std::size_t data_len = connection->recv_buffer_size();

    if (data_len < 2)
    {
        return true;
    }

    std::size_t need_len = static_cast<unsigned char>(data[0]) * 256U + static_cast<unsigned char>(data[1]);
    if (data_len < need_len)
    {
        connection->recv_buffer_water_mark(need_len);
        return true;
    }

    if (!check_message(connection, data, need_len))
    {
        assert(false);
        return false;
    }

    if (!connection->recv_buffer_drop(need_len))
    {
        assert(false);
        return false;
    }

    connection->recv_buffer_water_mark(2);

    if (!send_message(connection))
    {
        connection->close();
        return false;
    }

    return true;
}

bool TestService::check_message(BoostNet::TcpConnectionSharedPtr connection, const char * data, std::size_t data_len)
{
    std::size_t count = reinterpret_cast<std::size_t>(connection->get_user_data());
    if (count >= m_max_message_count)
    {
        assert(false);
        return false;
    }

    ++count;

    const std::size_t need_len = 2 + msg_len * count + 1;
    if (need_len != data_len)
    {
        assert(false);
        return false;
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

    return true;
}

bool TestService::on_connect(BoostNet::UdpConnectionSharedPtr connection, const void * identity)
{
    assert(!m_use_tcp && m_requester);

    if (m_requester)
    {
        return insert_connection(connection) && send_message(connection);
    }
    else
    {
        assert(false);
        return false;
    }
}

bool TestService::on_accept(BoostNet::UdpConnectionSharedPtr connection, unsigned short listener_port)
{
    assert(!m_use_tcp && !m_requester);

    if (!m_requester)
    {
        return insert_connection(connection);
    }
    else
    {
        assert(false);
        return false;
    }
}

bool TestService::on_recv(BoostNet::UdpConnectionSharedPtr connection)
{
    return recv_message(connection);
}

bool TestService::on_send(BoostNet::UdpConnectionSharedPtr connection)
{
    return true;
}

void TestService::on_close(BoostNet::UdpConnectionSharedPtr connection)
{
    remove_connection(connection);
}

void TestService::on_error(BoostNet::UdpConnectionSharedPtr connection, const char * operater, const char * action, int error, const char * message)
{

}

bool TestService::insert_connection(BoostNet::UdpConnectionSharedPtr connection)
{
    if (!connection)
    {
        return false;
    }
    std::string host_ip;
    unsigned short host_port = 0;
    connection->get_host_address(host_ip, host_port);
    std::string peer_ip;
    unsigned short peer_port = 0;
    connection->get_peer_address(peer_ip, peer_port);
    printf("connect: %u, [%s:%u] -> [%s:%u]\n", static_cast<uint32_t>(++m_connect_count), host_ip.c_str(), host_port, peer_ip.c_str(), peer_port);
    connection->set_user_data(reinterpret_cast<void *>(0));
    return true;
}

bool TestService::remove_connection(BoostNet::UdpConnectionSharedPtr connection)
{
    std::string host_ip;
    unsigned short host_port = 0;
    connection->get_host_address(host_ip, host_port);
    std::string peer_ip;
    unsigned short peer_port = 0;
    connection->get_peer_address(peer_ip, peer_port);
    printf("disconnect: %u, [%s:%u] -> [%s:%u]\n", static_cast<uint32_t>(++m_disconnect_count), host_ip.c_str(), host_port, peer_ip.c_str(), peer_port);
    std::size_t count = reinterpret_cast<std::size_t>(connection->get_user_data());
    if (count < m_max_message_count)
    {
        assert(false);
    }
    return true;
}

bool TestService::send_message(BoostNet::UdpConnectionSharedPtr connection)
{
    if (0 == m_max_message_count)
    {
        return true;
    }

    std::size_t count = reinterpret_cast<std::size_t>(connection->get_user_data());
    if (count >= m_max_message_count)
    {
        std::string host_ip;
        unsigned short host_port = 0;
        connection->get_host_address(host_ip, host_port);
        std::string peer_ip;
        unsigned short peer_port = 0;
        connection->get_peer_address(peer_ip, peer_port);
        printf("send finish: %u, [%s:%u] -> [%s:%u]\n", static_cast<uint32_t>(++m_send_finish_count), host_ip.c_str(), host_port, peer_ip.c_str(), peer_port);
        return false;
    }

    ++count;

    if (!connection->send_buffer_fill(msg_blk, msg_len))
    {
        assert(false);
        return false;
    }

    connection->set_user_data(reinterpret_cast<void *>(count));

    return true;
}

bool TestService::recv_message(BoostNet::UdpConnectionSharedPtr connection)
{
    assert(0 != m_max_message_count);

    if (!connection->recv_buffer_has_data())
    {
        assert(false);
        return false;
    }

    const char * data = reinterpret_cast<const char *>(connection->recv_buffer_data());
    std::size_t data_len = connection->recv_buffer_size();

    if (!check_message(connection, data, data_len))
    {
        assert(false);
        return false;
    }

    if (!connection->recv_buffer_drop())
    {
        assert(false);
        return false;
    }

    if (connection->recv_buffer_has_data())
    {
        assert(false);
        return false;
    }

    if (!send_message(connection))
    {
        connection->close();
        return false;
    }

    return true;
}

bool TestService::check_message(BoostNet::UdpConnectionSharedPtr connection, const char * data, std::size_t data_len)
{
    std::size_t count = reinterpret_cast<std::size_t>(connection->get_user_data());
    if (count >= m_max_message_count)
    {
        assert(false);
        return false;
    }

    ++count;

    if (msg_len != data_len || 0 != memcmp(data, msg_blk, msg_len))
    {
        assert(false);
        return false;
    }

    connection->set_user_data(reinterpret_cast<void *>(count));

    return true;
}

bool TestService::init()
{
    if (m_use_tcp)
    {
        if (m_requester)
        {
            if (!m_tcp_manager.init(this, 5, nullptr, 0))
            {
                return false;
            }
            for (std::size_t index = 0; index < m_max_connect_count; ++index)
            {
                if (!m_tcp_manager.create_connection("127.0.0.1", 12345, m_sync_connect))
                {
                    return false;
                }
            }
            return true;
        }
        else
        {
            const char * host = "0.0.0.0";
            unsigned short port[] = { 12345 };
            if (!m_tcp_manager.init(this, 5, host, port, sizeof(port) / sizeof(port[0])))
            {
                return false;
            }
            return true;
        }
    }
    else
    {
        if (m_requester)
        {
            if (!m_udp_manager.init(this, 5, nullptr, 0))
            {
                return false;
            }
            for (std::size_t index = 0; index < m_max_connect_count; ++index)
            {
                if (!m_udp_manager.create_connection("127.0.0.1", 12345, m_sync_connect))
                {
                    return false;
                }
            }
            return true;
        }
        else
        {
            const char * host = "0.0.0.0";
            unsigned short port[] = { 12345 };
            if (!m_udp_manager.init(this, 5, host, port, sizeof(port) / sizeof(port[0])))
            {
                return false;
            }
            return true;
        }
    }
}

void TestService::exit()
{
    m_tcp_manager.exit();
    m_udp_manager.exit();
}

TestServer::TestServer(bool use_tcp, std::size_t send_times)
    : TestService(use_tcp, false, false, send_times, 0)
{

}

TestClient::TestClient(bool use_tcp, bool sync_connect, std::size_t send_times, std::size_t connect_count)
    : TestService(use_tcp, true, sync_connect, send_times, connect_count)
{

}
