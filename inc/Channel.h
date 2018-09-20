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
#include <boost/bind.hpp>
#include <boost/asio.hpp>
#include <boost/array.hpp>


using namespace std;
using boost::asio::ip::tcp;

enum{ maxDataLength = 1024};

class TcpSession{

private:
    tcp::socket socket_;
    boost::array<char, maxDataLength> readBuffer_;
    std::string writeBuffer_;

public:
    TcpSession(boost::asio::io_service& io_service)
        : socket_(io_service){
    }

    tcp::socket& socket(){
        return socket_;
    }

    void startSession(){
        socket_.async_read_some( boost::asio::buffer(readBuffer_, maxDataLength),
            boost::bind(&TcpSession::handle_read, this,
                boost::asio::placeholders::error,
                boost::asio::placeholders::bytes_transferred));
    }

private:
    void handle_read(const boost::system::error_code& error, size_t bytes_transferred){
        if (!error){
            readBuffer_[bytes_transferred] = '\0';
            writeBuffer_ = std::string(readBuffer_.data()) + "Bunny really ruvs Pooh";

            boost::asio::async_write(socket_,  boost::asio::buffer(writeBuffer_),
                boost::bind(&TcpSession::handle_write, this,
                    boost::asio::placeholders::error));
        }
        else{
            delete this;
        }
    }

    void handle_write(const boost::system::error_code& error){
        if (!error){
            socket_.async_read_some(boost::asio::buffer(readBuffer_, maxDataLength),
                boost::bind(&TcpSession::handle_read, this,
                    boost::asio::placeholders::error,
                    boost::asio::placeholders::bytes_transferred));
        }
        else{
            delete this;
        }
    }


};


class TcpServer
{
private:
    boost::asio::io_service& io_;
    tcp::acceptor acceptor_;

public:
    TcpServer(boost::asio::io_service& io, short port)
        : io_(io),
        acceptor_(io, tcp::endpoint(tcp::v4(), port)){

        TcpSession* newSession = new TcpSession(io_);
        acceptor_.async_accept(newSession->socket(),
            boost::bind(&TcpServer::handle_accept, this, newSession,
                boost::asio::placeholders::error));
    }

    void close(){
        acceptor_.cancel();
        acceptor_.close();
    }

private:

    void handle_accept(TcpSession* newSession, const boost::system::error_code& error) {

        if (!error){
            newSession->startSession();
            newSession = new TcpSession(io_);
            acceptor_.async_accept(newSession->socket(),
                boost::bind(&TcpServer::handle_accept, this, newSession,
                    boost::asio::placeholders::error));
        }
        else{
            delete newSession;
        }
    }

};




template<class T>
class ChannelProvider{

private:
    boost::asio::io_service io_;
    TcpServer server_;
    std::thread workerThread;

    void runIo(){
        io_.run();
    }

public:
    ChannelProvider(short port)
        : server_(io_, port) {

        workerThread = std::thread(&ChannelProvider<T>::runIo, this);

    }

    ~ChannelProvider(){

        server_.close();
        workerThread.join();

    }


};


template<class T>
class ChannelClient{

private:

    boost::asio::io_service io_;
    tcp::socket socket_;

public:
    ChannelClient(short port)
        : socket_(io_) {

        tcp::resolver resolver(io_);
        tcp::resolver::results_type endpoints =
          resolver.resolve("127.0.0.1", to_string(port));

        boost::asio::connect(socket_, endpoints);

    }

    ~ChannelClient(){

        socket_.cancel();
        socket_.close();
    }


    string sendReceive( string send){

        boost::system::error_code error;
        socket_.write_some(boost::asio::buffer(send), error);
        if (error){
            throw boost::system::system_error(error);
        }

        boost::array<char, maxDataLength> buf;
        size_t rlen = socket_.read_some(boost::asio::buffer(buf), error);
        if (error && error != boost::asio::error::eof ){
            throw boost::system::system_error(error);
        }

        buf[rlen] = '\0';
        return buf.data();

    }

};



#endif // Channel_H