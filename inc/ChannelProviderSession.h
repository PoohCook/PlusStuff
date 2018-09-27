/*
 * File:   ChannelProviderSession.h
 * Author: Pooh
 *
 * Created on September 21, 2018, 3:39 PM
 */

#ifndef ChannelSession_H
#define ChannelSession_H

#include <iostream>
#include <string>
#include <sstream>
#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/text_oarchive.hpp>
#include <boost/bind.hpp>
#include <boost/asio.hpp>
#include "ChannelSendProcessor.h"


using namespace std;
using boost::asio::ip::tcp;

template< class C, class R, class H > class ChannelProvider;


template< class C, class R, class H >
class ChannelProviderSession: public ChannelSendProcessor<C,R,H>
{
private:

    std::vector<char> read_buffer_;
    int attached_client_id_ = 0;
    ChannelProvider<C,R,H>* parent_server;

public:
    ChannelProviderSession(boost::asio::io_service& io_service, int buffer_size = DEFAULT_TCP_SESSION_BUFFER_SIZE, ChannelProvider<C,R,H>* parent =  NULL)
        : ChannelSendProcessor<C,R,H>(io_service, buffer_size), read_buffer_(buffer_size), parent_server(parent){
    }

    tcp::socket& socket(){
        return ChannelSendProcessor<C,R,H>::socket_;
    }

    bool startSession(){

        boost::system::error_code error;
        size_t rlen = ChannelSendProcessor<C,R,H>::socket_.read_some(boost::asio::buffer(read_buffer_), error);
        if (error){
            return false;
        }

        std::stringstream sr;
        read_buffer_[rlen] = '\0';
        sr << read_buffer_.data();
        boost::archive::text_iarchive ia(sr);
        CommandHeader header;

        ia >> header;

        if( header.type != MESSAGE_TYPE_ATTACH){
            return false;
        }

        if( parent_server != NULL && !parent_server->authorise_attach(header.id)){
            return false;
        }

        attached_client_id_ = header.id;

        std::stringstream ss;
        boost::archive::text_oarchive oa(ss);
        header.type = MESSAGE_TYPE_ATTACHED;

        oa << header ;

        ChannelSendProcessor<C,R,H>::socket_.write_some(boost::asio::buffer(ss.str()), error);
        if (error){
            return false;
        }

        ChannelSendProcessor<C,R,H>::startRecieving();

        return true;
    }

    int attachedClientId(){
        return attached_client_id_;
    }

protected:
    void handle_comm_error( const boost::system::error_code& error ){

        //cout << error.message() << std::endl;


        if( parent_server != NULL ) parent_server->detach(this);
        delete this;

    }



};


#endif // ChannelSession_H
