#ifndef TEST_SERVICE_H
#define TEST_SERVICE_H


#include <map>
#include <atomic>
#include "boost_net.h"

class TestService : public BoostNet::TcpServiceBase, public BoostNet::UdpServiceBase
{
public:
    TestService(bool use_tcp, bool requester, bool sync_connect, std::size_t send_times, std::size_t connect_count);
    virtual ~TestService();

public:
    bool init();
    void exit();

private:
    virtual bool on_connect(BoostNet::TcpConnectionSharedPtr connection, const void * identity) override;
    virtual bool on_accept(BoostNet::TcpConnectionSharedPtr connection, unsigned short listener_port) override;
    virtual bool on_recv(BoostNet::TcpConnectionSharedPtr connection) override;
    virtual bool on_send(BoostNet::TcpConnectionSharedPtr connection) override;
    virtual void on_close(BoostNet::TcpConnectionSharedPtr connection) override;

private:
    bool insert_connection(BoostNet::TcpConnectionSharedPtr connection);
    bool remove_connection(BoostNet::TcpConnectionSharedPtr connection);

private:
    bool check_message(BoostNet::TcpConnectionSharedPtr connection, const char * data, std::size_t data_len);

private:
    bool send_message(BoostNet::TcpConnectionSharedPtr connection);
    bool recv_message(BoostNet::TcpConnectionSharedPtr connection);

private:
    virtual bool on_connect(BoostNet::UdpConnectionSharedPtr connection, const void * identity) override;
    virtual bool on_accept(BoostNet::UdpConnectionSharedPtr connection, unsigned short listener_port) override;
    virtual bool on_recv(BoostNet::UdpConnectionSharedPtr connection) override;
    virtual bool on_send(BoostNet::UdpConnectionSharedPtr connection) override;
    virtual void on_close(BoostNet::UdpConnectionSharedPtr connection) override;

private:
    bool insert_connection(BoostNet::UdpConnectionSharedPtr connection);
    bool remove_connection(BoostNet::UdpConnectionSharedPtr connection);

private:
    bool check_message(BoostNet::UdpConnectionSharedPtr connection, const char * data, std::size_t data_len);

private:
    bool send_message(BoostNet::UdpConnectionSharedPtr connection);
    bool recv_message(BoostNet::UdpConnectionSharedPtr connection);

private:
    bool                                                        m_use_tcp;
    bool                                                        m_requester;
    bool                                                        m_sync_connect;
    std::size_t                                                 m_max_message_count;
    std::size_t                                                 m_max_connect_count;
    std::atomic_long                                            m_connect_count;
    std::atomic_long                                            m_disconnect_count;
    std::atomic_long                                            m_send_finish_count;
    BoostNet::TcpManager                                        m_tcp_manager;
    BoostNet::UdpManager                                        m_udp_manager;
};

class TestServer : public TestService
{
public:
    TestServer(bool use_tcp, std::size_t send_times);
};

class TestClient : public TestService
{
public:
    TestClient(bool use_tcp, bool sync_connect, std::size_t send_times, std::size_t connection_count);
};


#endif // TEST_SERVICE_H
