/*
 * File:   ChannelProvider.h
 * Author: Pooh
 *
 * Created on September 18, 2018, 4:09 PM
 */

#ifndef ChannelProvider_H
#define ChannelProvider_H

#include <iostream>
#include <string>
#include <thread>
#include <sstream>
#include <exception>
#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/text_oarchive.hpp>
#include <boost/bind.hpp>
#include <boost/asio.hpp>
#include <boost/array.hpp>
#include "TcpServer.h"

using namespace std;


template< class C, class R, class H >
class ChannelProvider
{

private:
    boost::asio::io_service io_service;
    TcpServer<C,R,H> server_;
    std::thread worker_thread;
    int buffer_size_;

    void run_io(){
        io_service.run();
    }

public:
    ChannelProvider(short port, int buffer_size = DEFAULT_TCP_SESSION_BUFFER_SIZE)
        : server_(io_service, port),
          buffer_size_(buffer_size) {

        worker_thread = std::thread(&ChannelProvider<C,R,H>::run_io, this);

    }

    ~ChannelProvider(){
        server_.close();
        worker_thread.join();
    }

    vector<int> attachedSessionIds(){
        return server_.attachedSessionIds();

    }

    void whitelist(int id){
        server_.whitelist(id);

    }

    R send( int client_id, C command){
        return server_.send( client_id, command);

    }



};


#endif // ChannelProvider_H