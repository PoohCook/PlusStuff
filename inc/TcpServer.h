/*
 * File:   TcpServer.h
 * Author: Pooh
 *
 * Created on September 21, 2018, 3:39 PM
 */

#ifndef TcpServer_H
#define TcpServer_H

#include <string>
#include <sstream>
#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/text_oarchive.hpp>
#include <boost/bind.hpp>
#include <boost/asio.hpp>


using namespace std;
using boost::asio::ip::tcp;

#define DEFAULT_TCP_SESSION_BUFFER_SIZE  1024

template< class C, class R, class H >
class TcpSession: H
{

private:
    tcp::socket socket_;
    std::vector<char> read_buffer_;
    std::string write_buffer_;

public:
    TcpSession(boost::asio::io_service& io_service, int buffer_size = DEFAULT_TCP_SESSION_BUFFER_SIZE)
        : socket_(io_service), read_buffer_(buffer_size){
    }

    tcp::socket& socket(){
        return socket_;
    }

    void startSession(){
        socket_.async_read_some( boost::asio::buffer(read_buffer_, read_buffer_.size()),
            boost::bind(&TcpSession<C,R,H>::handleRead, this,
                boost::asio::placeholders::error,
                boost::asio::placeholders::bytes_transferred));
    }



private:
    void handleRead(const boost::system::error_code& error, size_t bytes_transferred){
        if (!error){
            read_buffer_[bytes_transferred] = '\0';

            std::stringstream sr;
            sr << read_buffer_.data();
            boost::archive::text_iarchive ia(sr);
            C command;

            ia >> command;

            ///  handler class must pocess this function
            R response = H::process( command);

            std::stringstream ss;
            boost::archive::text_oarchive oa(ss);
            oa << response;

            write_buffer_ = ss.str();

            boost::asio::async_write(socket_,  boost::asio::buffer(write_buffer_),
                boost::bind(&TcpSession<C,R,H>::handleWrite, this,
                    boost::asio::placeholders::error));
        }
        else{
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
            delete this;
        }
    }


};


template< class C, class R, class H >
class TcpServer
{
private:
    boost::asio::io_service& io_service;
    tcp::acceptor acceptor_;
    int buffer_size_;

public:
    TcpServer(boost::asio::io_service& io, short port, int buffer_size = DEFAULT_TCP_SESSION_BUFFER_SIZE)
        : io_service(io),
        acceptor_(io, tcp::endpoint(tcp::v4(), port)),
        buffer_size_ (buffer_size) {

        TcpSession<C,R,H>* newSession = new TcpSession<C,R,H>(io_service, buffer_size_);
        acceptor_.async_accept(newSession->socket(),
            boost::bind(&TcpServer::handle_accept, this, newSession,
                boost::asio::placeholders::error));
    }

    void close(){
        acceptor_.cancel();
        acceptor_.close();
    }

private:

    void handle_accept(TcpSession<C,R,H>* newSession, const boost::system::error_code& error) {

        if (!error){
            newSession->startSession();
            newSession = new TcpSession<C,R,H>(io_service, buffer_size_);
            acceptor_.async_accept(newSession->socket(),
                boost::bind(&TcpServer::handle_accept, this, newSession,
                    boost::asio::placeholders::error));
        }
        else{
            delete newSession;
        }
    }

};


#endif // TcpServer_H
