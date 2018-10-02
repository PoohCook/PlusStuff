/**
 * @file ChannelProvider.h
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

#ifndef ChannelProvider_H
#define ChannelProvider_H

#include <iostream>
#include <string>
#include <thread>
#include <sstream>
#include <exception>
#include <boost/random/mersenne_twister.hpp>
#include <boost/random/uniform_int_distribution.hpp>
#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/text_oarchive.hpp>
#include <boost/bind.hpp>
#include <boost/asio.hpp>
#include <boost/array.hpp>
#include "ChannelProviderSession.h"

using namespace std;

/**
 * @class ChannelProvider
 * @brief Create a TCP Channel sever that can be connected to by ChannelClient instances
 *
 * provides a channel server instance that can be connected to by ChannelClient instances using TCP protocol.
 * The server is constructed specifying C (command), R (response), and H (handler) types and can be used to both
 * send/receive with a specific client and handle send/receive exchanges from clients.
 * An integer client_id is expected from clients as they connect. A whitelist can be setup on the ChannelProvider to
 * exclude un registered clients or accept any client if not setup.
 *
 * The handler class must possess a method of the signature: R process ( client_id, C command);

 */
template< class C, class R, class H >
class ChannelProvider
{
    // the provider session uses a private method to call back to the provider to do authorization and registration
    friend class ChannelProviderSession<C,R,H>;

private:
    boost::asio::io_service io_service;
    tcp::acceptor acceptor_;
    int process_timeout_seconds_;
    int initial_command_id_;
    int buffer_size_;

    vector<ChannelProviderSession<C,R,H>*> attached_sessions;
    vector<int>whitelist_ ;

    std::thread worker_thread;

    boost::random::mt19937 gen_;

public:
    /**
     * Constructor for creating a ClientProvider instance connected to a port number
     *
     * @param port specifies the port number of the channel
     * @param process_timeout_seconds number of seconds to wait before timing out a process attempt. Default is 10
     * @param initial_command_id each channel message is given a sequentially increasing command id starting at this number
     *          default is 4000
     * @param buffer_size defines the size of the channel buffers for TCP messages.  This is the max serialization size
     *          for the command or response object. The serialization size is determined by boost serialization libraries.
     *          default is 4096 chars
     *
     */
    ChannelProvider(short port, int process_timeout_seconds = 10, int initial_command_id = 4000, int buffer_size = DEFAULT_TCP_SESSION_BUFFER_SIZE)
        : acceptor_(io_service, tcp::endpoint(tcp::v4(), port)),
        process_timeout_seconds_(process_timeout_seconds), initial_command_id_(initial_command_id), buffer_size_(buffer_size) {

        spawn_new_session();

        worker_thread = std::thread(&ChannelProvider<C,R,H>::run_io, this);

    }

    ~ChannelProvider(){
        close();
    }

    /**
     * Closes the close the provider and shutdown the io_service
     *
     */
    void close () {
        acceptor_.close();
        for( auto it = attached_sessions.begin() ; it != attached_sessions.end() ; it++ ){
            delete *it;
        }
        attached_sessions.clear();

        if( worker_thread.joinable() ) worker_thread.join();

    }


    /**
     * returns a list currently attached client ids
     *
     * @return an std::vector<int> containing all currently attached client ids
     *
     */
    vector<int> attachedClientIds(){
        vector<int> attached_ids;
        for ( auto it = attached_sessions.begin() ; it != attached_sessions.end() ; it++ ){
            attached_ids.push_back( (*it)->attachedClientId() );
        }

        return attached_ids;

    }

    /**
     * Generates a random client id and adds it to the current whitelist
     *
     * Note: if the whitelist is empty, then all clients will be allowed ot connect. If any client_ids are generated for
     * the whitelist, then only those client_ids will be allowed to connect.
     *
     * @param client_id  the id of the client to be whitelisted
     *
     */
    int generateWhitelistId(){
        boost::random::uniform_int_distribution<int> dist;
        int newWhiteId = 0;

        while ( newWhiteId <= 0 ||
                find ( whitelist_.begin(), whitelist_.end(), newWhiteId) != whitelist_.end() ){

            newWhiteId = dist(gen_);
        }

        whitelist_.push_back(newWhiteId);
        return newWhiteId;
    }

    /**
     * Synchronous send and receive method
     *
     * Command is serialized and sent to the ChannelClient identified by client_id and then a Serialized response is
     * awaited. Response is then deserialized and returned to the caller.
     *
     * @param client_id is a the client id of the ChannelClient to be sent to
     * @param command is a class of type C (command) as defined in the templated instance
     *
     * @return a class of type R (response) as defined by the templated instance
     *
     */
    R send( int client_id, C command){
        auto session = get_attached_session( client_id);
        if( session == NULL){
            throw runtime_error("unknown session for client_id " + to_string(client_id));
        }

        return session->send( command);

    }

private:
    void run_io(){
        io_service.run();
    }

    void spawn_new_session(){
        ChannelProviderSession<C,R,H>* new_session =
            new ChannelProviderSession<C,R,H>(io_service, process_timeout_seconds_, initial_command_id_, buffer_size_, this);
        acceptor_.async_accept(new_session->socket(),
            boost::bind(&ChannelProvider::handle_accept, this, new_session,
                boost::asio::placeholders::error));

    }

    void handle_accept(ChannelProviderSession<C,R,H>* new_session, const boost::system::error_code& error) {
        if (!error){
            if( !new_session->startSession()){
                delete new_session;
            }

            spawn_new_session();
        }
        else{
            delete new_session;
        }
    }

    void detach(ChannelProviderSession<C,R,H>* session){

        auto to_detach = find(attached_sessions.begin(), attached_sessions.end(), session);

        if (to_detach != attached_sessions.end())
        {
            // session is multipurpose so let him delete himself
           //delete *to_detach ;

           attached_sessions.erase(to_detach);

        }

    }

    bool authorise_attach( int client_id, ChannelProviderSession<C,R,H>* session){
        //  empty whitelist means all authorizations are valid
       bool authorized =  whitelist_.empty() || ( find( whitelist_.begin(), whitelist_.end(), client_id) != whitelist_.end());

       if(authorized) attached_sessions.push_back(session);

       return authorized;

    }

    ChannelProviderSession<C,R,H>* get_attached_session( int id){

        ChannelProviderSession<C,R,H>* session = NULL;
        for ( auto it = attached_sessions.begin() ; it != attached_sessions.end(); it++){
            if( (*it)->attachedClientId() == id){
                session = *it;
                break;
            }
        }

        return session;
    }


};


#endif // ChannelProvider_H