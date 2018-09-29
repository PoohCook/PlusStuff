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
#include "Logger.h"


using namespace std;
using boost::asio::ip::tcp;

#define DEFAULT_TCP_SESSION_BUFFER_SIZE  4096

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

    string str(){
        return to_string(id) + ":" + to_string(type);
    }

};



template< class C, class R, class H >
class ChannelSendProcessor: H
{
protected:
    tcp::socket socket_;

private:
    boost::asio::io_service& io_service;
    int buffer_size_;
    std::vector<char> read_buffer_;

    mutex write_mutex;
    mutex response_mutex;
    condition_variable response_available;
    condition_variable write_in_progress;
    bool write_in_progress_flag = false;

    int command_id_;
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
            ss << ";";

            write_data(ss.str(), header);

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
    ChannelSendProcessor(boost::asio::io_service& io_service, int initial_command_id = 1000, int buffer_size = DEFAULT_TCP_SESSION_BUFFER_SIZE)
        : socket_(io_service), io_service(io_service), buffer_size_(buffer_size), read_buffer_(buffer_size),
        command_id_(initial_command_id) {
    }

    virtual ~ChannelSendProcessor(){
    }

    void startRecieving(){
        lock_guard<mutex> lock(write_mutex);

        socket_.async_receive( boost::asio::buffer(read_buffer_, read_buffer_.size()),
            boost::bind(&ChannelSendProcessor<C,R,H>::handle_read, this,
                boost::asio::placeholders::error,
                boost::asio::placeholders::bytes_transferred));


    }

    virtual void handle_comm_error( const boost::system::error_code& error ){

    }



private:

    void write_data (string send_data, CommandHeader header ){

        lock_guard<mutex> lock(write_mutex);
         LogMessage(string("write start: ") + header.str() );

//        unique_lock<mutex> lock(write_mutex);
//        write_in_progress.wait(lock, [this]{return !write_in_progress_flag;});
//
//        write_in_progress_flag = true;
//        cout << "write locked\n";
//
//        boost::asio::async_write(socket_,  boost::asio::buffer(send_data),
//            boost::bind(&ChannelSendProcessor<C,R,H>::handle_write, this,
//                boost::asio::placeholders::error));

        boost::system::error_code error;
        ChannelSendProcessor<C,R,H>::socket_.write_some(boost::asio::buffer(send_data), error);
        if (error){
            handle_comm_error(error);
        }

        LogMessage(string("write end: ") + header.str() );

    }

//    void handle_write(const boost::system::error_code& error){
//
//        {
//            lock_guard<mutex> lock(write_mutex);
//            write_in_progress_flag = false;
//        }
//
//        write_in_progress.notify_all();
//
//        LogMessage("write unlocked");
//
//        if (!error){
//
//        }
//        else{
//            handle_comm_error(error);
//        }
//    }


    void handle_read(const boost::system::error_code& error, size_t bytes_transferred){

        if (!error){
            read_buffer_[bytes_transferred] = '\0';

            //  this needs some splaining... for reasons that that seemingly can't be locked away,
            //  the results of a boost asio async read can often have multiple read messsages concatinated
            //  together into one...  This only occurs when multiple threads are involved calling from
            //  client and provider. This stops the down stream problem by parsing themback out based upon
            //  a semicolon added at serilization time...
            std::stringstream ss(read_buffer_.data());
            std::string message;
            while (std::getline(ss, message, ';')) {
                handle_archive_message(message);
            }

            startRecieving();

        }
        else{
            handle_comm_error(error);
        }
    }


    void handle_archive_message(string message){
        try{
            CommandHeader header;
            {
                lock_guard<mutex> lock(response_mutex);


                LogMessage(  string(message)  );

                std::stringstream sr;
                sr << message;
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


       }catch(boost::archive::archive_exception err ){
            cout << "caught2: " << err.what() << std::endl;
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
        ss << ";";

        write_data(ss.str(), rcv_command_header_);

    }



};



#endif // ChannelSendProcessor_H
