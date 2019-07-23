#ifndef TEST_SERVICE_H
#define TEST_SERVICE_H


#include <map>
#include <atomic>
#include "boost_net.h"

class TestService : public BoostNet::TcpServiceBase, public BoostNet::UdpServiceBase
{
public:
    TestService(bool use_tcp, bool requester, std::size_t send_times, std::size_t connection_count);
    virtual ~TestService();

public:
    bool init();
    void exit();

private:
    virtual bool on_connect(BoostNet::TcpConnectionSharedPtr connection, std::size_t identity) override;
    virtual bool on_accept(BoostNet::TcpConnectionSharedPtr connection, unsigned short listener_port) override;
    virtual bool on_recv(BoostNet::TcpConnectionSharedPtr connection) override;
    virtual bool on_send(BoostNet::TcpConnectionSharedPtr connection) override;
    virtual bool on_close(BoostNet::TcpConnectionSharedPtr connection) override;

private:
    bool insert_connection(BoostNet::TcpConnectionSharedPtr connection);
    bool remove_connection(BoostNet::TcpConnectionSharedPtr connection);

private:
    bool check_message(BoostNet::TcpConnectionSharedPtr connection, const char * data, std::size_t data_len);

private:
    bool send_message(BoostNet::TcpConnectionSharedPtr connection);
    bool recv_message(BoostNet::TcpConnectionSharedPtr connection);

private:
    virtual bool on_connect(BoostNet::UdpConnectionSharedPtr connection, std::size_t identity) override;
    virtual bool on_accept(BoostNet::UdpConnectionSharedPtr connection, unsigned short listener_port) override;
    virtual bool on_recv(BoostNet::UdpConnectionSharedPtr connection) override;
    virtual bool on_send(BoostNet::UdpConnectionSharedPtr connection) override;
    virtual bool on_close(BoostNet::UdpConnectionSharedPtr connection) override;

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
    std::size_t                                                 m_max_msg_cnt;
    std::size_t                                                 m_max_connection_cnt;
    std::atomic_long                                            m_connect_count;
    std::atomic_long                                            m_disconnect_count;
    std::atomic_long                                            m_send_finish_count;
    BoostNet::TcpManager                                        m_tcp_manager;
    BoostNet::UdpManager                                        m_udp_manager;
};


#endif // TEST_SERVICE_H
