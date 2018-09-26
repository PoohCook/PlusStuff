/*
 * File:   Channel.h
 * Author: Pooh
 *
 * Created on September 18, 2018, 4:09 PM
 */

#ifndef Channel_H
#define Channel_H

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
#include "TcpServer.h"

using namespace std;


template< class C, class R, class H >
class ChannelProvider
{

private:
    boost::asio::io_service io_service;
    TcpServer<C,R,H> server_;
    std::thread worker_thread;
    int buffer_size_;

    void run_io(){
        io_service.run();
    }

public:
    ChannelProvider(short port, int buffer_size = DEFAULT_TCP_SESSION_BUFFER_SIZE)
        : server_(io_service, port),
          buffer_size_(buffer_size) {

        worker_thread = std::thread(&ChannelProvider<C,R,H>::run_io, this);

    }

    ~ChannelProvider(){
        server_.close();
        worker_thread.join();
    }

    vector<int> attachedSessionIds(){
        return server_.attachedSessionIds();

    }

    void whitelist(int id){
        server_.whitelist(id);

    }

    R send( int client_id, C command){
        return server_.send( client_id, command);

    }



};


template< class C, class R >
class NullHandler{
public:
    R process(C command ){
       return R();
    }
};


template< class C, class R, class H = NullHandler<C,R> >
class ChannelClient : H
{

private:

    boost::asio::io_service io_service;
    boost::asio::io_service::work* working_ = NULL;
    tcp::socket socket_;
    std::thread worker_thread;
    int command_id_ = 1000;  //   arbitray start index for commnad ids
    int buffer_size_;
    std::vector<char> read_buffer_;

    mutex response_mutex;
    condition_variable response_available;

    CommandHeader rcv_header_;
    R rcv_response_;
    C rcv_command_;

    void run_io(){
        io_service.run();
    }

    bool attach_socket(int client_id){
        std::stringstream ss;
        boost::archive::text_oarchive oa(ss);
        CommandHeader header(MESSAGE_TYPE_ATTACH, client_id);

        oa << header ;

        boost::system::error_code error;
        socket_.write_some(boost::asio::buffer(ss.str()), error);
        if (error){
            throw boost::system::system_error(error);
        }

        std::vector<char> buf(buffer_size_);
        size_t rlen = socket_.read_some(boost::asio::buffer(buf), error);
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


public:
    ChannelClient(int client_id, short port, string address = "127.0.0.1", int buffer_size = DEFAULT_TCP_SESSION_BUFFER_SIZE)
        : socket_(io_service),
          buffer_size_(buffer_size),
          read_buffer_(buffer_size)  {

        tcp::resolver resolver(io_service);
        tcp::resolver::results_type endpoints =
          resolver.resolve(address, to_string(port));

        boost::asio::connect(socket_, endpoints);
        if( attach_socket(client_id) ){
            working_ = new boost::asio::io_service::work(io_service);

            worker_thread = std::thread(&ChannelClient<C,R,H>::run_io, this);

            socket_.async_read_some(boost::asio::buffer(read_buffer_, read_buffer_.size()),
                boost::bind(&ChannelClient<C,R,H>::handle_read, this,
                    boost::asio::placeholders::error,
                    boost::asio::placeholders::bytes_transferred));
        }

    }

    ~ChannelClient(){

        if( working_ != NULL ){
            socket_.cancel();
            socket_.close();
            delete working_;
            worker_thread.join();

        }
    }


    R send( C command){

        int command_id = command_id_++;
        CommandHeader header(MESSAGE_TYPE_COMMAND, command_id);

        std::stringstream ss;
        boost::archive::text_oarchive oa(ss);

        oa << header;
        oa << command ;

        boost::asio::async_write(socket_,  boost::asio::buffer(ss.str()),
            boost::bind(&ChannelClient<C,R,H>::handle_write, this,
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

    void handle_write(const boost::system::error_code& error){
        if (error){
        }
    }

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
                boost::bind(&ChannelClient<C,R,H>::handle_read, this,
                    boost::asio::placeholders::error,
                    boost::asio::placeholders::bytes_transferred));

        }
    }

    void handle_command(){

        ///  handler class must pocess this function
        R response = H::process( rcv_command_);

        std::stringstream ss;
        boost::archive::text_oarchive oa(ss);
        rcv_header_.type = MESSAGE_TYPE_RESPONSE;
        oa << rcv_header_;
        oa << response;

        boost::asio::async_write(socket_,  boost::asio::buffer(ss.str()),
            boost::bind(&ChannelClient<C,R,H>::handle_write, this,
                boost::asio::placeholders::error));

    }

};


#endif // Channel_H