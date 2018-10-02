/**
 * @file ChannelClient.h
 * @author  Pooh Cook
 * @version 1.0
 * @created September 18, 2018, 4:09 PM
 *
 * @section LICENSE
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details at
 * https://www.gnu.org/copyleft/gpl.html
 *
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

/**
 * @class NullHandler
 * @brief Empty handler class for instances when H (handler class is not provided for ChannelClient
 *
 */
template< class C, class R >
class NullHandler{
public:
    R process(int client_id, C command ){
       return R();
    }
};

/**
 * @class ChannelClient
 * @brief Connect to a Channel as a client
 *
 * provides a client connection to a channel instance created using ChannelProvider. The client can be constructed
 * specifying C (command) and R (response) types and then solely used as a send/receive mechanism or H (handler) can
 * also be specified to provide handling of of send operations initiated by the ClientProvider instance.
 * An integer client_id is required when constructing this class to identify the particular client connection to
 * the ClientProvider
 *
 * The handler class must possess a method of the signature: R process ( client_id, C command);
 *
 */
template< class C, class R, class H = NullHandler<C,R> >
class ChannelClient
{
private:
    boost::asio::io_service io_service;
    std::thread worker_thread;
    boost::asio::io_service::work* working_ = NULL;
    ChannelClientSession<C,R,H> session_;

public:
    /**
     * Constructor for connecting to a ClientProvider instance at a specified address adn port number
     *
     * @param client_id specifies the client_id to be used to identify this client when connecting
     * @param port specifies the port number of the channel
     * @param address specifies the ip address of the channel. If omitted, then "127.0.0.1" (home) is assumed
     * @param initial_command_id each channel message is given a sequentially increasing command id starting at this number
     *          default is 1000
     * @param buffer_size defines the size of the channel buffers for TCP messages.  This is the max serialization size
     *          for the command or response object. The serialization size is determined by boost serialization libraries.
     *          default is 4096 chars
     *
     */
    ChannelClient(int client_id, short port, string address = "127.0.0.1", int initial_command_id = 1000, int buffer_size = DEFAULT_TCP_SESSION_BUFFER_SIZE)
    : io_service(), session_( io_service, client_id, port, address , initial_command_id, buffer_size) {

        working_ = new boost::asio::io_service::work(io_service);
        worker_thread = std::thread(&ChannelClient<C,R,H>::run_io, this);
    }

    ~ChannelClient(){
        close();
    }

    /**
     * Closes the close the client and shutdown the io_service
     *
     */
    void close () {
        session_.close();
        if( working_ != NULL ){
            delete working_;
            working_ = NULL;
            if( worker_thread.joinable() )worker_thread.join();
        }
    }



    /**
     * Synchronous send and receive method
     *
     * Command is serialized and sent to the ChannelProvider and then a Serialized response is awaited. Response is then
     * deserialized and returned to the caller.
     *
     * @param command is a class of type C (command) as defined in the templated instance
     *
     * @return a class of type R (response) as defined by the templated instance
     *
     */
    R send( C command ){
        return session_.send(command);
    }

private:
    void run_io(){
        io_service.run();
    }

};


#endif // ChannelClient_H