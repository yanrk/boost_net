# boost_net

*boost_net is a simple tcp/udp wrapper with C++11 while base on boost.asio*



## build

1. download boost (version 1.70.0 at least) from www.boost.org and build it first
2. download boost_net, go to folder *sln*, change the boost include and library path of file *boost_net.vcxproj* / *Makefile_a* / *Makefile_so*
3. for **Windows**, open *boost_net.sln* with **virtual studio 2017**, build it
4. for **Linux** / **OS X**, run command: `make -f Makefile_a`  for static library, or `make -f Makefile_so` for dynamic library
5. the files *boost_net.h*, *boost_net.lib*, *boost_net.dll* are what we need



## usage

the file *test/test_service.h* has show you how to use it, and a quick explanation here

1. the first thing is include the head file and library

   ```c++
   #include "boost_net.h"
   ```

2. then you need to define a class to inherit from **TcpServiceBase** if you need handle tcp connections, and inherit from **UdpServiceBase** if you need handle udp connections, at this sample, we need both tcp and udp，so we need to implement the pure virtual functions of the base classes

   ```c++
   class TestService : public BoostNet::TcpServiceBase, public BoostNet::UdpServiceBase
   {
       ...
   private: /* TcpServiceBase */
       virtual bool on_connect(BoostNet::TcpConnectionSharedPtr connection, std::size_t identity) override;
       virtual bool on_accept(BoostNet::TcpConnectionSharedPtr connection, unsigned short listener_port) override;
       virtual bool on_recv(BoostNet::TcpConnectionSharedPtr connection) override;
       virtual bool on_send(BoostNet::TcpConnectionSharedPtr connection) override;
       virtual void on_close(BoostNet::TcpConnectionSharedPtr connection) override;
       
   private: /* UdpServiceBase */
       virtual bool on_connect(BoostNet::UdpConnectionSharedPtr connection, std::size_t identity) override;
       virtual bool on_accept(BoostNet::UdpConnectionSharedPtr connection, unsigned short listener_port) override;
       virtual bool on_recv(BoostNet::UdpConnectionSharedPtr connection) override;
       virtual bool on_send(BoostNet::UdpConnectionSharedPtr connection) override;
       virtual void on_close(BoostNet::UdpConnectionSharedPtr connection) override;
       ...
   }
   ```

3. we define the **TcpManager** and **UdpManager** instances to establish connections and send / receive data 

   ```c++
   class TestService : public BoostNet::TcpServiceBase, public BoostNet::UdpServiceBase
   {
       ...
   private:
       BoostNet::TcpManager                                        m_tcp_manager;
       BoostNet::UdpManager                                        m_udp_manager;
   }
   ```

4. we define *init()* and *exit()* for start and stop the test service

   ```c++
   class TestService : public BoostNet::TcpServiceBase, public BoostNet::UdpServiceBase
   {
       ...
   public:
       bool init();
       void exit();
       ...
   }
   ```

   - if the **TestService**  is a **server** (others will connect to its tcp ports, or send to its udp ports actively)

     ```c++
     bool TestService::init()
     {
         unsigned short tcp_port_array[] = { 10001, 10002 };
         std::size_t tcp_port_count = sizeof(tcp_port_array) / sizeof(tcp_port_array[0]);
         if (!m_tcp_manager.init(this, 5, tcp_port_array, tcp_port_count))
         {
             return (false);
         }
     
         unsigned short udp_port_array[] = { 20001, 20002 };
         std::size_t udp_port_count = sizeof(udp_port_array) / sizeof(udp_port_array[0]);
         if (!m_udp_manager.init(this, 5, udp_port_array, udp_port_count))
         {
             return (false);
         }
                 
         return (true);
     }
     ```

   - if the **TestService** is a pure **client** (only connect to other's tcp ports, and send to other's udp ports actively)

     ```c++
     bool TestService::init()
     {
         if (!m_tcp_manager.init(this, 5))
         {
             return (false);
         }
     
         if (!m_udp_manager.init(this, 5))
         {
             return (false);
         }
                 
         return (true);
     }
     ```
   
   - easy to connect the other server

     ```c++
    {
         const char * tcp_host = "172.16.4.33";
         bool tcp_sync_connect = false;
         
         unsigned short tcp_port_1 = 9001;
         std::size_t tcp_identity_1 = 11111; /* when you need to identify the connection */
         if (!m_tcp_manager.create_connection(tcp_host, tcp_port_1, tcp_sync_connect, tcp_identity_1))
         {
             return (false);
         }
     
         unsigned short tcp_port_2 = 9002;
         std::size_t tcp_identity_2 = 22222; /* when you need to identify the connection */
         if (!m_tcp_manager.create_connection(tcp_host, tcp_port_2, tcp_sync_connect, tcp_identity_2))
         {
             return (false);
         }
         
         const char * udp_host = "192.168.1.113";
         bool udp_sync_connect = false;
         
         unsigned short udp_port_1 = 9011;
         std::size_t udp_identity_1 = 33333; /* when you need to identify the connection */
         if (!m_udp_manager.create_connection(udp_host, udp_port_1, udp_sync_connect, udp_identity_1))
         {
             return (false);
         }
         
         unsigned short udp_port_2 = 9012;
         std::size_t udp_identity_2 = 44444; /* when you need to identify the connection */
         if (!m_udp_manager.create_connection(udp_host, udp_port_2, udp_sync_connect, udp_identity_2))
         {
             return (false);
         }
         
         return (true);
     }
     ```
   
   - easy to stop **TestService**

     ```c++
     void TestService::exit()
     {
         m_tcp_manager.exit();
         m_udp_manager.exit();
     }
     ```
   
5. when **client** active-connect complete , **on_connect**(*connection*, *identity*) will callback, *identity* identify which connect-operation, if *connection* is *nullptr* means connect failed, **note** that the **return value is very important**, if return false means that the current connection is abandoned, which is equivalent to calling *connection->close()* implicitly

   ```c++
   bool TestService::on_connect(BoostNet::TcpConnectionSharedPtr connection, std::size_t identity)
   {
       if (!connection)
       {
           /* maybe we want retry, async-connect is better than sync-connect at here */
           if (11111 == identity) /* the first tcp connect operate failed */
           {
               m_tcp_manager.create_connection("172.16.4.33", 9001, false, 11111); 
           }
           else if (22222 == identity) /* the second tcp connect operate failed */
           {
               m_tcp_manager.create_connection("172.16.4.33", 9002, false, 22222); 
           }
           else
           {
               assert(false); /* unknown connection, never come here */
           }
           return (false);
       }
       else
       {
           /* we can store identity as a user data, if we want */
           connection->set_user_data(reinterpret_cast<void *>(identity));
            
           /* also we can store connection with a member variable */
           if (11111 == identity) /* the first tcp connect operate success */
           {
               m_tcp_connection_1 = connection;
           }
           else if (22222 == identity) /* the second tcp connect operate success */
           {
               m_tcp_connection_2 = connection;
           }
           else
           {
               assert(false); /* unknown connection, never come here */
           }
           return (true);
       }
   }
   
   bool TestService::on_connect(BoostNet::UdpConnectionSharedPtr connection, std::size_t identity)
   {
       ... // handle the udp connection same as tcp connection callback
   }
   ```

6. when **TestService** is a **server**, and others connect to its bind-ports, **on_accept**(*connection*, *listener_port*) will callback, *listener_port* identify which port connect from, the *connection* never be *nullptr* here, **note** that the **return value is very important**, if return false means that the current connection is abandoned, which is equivalent to calling *connection->close()* implicitly

   ```c++
   bool TestService::on_accept(BoostNet::TcpConnectionSharedPtr connection, unsigned short listener_port)
   {
       assert(!!connection);
       /* maybe we need increase the number of connections on each listener port */
       if (10001 == listener_port)
       {
           ++m_tcp_port_count_1;
       }
       else if (10002 == listener_port)
       {
           ++m_tcp_port_count_2;
       }
       else
       {
           assert(false); /* unknown listener port, never come here */
       }
       return (true);
   }
   
   bool TestService::on_accept(BoostNet::UdpConnectionSharedPtr connection, unsigned short listener_port)
   {
       ... // handle the udp connection same as tcp connection callback
   }
   ```

7. when connection close, **on_close**(*connection*) will callback, it is the last callback of connection, the *connection* never be *nullptr* here

   - if **TestService** is **client** maybe we do like this

     ```c++
     void TestService::on_close(BoostNet::TcpConnectionSharedPtr connection)
     {
         assert(!!connection);
         /* we have stored identity as a user data before, now can load it */
         std::size_t identity = reinterpret_cast(connection->get_user_data());
         /* maybe we want to reconnect */
         if (11111 == identity) /* it is the first tcp connection close */
         {
             m_tcp_connection_1.reset();
             m_tcp_manager.create_connection("172.16.4.33", 9001, false, 11111); 
         }
         else if (22222 == identity) /* it is the second tcp connection close */
         {
             m_tcp_connection_2.reset();
             m_tcp_manager.create_connection("172.16.4.33", 9002, false, 22222); 
         }
         else
         {
             assert(false); /* unknown connection, never come here */
         }
     }
     
     void TestService::on_close(BoostNet::UdpConnectionSharedPtr connection)
     {
         /*
          * maybe we handle the udp connection same as tcp connection callback
          * but we handle it with another way
          */
         assert(!!connection);
         /* maybe we want to reconnect */
         if (connection == m_udp_connection_1) /* it is the first udp connection close */
         {
             m_udp_connection_1.reset();
             m_udp_manager.create_connection("192.168.1.113", 9011, false, 33333); 
         }
         else if (connection == m_udp_connection_2) /* it is the second udp connection close */
         {
             m_udp_connection_2.reset();
             m_udp_manager.create_connection("192.168.1.113", 9012, false, 44444); 
         }
         else
         {
             assert(false); /* unknown connection, never come here */
         }
     }
     ```
   
   - if **TestService** is **server** maybe we do like this
   
     ```c++
     void TestService::on_close(BoostNet::TcpConnectionSharedPtr connection)
     {
         assert(!!connection);
         /* maybe we need decrease the number of connections on each listener port */
         std::string host_ip;
         unsigned short host_port = 0;
         connection->get_host_address(host_ip, host_port);
         if (10001 == host_port)
         {
             --m_tcp_port_count_1;
         }
         else if (10002 == host_port)
         {
             --m_tcp_port_count_2;
         }
         else
         {
             assert(false); /* unknown listener port, never come here */
         }
     }
     
     void TestService::on_close(BoostNet::UdpConnectionSharedPtr connection)
     {
         ... // maybe we handle the udp connection same as tcp connection callback
     }
     ```
   
8. when received any new data, **on_recv**(*connection*) will callback, the *connection* never be *nullptr* here, **note** that the **return value is very important**, if return false means that the current connection is abandoned, which is equivalent to calling *connection->close()* implicitly

   ```c++
   bool TestService::on_recv(BoostNet::TcpConnectionSharedPtr connection)
   {
       /*
        * use ‘connection->recv_buffer_data()’ to get data point
        * use ‘connection->recv_buffer_size()’ to get data size
        * use ‘connection->recv_buffer_copy(buff, len)’ to copy data
        * use ‘connection->recv_buffer_drop(len)’ to drop data
        * use ‘connection->recv_buffer_move(buff, len)’ to copy data and drop it
        * use ‘connection->recv_buffer_water_mark(len)’ to reset watermark, default to 1
        * use ‘connection->send_buffer_fill(data, len)’ to send data
        */
       assert(!!connection);
       /* maybe we want just send it back here */
       const char * data = reinterpret_cast<const char *>(connection->recv_buffer_data());
       std::size_t size = connection->recv_buffer_size();
       if (!connection->send_buffer_fill(data, size))
       {
           return (false); // send failed, return false for notify to close it
       }
       if (!connection->recv_buffer_drop(size))
       {
           return (false); // something wrong, return false for notify to close it
       }
       return (true);
   }
   
   bool TestService::on_recv(BoostNet::UdpConnectionSharedPtr connection)
   {
       /*
        * use ‘connection->recv_buffer_has_data()’ to check whether has more frame data
        * use ‘connection->recv_buffer_data()’ to get the first frame data point
        * use ‘connection->recv_buffer_size()’ to get the first frame data size
        * use ‘connection->recv_buffer_drop()’ to drop the first frame data
        * use ‘connection->send_buffer_fill(data, len)’ to send frame data
        */
       assert(!!connection);
       /* maybe we want just send it back here */
       while (connection->recv_buffer_has_data())
       {
           const char * data = reinterpret_cast<const char *>(connection->recv_buffer_data());
           std::size_t size = connection->recv_buffer_size();
           if (!connection->send_buffer_fill(data, size))
           {
               return (false); // send failed, return false for notify to close it
           }
           if (!connection->recv_buffer_drop())
           {
               return (false); // something wrong, return false for notify to close it
           }
       }
       return (true);
   }
   ```
   
9. when all data has sent complete, **on_send**(*connection*) will callback, the *connection* never be *nullptr* here, **note** that the **return value is very important**, if return false means that the current connection is abandoned, which is equivalent to calling *connection->close()* implicitly, so mostly we just return true 

   ```c++
   bool TestService::on_send(BoostNet::TcpConnectionSharedPtr connection)
   {
       assert(!!connection);
       return (true);
   }
   
   bool TestService::on_send(BoostNet::UdpConnectionSharedPtr connection)
   {
       assert(!!connection);
       return (true);
   }
   ```

10. **note** that each **callback** for each connection is **blocked**, so don't do anything too time-consuming within the callback

11. **note** that each **callback** for each connection is **mutually exclusive**, so  we need not any mutex to protect it, but if we save the *connection* as a member variable and use it in non-callback functions (meaning other threads), pay attention to the usage of smart pointer member variable

    ```c++
    {
        /* 1, bad way to use smart pointer member variable */
        if (!!m_tcp_connection_1)
        {
            /*
             * maybe on_close(connection) called at the same time
             * and m_tcp_connection_1.reset() been called there
             * the next operate will get a bomb
             */
            m_tcp_connection_1->send_buffer_fill("hello", 5);
        }
        
        /* 2, right way to use smart pointer member variable */
        BoostNet::TcpConnectionSharedPtr connection = m_tcp_connection_1;
        if (!!connection)
        {
            connection->send_buffer_fill("hello", 5);
        } 
    }
    ```

    

