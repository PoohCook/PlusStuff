/*
 * File:   TcpSession.h
 * Author: Pooh
 *
 * Created on September 21, 2018, 3:39 PM
 */

#ifndef TcpSession_H
#define TcpSession_H

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



template< class C, class R, class H >
class TcpSendProcesor: H
{
protected:
    tcp::socket socket_;

private:
    std::vector<char> read_buffer_;
    mutex response_mutex;
    condition_variable response_available;

    int command_id_ = 5000;
    CommandHeader rcv_response_header_;
    R rcv_response_;

    CommandHeader rcv_command_header_;
    C rcv_command_;


public:
    R send( C command){

        int command_id = command_id_++;
        CommandHeader header(MESSAGE_TYPE_COMMAND, command_id);

        std::stringstream ss;
        boost::archive::text_oarchive oa(ss);

        oa << header;
        oa << command ;

        boost::asio::async_write(socket_,  boost::asio::buffer(ss.str()),
            boost::bind(&TcpSendProcesor<C,R,H>::handle_write, this,
                boost::asio::placeholders::error));

        //  await response
        unique_lock<mutex> lock(response_mutex);
        rcv_response_header_ = CommandHeader();
        response_available.wait(lock, [this]{return rcv_response_header_.type == MESSAGE_TYPE_RESPONSE;});

        if( command_id != rcv_response_header_.id){
            throw std::runtime_error("wrong command id in channel provider; expected(" + to_string(command_id) + ")  returned(" + to_string(rcv_response_header_.id) + ") ");
        }

        return rcv_response_;

    }

protected:
    TcpSendProcesor(boost::asio::io_service& io_service, int buffer_size = DEFAULT_TCP_SESSION_BUFFER_SIZE)
        : socket_(io_service), read_buffer_(buffer_size){
    }

    virtual ~TcpSendProcesor(){
    }

    void startRecieving(){
        socket_.async_read_some( boost::asio::buffer(read_buffer_, read_buffer_.size()),
            boost::bind(&TcpSendProcesor<C,R,H>::handle_read, this,
                boost::asio::placeholders::error,
                boost::asio::placeholders::bytes_transferred));


    }

    virtual void handle_comm_error( const boost::system::error_code& error ){

    }



private:
    void handle_read(const boost::system::error_code& error, size_t bytes_transferred){
        if (!error){

            CommandHeader header;
            {
                lock_guard<mutex> lock(response_mutex);

                read_buffer_[bytes_transferred] = '\0';

                std::stringstream sr;
                sr << read_buffer_.data();
                boost::archive::text_iarchive ia(sr);

                ia >> header;
                if( header.type == MESSAGE_TYPE_RESPONSE){
                    rcv_response_header_ = header;
                    ia >> rcv_response_;
                }
                else if( header.type == MESSAGE_TYPE_COMMAND){
                    rcv_command_header_ = header;
                    ia >> rcv_command_;
                }

            }

            if( header.type == MESSAGE_TYPE_RESPONSE){
                response_available.notify_all();
            }

            else if( header.type == MESSAGE_TYPE_COMMAND){
                handle_command();
            }

            socket_.async_read_some(boost::asio::buffer(read_buffer_, read_buffer_.size()),
                boost::bind(&TcpSendProcesor<C,R,H>::handle_read, this,
                    boost::asio::placeholders::error,
                    boost::asio::placeholders::bytes_transferred));


        }
        else{
            handle_comm_error(error);
        }
    }

    void handle_write(const boost::system::error_code& error){
        if (error){
            handle_comm_error(error);
        }
    }

    void handle_command( ){

        ///  handler class must pocess this function
        R response = H::process( rcv_command_);

        std::stringstream ss;
        boost::archive::text_oarchive oa(ss);
        rcv_command_header_.type = MESSAGE_TYPE_RESPONSE;
        oa << rcv_command_header_;
        oa << response;

        boost::asio::async_write(socket_,  boost::asio::buffer(ss.str()),
            boost::bind(&TcpSendProcesor<C,R,H>::handle_write, this,
                boost::asio::placeholders::error));

    }



};






template< class C, class R, class H > class ChannelProvider;


template< class C, class R, class H >
class TcpSession: public TcpSendProcesor<C,R,H>
{
private:

    std::vector<char> read_buffer_;
    int attached_client_id_ = 0;
    ChannelProvider<C,R,H>* parent_server;

public:
    TcpSession(boost::asio::io_service& io_service, int buffer_size = DEFAULT_TCP_SESSION_BUFFER_SIZE, ChannelProvider<C,R,H>* parent =  NULL)
        : TcpSendProcesor<C,R,H>(io_service, buffer_size), read_buffer_(buffer_size), parent_server(parent){
    }

    tcp::socket& socket(){
        return TcpSendProcesor<C,R,H>::socket_;
    }

    bool startSession(){

        boost::system::error_code error;
        size_t rlen = TcpSendProcesor<C,R,H>::socket_.read_some(boost::asio::buffer(read_buffer_), error);
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

        TcpSendProcesor<C,R,H>::socket_.write_some(boost::asio::buffer(ss.str()), error);
        if (error){
            return false;
        }

        TcpSendProcesor<C,R,H>::startRecieving();

        return true;
    }

    int attachedClientId(){
        return attached_client_id_;
    }

protected:
    void handle_comm_error( const boost::system::error_code& error ){

        if( parent_server != NULL ) parent_server->detach(this);
        delete this;

    }



};


#endif // TcpSession_H
