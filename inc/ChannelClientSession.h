/**
 * @file ChannelClientSession.h
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


/**
 * @class ChannelClientSession
 * @brief Connect to a Channel as a client
 *
 * This class is used by ChannelClient and is not intended for direct consumer usage.
 * The class provides a client session to a channel instance created using ChannelProvider. The client is constructed
 * specifying C (command), R (response), and H (handler) types and provides for synchronous send / recieve as well as
 * handling of of send operations initiated by the ClientProvider instance.
 *
 */
template< class C, class R, class H >
class ChannelClientSession : public ChannelSendProcessor<C,R,H>
{

private:

    int buffer_size_;


public:
    /**
     * Constructor for connecting to a ClientProvider instance at a specified address and port number
     *
     * @param io_service reference to an instance of an asio io service object
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
    ChannelClientSession(boost::asio::io_service& io_service, int client_id, short port, string address = "127.0.0.1",
        int initial_command_id = 1000, int buffer_size = DEFAULT_TCP_SESSION_BUFFER_SIZE)
            : ChannelSendProcessor<C,R,H>(io_service, initial_command_id, buffer_size),
          buffer_size_(buffer_size) {

        ChannelSendProcessor<C,R,H>::set_attached_client_id(client_id);

        tcp::resolver resolver(io_service);
        tcp::resolver::results_type endpoints =
          resolver.resolve(address, to_string(port));


        boost::asio::connect(ChannelSendProcessor<C,R,H>::socket(), endpoints);
        if( attach_socket(client_id) ){
            ChannelSendProcessor<C,R,H>::startReceiving();
        }

    }

private:
    bool attach_socket(int client_id){
         std::stringstream ss;
        boost::archive::text_oarchive oa(ss);
        ChannelMessageHeader header(MESSAGE_TYPE_ATTACH, client_id);

        oa << header ;
        ss << ";";

        boost::system::error_code error;
        ChannelSendProcessor<C,R,H>::socket().write_some(boost::asio::buffer(ss.str()), error);
        if (error){
            throw boost::system::system_error(error);
        }

        std::vector<char> buf(buffer_size_);
        size_t rlen = ChannelSendProcessor<C,R,H>::socket().read_some(boost::asio::buffer(buf), error);
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



};


#endif // ChannelClientSession_H