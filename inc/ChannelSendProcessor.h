/*
 * File:   ChannelSendProcessor.h
 * Author: Pooh
 *
 * Created on September 27, 2018, 7:34 PM
 */

#ifndef ChannelSendProcessor_H
#define ChannelSendProcessor_H

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
class ChannelSendProcessor: H
{
protected:
    tcp::socket socket_;

private:
    std::vector<char> read_buffer_;
    mutex sync_mutex;
    mutex response_mutex;
    condition_variable response_available;

    int command_id_ = 5000;
    CommandHeader rcv_response_header_;
    R rcv_response_;

    CommandHeader rcv_command_header_;
    C rcv_command_;


public:
    R send( C command){

        try{

            int command_id = command_id_++;
            CommandHeader header(MESSAGE_TYPE_COMMAND, command_id);

            std::stringstream ss;
            boost::archive::text_oarchive oa(ss);

            oa << header;
            oa << command ;

            boost::asio::async_write(socket_,  boost::asio::buffer(ss.str()),
                boost::bind(&ChannelSendProcessor<C,R,H>::handle_write, this,
                    boost::asio::placeholders::error));

            //  await response
            unique_lock<mutex> lock(response_mutex);
            rcv_response_header_ = CommandHeader();
            response_available.wait(lock, [this]{return rcv_response_header_.type == MESSAGE_TYPE_RESPONSE;});

            if( command_id != rcv_response_header_.id){
                throw std::runtime_error("wrong command id in channel provider; expected(" + to_string(command_id) + ")  returned(" + to_string(rcv_response_header_.id) + ") ");
            }

        }catch(boost::archive::archive_exception err ){
            cout << "caught: " << err.what() << std::endl;
        }


        return rcv_response_;



    }

protected:
    ChannelSendProcessor(boost::asio::io_service& io_service, int buffer_size = DEFAULT_TCP_SESSION_BUFFER_SIZE)
        : socket_(io_service), read_buffer_(buffer_size){
    }

    virtual ~ChannelSendProcessor(){
    }

    void startRecieving(){
        lock_guard<mutex> lock(sync_mutex);

        socket_.async_receive( boost::asio::buffer(read_buffer_, read_buffer_.size()),
            boost::bind(&ChannelSendProcessor<C,R,H>::handle_read, this,
                boost::asio::placeholders::error,
                boost::asio::placeholders::bytes_transferred));


    }

    virtual void handle_comm_error( const boost::system::error_code& error ){

    }



private:
    void handle_read(const boost::system::error_code& error, size_t bytes_transferred){

        if (!error){
            try{
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

                socket_.async_receive(boost::asio::buffer(read_buffer_, read_buffer_.size()),
                    boost::bind(&ChannelSendProcessor<C,R,H>::handle_read, this,
                        boost::asio::placeholders::error,
                        boost::asio::placeholders::bytes_transferred));

           }catch(boost::archive::archive_exception err ){
                cout << "caught2: " << err.what() <<  "read_buffer: " << read_buffer_.data() << std::endl;
            }


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
            boost::bind(&ChannelSendProcessor<C,R,H>::handle_write, this,
                boost::asio::placeholders::error));

    }



};



#endif // ChannelSendProcessor_H
