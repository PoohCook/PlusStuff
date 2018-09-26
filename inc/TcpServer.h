/*
 * File:   TcpServer.h
 * Author: Pooh
 *
 * Created on September 21, 2018, 3:39 PM
 */

#ifndef TcpServer_H
#define TcpServer_H

#include <iostream>
#include <string>
#include <sstream>
#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/text_oarchive.hpp>
#include <boost/bind.hpp>
#include <boost/asio.hpp>


using namespace std;
using boost::asio::ip::tcp;

#define DEFAULT_TCP_SESSION_BUFFER_SIZE  1024

typedef enum{
    MESSAGE_TYPE_WAITING = 0,
    MESSAGE_TYPE_COMMAND,
    MESSAGE_TYPE_RESPONSE,
    MESSAGE_TYPE_ATTACH,
    MESSAGE_TYPE_ATTACHED
} MessageType;

class CommandHeader{
public:
    MessageType type;
    int id;

    CommandHeader(){
        type = MESSAGE_TYPE_WAITING;
        id = 0;
    }

    CommandHeader( MessageType type, int id){
        this->type = type;
        this->id = id;
    }

    template<class Archive>
    void serialize(Archive & ar, const unsigned int version)
    {
        ar & type;
        ar & id;
    }

};

template< class C, class R, class H > class TcpServer;


template< class C, class R, class H >
class TcpSession: H
{
private:
    tcp::socket socket_;
    std::vector<char> read_buffer_;

    mutex response_mutex;
    condition_variable response_available;

    int attached_client_id_ = 0;
    TcpServer<C,R,H>* parent_server;

    int command_id_ = 5000;
    CommandHeader rcv_header_;
    R rcv_response_;
    C rcv_command_;

public:
    TcpSession(boost::asio::io_service& io_service, int buffer_size = DEFAULT_TCP_SESSION_BUFFER_SIZE, TcpServer<C,R,H>* parent =  NULL)
        : socket_(io_service), read_buffer_(buffer_size), parent_server(parent){
    }

    tcp::socket& socket(){
        return socket_;
    }

    bool startSession(){

        boost::system::error_code error;
        size_t rlen = socket_.read_some(boost::asio::buffer(read_buffer_), error);
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

        socket_.write_some(boost::asio::buffer(ss.str()), error);
        if (error){
            return false;
        }

        socket_.async_read_some( boost::asio::buffer(read_buffer_, read_buffer_.size()),
            boost::bind(&TcpSession<C,R,H>::handle_read, this,
                boost::asio::placeholders::error,
                boost::asio::placeholders::bytes_transferred));

        return true;
    }

    int attachedClientId(){
        return attached_client_id_;
    }

    R send( C command){

        int command_id = command_id_++;
        CommandHeader header(MESSAGE_TYPE_COMMAND, command_id);

        std::stringstream ss;
        boost::archive::text_oarchive oa(ss);

        oa << header;
        oa << command ;

        boost::asio::async_write(socket_,  boost::asio::buffer(ss.str()),
            boost::bind(&TcpSession<C,R,H>::handle_write, this,
                boost::asio::placeholders::error));

        //  await response
        unique_lock<mutex> lock(response_mutex);
        rcv_header_ = CommandHeader();
        response_available.wait(lock, [this]{return rcv_header_.type == MESSAGE_TYPE_RESPONSE;});

        if(  rcv_header_.type != MESSAGE_TYPE_RESPONSE ){
            throw std::runtime_error("wrong messge type recieved on response");
        }

        if( command_id != rcv_header_.id){
            throw std::runtime_error("wrong commnad id returned (" + to_string(command_id) + ") expected (" + to_string(rcv_header_.id) + ") returned");
        }

        return rcv_response_;

    }


private:
    void handle_read(const boost::system::error_code& error, size_t bytes_transferred){
        if (!error){

            {
                lock_guard<mutex> lock(response_mutex);

                read_buffer_[bytes_transferred] = '\0';

                std::stringstream sr;
                sr << read_buffer_.data();
                boost::archive::text_iarchive ia(sr);

                ia >> rcv_header_;
                if( rcv_header_.type == MESSAGE_TYPE_RESPONSE){
                    ia >> rcv_response_;
                }
                else if( rcv_header_.type == MESSAGE_TYPE_COMMAND){
                    ia >> rcv_command_;
                }

            }

            if( rcv_header_.type == MESSAGE_TYPE_RESPONSE){
                response_available.notify_all();
            }

            else if( rcv_header_.type == MESSAGE_TYPE_COMMAND){
                handle_command();
            }

            socket_.async_read_some(boost::asio::buffer(read_buffer_, read_buffer_.size()),
                boost::bind(&TcpSession<C,R,H>::handle_read, this,
                    boost::asio::placeholders::error,
                    boost::asio::placeholders::bytes_transferred));


        }
        else{
            if( parent_server != NULL ) parent_server->detach(this);
            delete this;
        }
    }

    void handle_write(const boost::system::error_code& error){
        if (error){
            if( parent_server != NULL ) parent_server->detach(this);
            delete this;
        }
    }

    void handle_command( ){

        ///  handler class must pocess this function
        R response = H::process( rcv_command_);

        std::stringstream ss;
        boost::archive::text_oarchive oa(ss);
        rcv_header_.type = MESSAGE_TYPE_RESPONSE;
        oa << rcv_header_;
        oa << response;

        boost::asio::async_write(socket_,  boost::asio::buffer(ss.str()),
            boost::bind(&TcpSession<C,R,H>::handle_write, this,
                boost::asio::placeholders::error));

    }

};


template< class C, class R, class H >
class TcpServer
{
    friend class TcpSession<C,R,H>;
private:
    boost::asio::io_service& io_service;
    tcp::acceptor acceptor_;
    int buffer_size_;
    vector<TcpSession<C,R,H>*> attached_sessions;
    vector<int>whitelist_ ;

public:
    TcpServer(boost::asio::io_service& io, short port, int buffer_size = DEFAULT_TCP_SESSION_BUFFER_SIZE)
        : io_service(io),
        acceptor_(io, tcp::endpoint(tcp::v4(), port)),
        buffer_size_ (buffer_size) {

        spawn_new_session();
    }

    void close(){
        acceptor_.cancel();
        acceptor_.close();
        for( auto it = attached_sessions.begin() ; it != attached_sessions.end() ; it++ ){
            delete *it;
        }
    }

    vector<int> attachedSessionIds(){
        vector<int> attached_ids;
        for ( auto it = attached_sessions.begin() ; it != attached_sessions.end() ; it++ ){
            attached_ids.push_back( (*it)->attachedClientId() );
        }

        return attached_ids;
    }

    void whitelist( int id){
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
        TcpSession<C,R,H>* new_session = new TcpSession<C,R,H>(io_service, buffer_size_, this);
        acceptor_.async_accept(new_session->socket(),
            boost::bind(&TcpServer::handle_accept, this, new_session,
                boost::asio::placeholders::error));

    }

    void handle_accept(TcpSession<C,R,H>* new_session, const boost::system::error_code& error) {

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
    
    void detach(TcpSession<C,R,H>* session){

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

    TcpSession<C,R,H>* get_attached_session( int id){

        TcpSession<C,R,H>* session = NULL;
        for ( auto it = attached_sessions.begin() ; it != attached_sessions.end(); it++){
            if( (*it)->attachedClientId() == id){
                session = *it;
                break;
            }
        }

        return session;
    }

};


#endif // TcpServer_H
