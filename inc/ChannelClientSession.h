/*
 * File:   ChannelClientSession.h
 * Author: Pooh
 *
 * Created on September 18, 2018, 4:09 PM
 */

#ifndef ChannelClientSession_H
#define ChannelClientSession_H

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
#include "ChannelSendProcessor.h"

using namespace std;


template< class C, class R, class H >
class ChannelClientSession : public ChannelSendProcessor<C,R,H>
{

private:

    int buffer_size_;
    std::vector<char> read_buffer_;


    bool attach_socket(int client_id){
         std::stringstream ss;
        boost::archive::text_oarchive oa(ss);
        CommandHeader header(MESSAGE_TYPE_ATTACH, client_id);

        oa << header ;
        ss << ";";

        boost::system::error_code error;
        ChannelSendProcessor<C,R,H>::socket_.write_some(boost::asio::buffer(ss.str()), error);
        if (error){
            throw boost::system::system_error(error);
        }

        std::vector<char> buf(buffer_size_);
        size_t rlen = ChannelSendProcessor<C,R,H>::socket_.read_some(boost::asio::buffer(buf), error);
        if (error){
            throw boost::system::system_error(error);
        }

        std::stringstream sr;
        buf[rlen] = '\0';
        sr << buf.data();
        boost::archive::text_iarchive ia(sr);

        ia >> header;
        if( header.type != MESSAGE_TYPE_ATTACHED){
            throw runtime_error("Session attach refused");
        }

        return true;
    }


public:
    ChannelClientSession(boost::asio::io_service& io_service, int client_id, short port, string address = "127.0.0.1",
        int initial_command_id = 1000, int buffer_size = DEFAULT_TCP_SESSION_BUFFER_SIZE)
            : ChannelSendProcessor<C,R,H>(io_service, initial_command_id, buffer_size),
          buffer_size_(buffer_size),
          read_buffer_(buffer_size)  {

        tcp::resolver resolver(io_service);
        tcp::resolver::results_type endpoints =
          resolver.resolve(address, to_string(port));


        boost::asio::connect(ChannelSendProcessor<C,R,H>::socket_, endpoints);
        if( attach_socket(client_id) ){
            ChannelSendProcessor<C,R,H>::startRecieving();
        }

    }

    void close(){
        ChannelSendProcessor<C,R,H>::socket_.cancel();
        ChannelSendProcessor<C,R,H>::socket_.close();
    }



};


#endif // ChannelClientSession_H