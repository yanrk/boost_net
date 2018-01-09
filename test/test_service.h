#ifndef TEST_SERVICE_H
#define TEST_SERVICE_H


#include <map>
#include <atomic>
#include "boost_net.h"

class TestService : public BoostNet::TcpServiceBase
{
public:
    TestService(bool requester, std::size_t send_times, std::size_t connection_count);
    virtual ~TestService();

public:
    bool init();
    void exit();

private:
    virtual bool on_connect(BoostNet::TcpConnectionSharedPtr connection, std::size_t identity);
    virtual bool on_accept(BoostNet::TcpConnectionSharedPtr connection, unsigned short listener_port);
    virtual bool on_recv(BoostNet::TcpConnectionSharedPtr connection);
    virtual bool on_send(BoostNet::TcpConnectionSharedPtr connection);
    virtual bool on_close(BoostNet::TcpConnectionSharedPtr connection);

private:
    bool insert_connection(BoostNet::TcpConnectionSharedPtr connection);
    bool remove_connection(BoostNet::TcpConnectionSharedPtr connection);

private:
    bool check_message(BoostNet::TcpConnectionSharedPtr connection, const char * data, std::size_t data_len);

private:
    bool send_message(BoostNet::TcpConnectionSharedPtr connection);
    bool recv_message(BoostNet::TcpConnectionSharedPtr connection);

private:
    bool                                                        m_requester;
    std::size_t                                                 m_max_msg_cnt;
    std::size_t                                                 m_max_connection_cnt;
    std::atomic_long                                            m_connect_count;
    std::atomic_long                                            m_disconnect_count;
    std::atomic_long                                            m_send_finish_count;
    BoostNet::TcpManager                                        m_tcp_manager;
};


#endif // TEST_SERVICE_H
