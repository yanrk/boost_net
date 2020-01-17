/********************************************************
 * Description : boost net
 * Data        : 2018-01-02 10:56:00
 * Author      : yanrk
 * Email       : yanrkchina@163.com
 * Blog        : blog.csdn.net/cxxmaker
 * Version     : 2.0
 * Copyright(C): 2018 - 2020
 ********************************************************/

#ifndef BOOST_NET_H
#define BOOST_NET_H


#include <cstddef>
#include <string>
#include <memory>

#ifdef _MSC_VER
    #define BOOST_NET_CDECL             __cdecl
    #define BOOST_NET_STDCALL           __stdcall
    #ifdef EXPORT_BOOST_NET_DLL
        #define BOOST_NET_API           __declspec(dllexport)
    #else
        #ifdef USE_BOOST_NET_DLL
            #define BOOST_NET_API       __declspec(dllimport)
        #else
            #define BOOST_NET_API
        #endif // USE_BOOST_NET_DLL
    #endif // EXPORT_BOOST_NET_DLL
#else
    #define BOOST_NET_CDECL
    #define BOOST_NET_STDCALL
    #define BOOST_NET_API
#endif // _MSC_VER

namespace BoostNet { // namespace BoostNet begin

class BOOST_NET_API TcpConnectionBase
{
public:
    TcpConnectionBase();
    virtual ~TcpConnectionBase();

public:
    void set_user_data(void * user_data);
    void * get_user_data();

public:
    virtual void get_host_address(std::string & ip, unsigned short & port) = 0;
    virtual void get_peer_address(std::string & ip, unsigned short & port) = 0;

public:
    virtual const void * recv_buffer_data() = 0;
    virtual std::size_t recv_buffer_size() = 0;
    virtual bool recv_buffer_copy_len(void * buf, std::size_t len) = 0;
    virtual bool recv_buffer_move_len(void * buf, std::size_t len) = 0;
    virtual bool recv_buffer_drop_len(std::size_t len) = 0;
    virtual void recv_buffer_water_mark(std::size_t len) = 0;
    virtual bool send_buffer_fill_len(const void * data, std::size_t len) = 0;

public:
    virtual void close() = 0;

private:
    void                                          * m_user_data;
};

typedef std::shared_ptr<TcpConnectionBase> TcpConnectionSharedPtr;
typedef std::weak_ptr<TcpConnectionBase> TcpConnectionWeakPtr;

class BOOST_NET_API TcpServiceBase
{
public:
    TcpServiceBase();
    virtual ~TcpServiceBase();

public:
    virtual bool on_connect(TcpConnectionSharedPtr connection, std::size_t identity) = 0;
    virtual bool on_accept(TcpConnectionSharedPtr connection, unsigned short listener_port) = 0;
    virtual bool on_recv(TcpConnectionSharedPtr connection) = 0;
    virtual bool on_send(TcpConnectionSharedPtr connection) = 0;
    virtual void on_close(TcpConnectionSharedPtr connection) = 0;
};

class TcpManagerImpl;

class BOOST_NET_API TcpManager
{
public:
    TcpManager();
    ~TcpManager();

public:
    TcpManager(const TcpManager &) = delete;
    TcpManager(TcpManager &&) = delete;
    TcpManager & operator = (const TcpManager &) = delete;
    TcpManager & operator = (TcpManager &&) = delete;

public:
    bool init(TcpServiceBase * tcp_service, std::size_t thread_count = 5, unsigned short * port_array = nullptr, std::size_t port_count = 0);
    void exit();

public:
    bool create_connection(const std::string & host, const std::string & service, bool sync_connect = true, std::size_t identity = 0, const char * bind_ip = "0.0.0.0", unsigned short bind_port = 0);
    bool create_connection(const std::string & host, unsigned short port, bool sync_connect = true, std::size_t identity = 0, const char * bind_ip = "0.0.0.0", unsigned short bind_port = 0);

private:
    TcpManagerImpl                                * m_manager_impl;
};

class BOOST_NET_API UdpConnectionBase
{
public:
    UdpConnectionBase();
    virtual ~UdpConnectionBase();

public:
    void set_user_data(void * user_data);
    void * get_user_data();

public:
    virtual void get_host_address(std::string & ip, unsigned short & port) = 0;
    virtual void get_peer_address(std::string & ip, unsigned short & port) = 0;

public:
    virtual bool recv_buffer_has_data() = 0;
    virtual const void * recv_buffer_data() = 0;
    virtual std::size_t recv_buffer_size() = 0;
    virtual bool recv_buffer_drop_len(std::size_t len) = 0;
    virtual bool send_buffer_fill_len(const void * data, std::size_t len) = 0;

public:
    virtual void close() = 0;

private:
    void                                          * m_user_data;
};

typedef std::shared_ptr<UdpConnectionBase> UdpConnectionSharedPtr;
typedef std::weak_ptr<UdpConnectionBase> UdpConnectionWeakPtr;

class BOOST_NET_API UdpServiceBase
{
public:
    UdpServiceBase();
    virtual ~UdpServiceBase();

public:
    virtual bool on_connect(UdpConnectionSharedPtr connection, std::size_t identity) = 0;
    virtual bool on_accept(UdpConnectionSharedPtr connection, unsigned short listener_port) = 0;
    virtual bool on_recv(UdpConnectionSharedPtr connection) = 0;
    virtual bool on_send(UdpConnectionSharedPtr connection) = 0;
    virtual void on_close(UdpConnectionSharedPtr connection) = 0;
};

class UdpManagerImpl;

class BOOST_NET_API UdpManager
{
public:
    UdpManager();
    ~UdpManager();

public:
    UdpManager(const UdpManager &) = delete;
    UdpManager & operator = (const UdpManager &) = delete;

public:
    bool init(UdpServiceBase * udp_service, std::size_t thread_count = 5, unsigned short * port_array = nullptr, std::size_t port_count = 0);
    void exit();

public:
    bool create_connection(const std::string & host, const std::string & service, bool sync_connect = true, std::size_t identity = 0, const char * bind_ip = "0.0.0.0", unsigned short bind_port = 0);
    bool create_connection(const std::string & host, unsigned short port, bool sync_connect = true, std::size_t identity = 0, const char * bind_ip = "0.0.0.0", unsigned short bind_port = 0);

private:
    UdpManagerImpl                                * m_manager_impl;
};

} // namespace BoostNet end


#endif // BOOST_NET_H
