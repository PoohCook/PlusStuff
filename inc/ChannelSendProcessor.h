/**
 * @file ChannelSendProcessor.h
 * @author  Pooh Cook
 * @version 1.0
 * @created September 27, 2018, 7:34 PM
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

#ifndef ChannelSendProcessor_H
#define ChannelSendProcessor_H

#include <iostream>
#include <string>
#include <sstream>
#include <chrono>
#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/text_oarchive.hpp>
#include <boost/bind.hpp>
#include <boost/asio.hpp>
#include "ChannelMessageHeader.h"
#include "Logger.h"

using namespace std;
using boost::asio::ip::tcp;

#define DEFAULT_TCP_SESSION_BUFFER_SIZE  4096

/**
 * @class NullHandler
 * @brief Empty handler class for instances when H (handler) class is not provided for ChannelClient or ChannelProvider
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
 * @class ChannelSendProcessor
 * @brief Processor template for R Send(C) implementation
 *
 * This class is not intended for direct client consumption. It is used by both the ChannelProviderSession and the
 * ChannelClientSession objects to implement the templated handling methods for "Response Send (Command)".
 * It performs the serialization and deserialization handling and also implements the Synchronous wait between
 * Command and Response messages. It derives from H (handler) class to provide access to the assigned handler process methods.
 *
 */
template< class C, class R, class H >
class ChannelSendProcessor: H
{

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
    ChannelMessageHeader rcv_response_header_;
    R rcv_response_;

    ChannelMessageHeader rcv_command_header_;
    C rcv_command_;

    int attached_client_id_ = 0;
    int process_timeout_seconds_;

    tcp::socket socket_;    ///  Boost asio socket for communication

public:
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
    R send( C command){

        int command_id = command_id_++;
        ChannelMessageHeader header(MESSAGE_TYPE_COMMAND, command_id);

        std::stringstream ss;
        boost::archive::text_oarchive oa(ss);

        oa << header;
        oa << command ;
        ss << ";";

        // setup response wait lock.
        // This needs to happen here because, response can be received prior to completion of write
        unique_lock<mutex> lock(response_mutex);
        rcv_response_header_ = ChannelMessageHeader();

        write_data(ss.str(), header);

        //  await response
        if( !response_available.wait_for(lock,
                                    std::chrono::seconds(process_timeout_seconds_),
                                    [this]{return rcv_response_header_.type == MESSAGE_TYPE_RESPONSE;})){

            throw std::runtime_error("timeout awaiting reply to frame(" + to_string(command_id) + ")");

        }

        if( command_id != rcv_response_header_.id){
            throw std::runtime_error("wrong command id in channel provider; expected(" + to_string(command_id) + ")  returned(" + to_string(rcv_response_header_.id) + ") ");
        }

         return rcv_response_;



    }

    /**
     * Closes the socket so the io_service can be shutdown during destruction of the consumer class
     *
     */
    void close(){
        socket_.close();
    }


    virtual ~ChannelSendProcessor(){

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

    /**
     * @brief access method to get the socket for the session
     *
     */
    tcp::socket& socket(){
        return socket_;
    }



protected:
    /**
     * Constructor for creating a ChannelSendProcessor instance
     *
     * @param io_service reference to an instance of an asio io service object
     * @param process_timeout_seconds number of seconds to wait before timing out a process attempt. Default is 10
     * @param initial_command_id each channel message is given a sequentially increasing command id starting at this number
     *          default is 1000
     * @param buffer_size defines the size of the channel buffers for TCP messages.  This is the max serialization size
     *          for the command or response object. The serialization size is determined by boost serialization libraries.
     *          default is 4096 chars
     *
     */
    ChannelSendProcessor(boost::asio::io_service& io_service, int process_timeout_seconds = 10,
                          int initial_command_id = 1000, int buffer_size = DEFAULT_TCP_SESSION_BUFFER_SIZE)
        : io_service(io_service), buffer_size_(buffer_size), read_buffer_(buffer_size),
        command_id_(initial_command_id), process_timeout_seconds_(process_timeout_seconds), socket_(io_service) {
    }

    /**
     *  @brief set the attached client id
     *
     */
    void set_attached_client_id(int client_id){
        attached_client_id_ = client_id;
    }

    /**
     * @brief begin receiving messages from the socket
     *
     */
    void startReceiving(){
        lock_guard<mutex> lock(write_mutex);

        socket_.async_receive( boost::asio::buffer(read_buffer_, read_buffer_.size()),
            boost::bind(&ChannelSendProcessor<C,R,H>::handle_read, this,
                boost::asio::placeholders::error,
                boost::asio::placeholders::bytes_transferred));


    }

    /**
     * virtual method to allow derived classes access to comm errors that occur
     *
     */
    virtual void handle_comm_error( const boost::system::error_code& error ){

    }



private:

    void write_data (string send_data, ChannelMessageHeader header ){

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
        socket_.write_some(boost::asio::buffer(send_data), error);
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
            //  the results of a boost asio async read can often have multiple read messages concatenated
            //  together into one...  This only occurs when multiple threads are involved calling from
            //  client and provider. This stops the down stream problem by parsing them back out based upon
            //  a semicolon added at serialization time...
            std::stringstream ss(read_buffer_.data());
            std::string message;
            while (std::getline(ss, message, ';')) {
                handle_archive_message(message);
            }

            startReceiving();

        }
        else{
            handle_comm_error(error);
        }
    }


    void handle_archive_message(string message){

         ChannelMessageHeader header;
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
            handle_command( header );
        }


    }

    void handle_command(ChannelMessageHeader header ){

         ///  handler class must possess this function
        R response = H::process( attached_client_id_, rcv_command_);

        std::stringstream ss;
        boost::archive::text_oarchive oa(ss);
        header.type = MESSAGE_TYPE_RESPONSE;
        oa << header;
        oa << response;
        ss << ";";

        write_data(ss.str(), header);

    }



};



#endif // ChannelSendProcessor_H
