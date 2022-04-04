/********************************************************
 * Description : ssl/tls proxy
 * Data        : 2022-04-04 02:25:00
 * Author      : yanrk
 * Email       : yanrkchina@163.com
 * Blog        : blog.csdn.net/cxxmaker
 * Version     : 2.0
 * Copyright(C): 2022 - 2023
 ********************************************************/

#include <ctime>
#include <cstdio>
#include <mutex>
#include <iostream>
#include "boost_net.h"

#define RUN_LOG_DBG(fmt, ...) fprintf(stdout, fmt "\n", ##__VA_ARGS__)
#define RUN_LOG_ERR(fmt, ...) fprintf(stderr, fmt "\n", ##__VA_ARGS__)

struct proxy_config_t
{
    unsigned short                      src_port;
    std::string                         dst_host;
    unsigned short                      dst_port;
    std::string                         crt_file;
    std::string                         key_file;
};

struct connection_pair_t
{
    std::mutex                          mtx;
    BoostNet::TcpConnectionSharedPtr    src;
    BoostNet::TcpConnectionSharedPtr    dst;
    std::string                         src_host;
    unsigned short                      src_port;
    std::string                         pxy_host;
    unsigned short                      pxy_port;
    std::string                         dst_host;
    unsigned short                      dst_port;
};

class SslProxy : public BoostNet::TcpServiceBase
{
public:
    SslProxy();
    virtual ~SslProxy();

private:
    SslProxy(const SslProxy &) = delete;
    SslProxy(SslProxy &&) = delete;
    SslProxy & operator = (const SslProxy &) = delete;
    SslProxy & operator = (SslProxy &&) = delete;

public:
    bool init(const proxy_config_t & proxy_config);
    void exit();

public:
    virtual bool on_connect(BoostNet::TcpConnectionSharedPtr connection, const void * identity);
    virtual bool on_accept(BoostNet::TcpConnectionSharedPtr connection, unsigned short listener_port);
    virtual bool on_recv(BoostNet::TcpConnectionSharedPtr connection);
    virtual bool on_send(BoostNet::TcpConnectionSharedPtr connection);
    virtual void on_close(BoostNet::TcpConnectionSharedPtr connection);

private:
    bool running() const;

private:
    void record_connection(const connection_pair_t * connection_pair, const char * description);

private:
    bool                                    m_running;
    std::string                             m_src_host;
    unsigned short                          m_src_port;
    std::string                             m_dst_host;
    unsigned short                          m_dst_port;
    BoostNet::TcpManager                    m_tcp_manager;
};

SslProxy::SslProxy()
    : m_running(false)
    , m_src_host()
    , m_src_port(0)
    , m_dst_host()
    , m_dst_port(0)
    , m_tcp_manager()
{

}

SslProxy::~SslProxy()
{
    exit();
}

bool SslProxy::running() const
{
    return (m_running);
}

bool SslProxy::init(const proxy_config_t & proxy_config)
{
    exit();

    RUN_LOG_DBG("ssl proxy init begin");

    m_running = true;

    m_src_host = "0.0.0.0";
    m_src_port = proxy_config.src_port;
    m_dst_host = proxy_config.dst_host;
    m_dst_port = proxy_config.dst_port;

    BoostNet::Certificate client_certificate = { 0x0 };
    client_certificate.pass_file_not_buffer = true;
    client_certificate.cert_file_or_buffer = proxy_config.crt_file.c_str();
    client_certificate.key_file_or_buffer = proxy_config.key_file.c_str();
    client_certificate.dh_file_or_buffer = nullptr;
    client_certificate.password = nullptr;

    if (!m_tcp_manager.init(this, 10, m_src_host.c_str(), &m_src_port, 1, nullptr, &client_certificate))
    {
        RUN_LOG_ERR("ssl proxy init failure while tcp manager init on port %d", m_src_port);
        return (false);
    }

    RUN_LOG_DBG("ssl proxy init success");

    return (true);
}

void SslProxy::exit()
{
    if (!running())
    {
        return;
    }

    RUN_LOG_DBG("ssl proxy exit begin");

    m_running = false;

    RUN_LOG_DBG("ssl proxy exit while tcp manager exit begin");
    m_tcp_manager.exit();
    RUN_LOG_DBG("ssl proxy exit while tcp manager exit end");

    RUN_LOG_DBG("ssl proxy exit success");
}

void SslProxy::record_connection(const connection_pair_t * connection_pair, const char * description)
{
    RUN_LOG_DBG("connection %s:%d - %s:%d - %s:%d %s",
        connection_pair->src_host.c_str(), connection_pair->src_port,
        connection_pair->pxy_host.c_str(), connection_pair->pxy_port,
        connection_pair->dst_host.c_str(), connection_pair->dst_port,
        description
    );
}

bool SslProxy::on_connect(BoostNet::TcpConnectionSharedPtr connection, const void * identity)
{
    connection_pair_t * connection_pair = const_cast<connection_pair_t *>(reinterpret_cast<const connection_pair_t *>(identity));
    if (nullptr == connection_pair)
    {
        return (false);
    }

    if (!!connection)
    {
        connection->get_peer_address(connection_pair->dst_host, connection_pair->dst_port);
        record_connection(connection_pair, "connect");
        connection->set_user_data(connection_pair);
        connection_pair->dst = connection;
        return (on_recv(connection_pair->src));
    }
    else
    {
        BoostNet::TcpConnectionSharedPtr src_connection = connection_pair->src;
        if (!!src_connection)
        {
            src_connection->close();
        }
        return (false);
    }
}

bool SslProxy::on_accept(BoostNet::TcpConnectionSharedPtr connection, unsigned short listener_port)
{
    connection_pair_t * connection_pair = new connection_pair_t;
    connection_pair->src = connection;
    connection->get_peer_address(connection_pair->src_host, connection_pair->src_port);
    connection->get_host_address(connection_pair->pxy_host, connection_pair->pxy_port);
    connection_pair->dst_port = 0;
    if (m_tcp_manager.create_connection(m_dst_host, m_dst_port, false, connection_pair))
    {
        connection->set_user_data(connection_pair);
        return (true);
    }
    else
    {
        delete connection_pair;
        return (false);
    }
}

bool SslProxy::on_recv(BoostNet::TcpConnectionSharedPtr connection)
{
    if (!connection)
    {
        return (false);
    }

    connection_pair_t * connection_pair = reinterpret_cast<connection_pair_t *>(connection->get_user_data());
    if (nullptr == connection_pair)
    {
        return (false);
    }

    BoostNet::TcpConnectionSharedPtr recv_connection;
    BoostNet::TcpConnectionSharedPtr send_connection;
    if (connection_pair->src == connection)
    {
        recv_connection = connection_pair->src;
        send_connection = connection_pair->dst;
    }
    else if (connection_pair->dst == connection)
    {
        recv_connection = connection_pair->dst;
        send_connection = connection_pair->src;
    }
    else
    {
        return (false);
    }

    if (!send_connection)
    {
        return (true);
    }

    const void * data = recv_connection->recv_buffer_data();
    size_t size = recv_connection->recv_buffer_size();

    if (!send_connection->send_buffer_fill(data, size))
    {
        return (false);
    }

    if (!recv_connection->recv_buffer_drop(size))
    {
        return (false);
    }

    return (true);
}

bool SslProxy::on_send(BoostNet::TcpConnectionSharedPtr connection)
{
    return (true);
}

void SslProxy::on_close(BoostNet::TcpConnectionSharedPtr connection)
{
    if (!connection)
    {
        return;
    }

    connection_pair_t * connection_pair = reinterpret_cast<connection_pair_t *>(connection->get_user_data());
    if (nullptr == connection_pair)
    {
        return;
    }

    bool need_release = false;

    {
        std::lock_guard<std::mutex> locker(connection_pair->mtx);

        if (connection_pair->src == connection)
        {
            connection_pair->src.reset();
            BoostNet::TcpConnectionSharedPtr dst_connection = connection_pair->dst;
            if (!!dst_connection)
            {
                dst_connection->close();
                record_connection(connection_pair, "disconnect");
            }
            else
            {
                need_release = true;
            }
        }
        else if (connection_pair->dst == connection)
        {
            connection_pair->dst.reset();
            BoostNet::TcpConnectionSharedPtr src_connection = connection_pair->src;
            if (!!src_connection)
            {
                src_connection->close();
                record_connection(connection_pair, "disconnect");
            }
            else
            {
                need_release = true;
            }
        }
    }

    if (need_release)
    {
        record_connection(connection_pair, "release");
        delete connection_pair;
    }
}

#ifndef _MSC_VER
#include <unistd.h>
#include <signal.h>

static bool s_running = true;

static void signal_handler(int signo)
{
    s_running = false;
}
#endif // _MSC_VER

int ssl_proxy_main(int argc, char * argv[])
{
    if (argc < 6)
    {
        std::cout << "usage: " << argv[0] << " <listen-port> <forward-host> <forward-port> <cert-file> <key-file>" << std::endl;
        return (-1);
    }

    proxy_config_t proxy_config;
    proxy_config.src_port = atoi(argv[1]);
    proxy_config.dst_host = argv[2];
    proxy_config.dst_port = atoi(argv[3]);
    proxy_config.crt_file = argv[4];
    proxy_config.key_file = argv[5];

    SslProxy ssl_proxy;
    if (!ssl_proxy.init(proxy_config))
    {
        std::cout << "init ssl proxy failure" << std::endl;
        return (5);
    }

#ifdef _MSC_VER
    std::cout << "ssl proxy start success, input \"exit\" to stop it" << std::endl;

    while (true)
    {
        std::cin.sync();
        std::cin.clear();
        std::string command;
        std::cin >> command;
        if ("exit" == command)
        {
            break;
        }
    }
#else
    signal(SIGHUP, signal_handler);
    signal(SIGINT, signal_handler);
    signal(SIGQUIT, signal_handler);
    signal(SIGTERM, signal_handler);

    while (s_running)
    {
        sleep(1);
    }
#endif // _MSC_VER

    ssl_proxy.exit();

    return (0);
}
