/**
 * @file ChannelProviderSession.h
 * @author  Pooh Cook
 * @version 1.0
 * @created September 21, 2018, 3:39 PM
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

// forward declaration to allow provider session to call back to provider for authorization 
template< class C, class R, class H > class ChannelProvider;


/**
 * @class ChannelProviderSession
 * @brief A session that a ChannelClientSession can connect to 
 *
 * This class is used by ChannelProvider and is not intended for direct consumer usage.
 * The class provides a provider session that can be connected to by  a channel client instance. the session is created 
 * specifying C (command), R (response), and H (handler) types and provides for synchronous send / receive as well as
 * handling of of send operations initiated by the ClientClient instance.
 *
 */
template< class C, class R, class H >
class ChannelProviderSession: public ChannelSendProcessor<C,R,H>
{
private:

    std::vector<char> read_buffer_;
    int attached_client_id_ = 0;
    ChannelProvider<C,R,H>* parent_server;

public:
    /**
     * Constructor for creating a ClientProviderSession instance 
     *
     * @param io_service reference to an instance of an asio io service object
     * @param initial_command_id each channel message is given a sequentially increasing command id starting at this number
     *          default is 1000
     * @param buffer_size defines the size of the channel buffers for TCP messages.  This is the max serialization size
     *          for the command or response object. The serialization size is determined by boost serialization libraries.
     *          default is 4096 chars
     * @param parent pointer to parent ChannelProvider. This is used to perform authorization upon client connection
     *
     */
    ChannelProviderSession(boost::asio::io_service& io_service, int initial_command_id = 1000,
        int buffer_size = DEFAULT_TCP_SESSION_BUFFER_SIZE, ChannelProvider<C,R,H>* parent =  NULL)
        : ChannelSendProcessor<C,R,H>(io_service, initial_command_id, buffer_size ),
          read_buffer_(buffer_size), parent_server(parent){
    }

    tcp::socket& socket(){
        return ChannelSendProcessor<C,R,H>::socket_;
    }

    /**
     * @brief Initiate session processing
     *
     * this function initiates the attachment of a session. It begins a read of the channel which will receive the
     * initial connection message from the channel. There is no time out for this read so the receive operation will wait
     * indefinitely for a client connection to occur. (or until the originating ClientProvider to be destroyed.
     *
     * @return bool value is true if a client has successfully connected or false if a connection attempt has failed
     *
     */
    bool startSession(){

        boost::system::error_code error;
        size_t read_length = ChannelSendProcessor<C,R,H>::socket_.read_some(boost::asio::buffer(read_buffer_), error);
        if (error){
            return false;
        }

        std::stringstream sr;
        read_buffer_[read_length] = '\0';
        sr << read_buffer_.data();
        boost::archive::text_iarchive ia(sr);
        ChannelMessageHeader header;

        ia >> header;

        if( header.type != MESSAGE_TYPE_ATTACH){
            return false;
        }

        if( parent_server != NULL && !parent_server->authorise_attach(header.id, this)){
            return false;
        }

        attached_client_id_ = header.id;

        std::stringstream ss;
        boost::archive::text_oarchive oa(ss);
        header.type = MESSAGE_TYPE_ATTACHED;

        oa << header ;
        ss << ";";

        ChannelSendProcessor<C,R,H>::socket_.write_some(boost::asio::buffer(ss.str()), error);
        if (error){
            return false;
        }

        ChannelSendProcessor<C,R,H>::startReceiving();

        return true;
    }

    /**
     * @brief get attached client id
     *
     *
     * @return client id of the attached client if a client has successfully connected or 0 if not client is connected
     *
     */
    int attachedClientId(){
        return attached_client_id_;
    }

protected:
    // override of virtual base class method to handle comm errors.  An error in the base class class
    // (such as EOF or session reset ) is handled by delete this session and informing the parent to remove it
    void handle_comm_error( const boost::system::error_code& error ){

        LogMessage( error.message() );

        if( parent_server != NULL ) parent_server->detach(this);
        delete this;

    }



};


#endif // ChannelSession_H
