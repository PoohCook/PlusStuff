/*
 * File:   ChannelClient.h
 * Author: Pooh
 *
 * Created on September 18, 2018, 4:09 PM
 */

#ifndef ChannelClient_H
#define ChannelClient_H

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
#include "ChannelClientSession.h"

using namespace std;


template< class C, class R >
class NullHandler{
public:
    R process(C command ){
       return R();
    }
};


template< class C, class R, class H = NullHandler<C,R> >
class ChannelClient
{

private:

    boost::asio::io_service io_service;
    boost::asio::io_service::work* working_ = NULL;
    std::thread worker_thread;
    ChannelClientSession<C,R,H> session_;

public:
    ChannelClient(int client_id, short port, string address = "127.0.0.1", int initial_client_id = 1000, int buffer_size = DEFAULT_TCP_SESSION_BUFFER_SIZE)
    : io_service(), session_( io_service, client_id, port, address , initial_client_id, buffer_size) {

        working_ = new boost::asio::io_service::work(io_service);
        worker_thread = std::thread(&ChannelClient<C,R,H>::run_io, this);
    }

    ~ChannelClient(){
        session_.close();
        if( working_ != NULL ){
            delete working_;
            worker_thread.join();

        }
    }

    R send( C command ){
        return session_.send(command);
    }

private:
    void run_io(){
        io_service.run();
    }


};


#endif // ChannelClient_H