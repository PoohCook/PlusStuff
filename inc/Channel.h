/*
 * File:   Channel.h
 * Author: Pooh
 *
 * Created on September 18, 2018, 4:09 PM
 */

#ifndef Channel_H
#define Channel_H

#include "TcpServer.h"
#include <iostream>
#include <string>
#include <thread>
#include <sstream>
#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/text_oarchive.hpp>
#include <boost/bind.hpp>
#include <boost/asio.hpp>
#include <boost/array.hpp>


using namespace std;



template< class C, class R, class H >
class ChannelProvider
{

private:
    boost::asio::io_service io_service;
    TcpServer<C,R,H> server_;
    std::thread worker_thread;
    int buffer_size_;

    void runIo(){
        io_service.run();
    }

public:
    ChannelProvider(short port, int buffer_size = DEFAULT_TCP_SESSION_BUFFER_SIZE)
        : server_(io_service, port),
          buffer_size_(buffer_size) {

        worker_thread = std::thread(&ChannelProvider<C,R,H>::runIo, this);

    }

    ~ChannelProvider(){

        server_.close();
        worker_thread.join();

    }


};


template< class C, class R >
class ChannelClient
{

private:

    boost::asio::io_service io_service;
    tcp::socket socket_;
    int buffer_size_;

public:
    ChannelClient(short port, string address = "127.0.0.1", int buffer_size = DEFAULT_TCP_SESSION_BUFFER_SIZE)
        : socket_(io_service),
          buffer_size_(buffer_size)  {

        tcp::resolver resolver(io_service);
        tcp::resolver::results_type endpoints =
          resolver.resolve(address, to_string(port));

        boost::asio::connect(socket_, endpoints);

    }

    ~ChannelClient(){

        socket_.cancel();
        socket_.close();
    }


    R send( C command){

        std::stringstream ss;
        boost::archive::text_oarchive oa(ss);
        oa << command ;

        boost::system::error_code error;
        socket_.write_some(boost::asio::buffer(ss.str()), error);
        if (error){
            throw boost::system::system_error(error);
        }

        std::vector<char> buf(buffer_size_);
        size_t rlen = socket_.read_some(boost::asio::buffer(buf), error);
        if (error && error != boost::asio::error::eof ){
            throw boost::system::system_error(error);
        }

        std::stringstream sr;
        buf[rlen] = '\0';
        sr << buf.data();
        boost::archive::text_iarchive ia(sr);
        R return_val;
        ia >> return_val;

        return return_val;

    }

};



#endif // Channel_H