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
    std::string write_buffer_;

    int attached_client_id_ = 0;
    TcpServer<C,R,H>* parent_server;

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
            throw boost::system::system_error(error);
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
            boost::bind(&TcpSession<C,R,H>::handleRead, this,
                boost::asio::placeholders::error,
                boost::asio::placeholders::bytes_transferred));

        return true;
    }

    int attachedClientId(){
        return attached_client_id_;
    }

private:
    void handleRead(const boost::system::error_code& error, size_t bytes_transferred){
        if (!error){
            read_buffer_[bytes_transferred] = '\0';

            std::stringstream sr;
            sr << read_buffer_.data();
            boost::archive::text_iarchive ia(sr);
            CommandHeader header;
            C command;

            ia >> header;
            ia >> command;

            if( header.type == MESSAGE_TYPE_COMMAND){
                handle_command(header, command);
            }
        }
        else{
            if( parent_server != NULL ) parent_server->detach(this);
            delete this;
        }
    }

    void handleWrite(const boost::system::error_code& error){
        if (!error){
            socket_.async_read_some(boost::asio::buffer(read_buffer_, read_buffer_.size()),
                boost::bind(&TcpSession<C,R,H>::handleRead, this,
                    boost::asio::placeholders::error,
                    boost::asio::placeholders::bytes_transferred));
        }
        else{
            if( parent_server != NULL ) parent_server->detach(this);
            delete this;
        }
    }

    void handle_command(CommandHeader header, C command ){

        ///  handler class must pocess this function
        R response = H::process( command);

        std::stringstream ss;
        boost::archive::text_oarchive oa(ss);
        header.type = MESSAGE_TYPE_RESPONSE;
        oa << header;
        oa << response;

        write_buffer_ = ss.str();

        boost::asio::async_write(socket_,  boost::asio::buffer(write_buffer_),
            boost::bind(&TcpSession<C,R,H>::handleWrite, this,
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

};


#endif // TcpServer_H
