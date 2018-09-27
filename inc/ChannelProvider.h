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
#include "ChannelProviderSession.h"

using namespace std;


template< class C, class R, class H >
class ChannelProvider
{
    friend class ChannelProviderSession<C,R,H>;

private:
    boost::asio::io_service io_service;
    tcp::acceptor acceptor_;
    int buffer_size_;
    vector<ChannelProviderSession<C,R,H>*> attached_sessions;
    vector<int>whitelist_ ;

    std::thread worker_thread;

    void run_io(){
        io_service.run();
    }

public:
    ChannelProvider(short port, int buffer_size = DEFAULT_TCP_SESSION_BUFFER_SIZE)
        : acceptor_(io_service, tcp::endpoint(tcp::v4(), port)),
        buffer_size_(buffer_size) {

        spawn_new_session();

        worker_thread = std::thread(&ChannelProvider<C,R,H>::run_io, this);

    }

    ~ChannelProvider(){
        acceptor_.cancel();
        acceptor_.close();
        for( auto it = attached_sessions.begin() ; it != attached_sessions.end() ; it++ ){
            delete *it;
        }
        worker_thread.join();
    }

    vector<int> attachedSessionIds(){
        vector<int> attached_ids;
        for ( auto it = attached_sessions.begin() ; it != attached_sessions.end() ; it++ ){
            attached_ids.push_back( (*it)->attachedClientId() );
        }

        return attached_ids;

    }

    void whitelist(int id){
        whitelist_.push_back(id);

    }

    R send( int client_id, C command){
        auto session = get_attached_session( client_id);
        if( session == NULL){
            throw runtime_error("unknown session for client_id " + to_string(client_id));
        }

        return session->send( command);

    }

private:

    void spawn_new_session(){
        ChannelProviderSession<C,R,H>* new_session = new ChannelProviderSession<C,R,H>(io_service, buffer_size_, this);
        acceptor_.async_accept(new_session->socket(),
            boost::bind(&ChannelProvider::handle_accept, this, new_session,
                boost::asio::placeholders::error));

    }

    void handle_accept(ChannelProviderSession<C,R,H>* new_session, const boost::system::error_code& error) {

        if (!error){
            if( new_session->startSession()){
                attached_sessions.push_back(new_session);
            }
            else{
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

    bool authorise_attach( int id){
        //  empty whitelist means all authorizations are valid
        if( whitelist_.empty() ) return true;

       return ( find( whitelist_.begin(), whitelist_.end(), id) != whitelist_.end());

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