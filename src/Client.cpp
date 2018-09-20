//
// client.cpp
// ~~~~~~~~~~
//
// Copyright (c) 2003-2018 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#include <iostream>
#include <string>
#include <boost/array.hpp>
#include <boost/asio.hpp>

using boost::asio::ip::tcp;

int main()
{
  try
  {

    boost::asio::io_context io_context;

    tcp::resolver resolver(io_context);
    tcp::resolver::results_type endpoints =
      resolver.resolve("127.0.0.1", "1028");

    tcp::socket socket(io_context);
    boost::asio::connect(socket, endpoints);
    boost::system::error_code error;

    for (;;)
    {
      std::cout << "writing\n";
      std::string message = "Pooh Ruvs Bunny ";
      size_t wlen = socket.write_some(boost::asio::buffer(message), error);
      std::cout << "write len = " << std::to_string(wlen) << "\n";


      std::cout << "readiing\n";

      boost::array<char, 128> buf;
      size_t rlen = socket.read_some(boost::asio::buffer(buf), error);

      if (error && error != boost::asio::error::eof ){
//        break; // Connection closed cleanly by peer.
//      else if (error)
        std::cout << "thowing\n";
        throw boost::system::system_error(error); // Some other error.
      }

      std::cout << "str len = " << std::to_string(rlen) << ";";
      std::cout.write(buf.data(), rlen);
      std::cout << '\n';


      boost::asio::steady_timer t(io_context, boost::asio::chrono::seconds(1));
      t.wait();

    }
  }
  catch (std::exception& e)
  {
    std::cerr << e.what() << std::endl;
  }

  return 0;
}