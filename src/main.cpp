


#include <iostream>

using namespace std;

#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>

void work_for_io_service(const boost::system::error_code& /*e*/, int number)
{
  std::cout << "Non-blocking wait("<< number <<") \n";

}

int main()
{
  boost::asio::io_service io;

  boost::asio::deadline_timer timer1(io, boost::posix_time::seconds(5));
  boost::asio::deadline_timer timer2(io, boost::posix_time::seconds(6));
  boost::asio::deadline_timer timer3(io, boost::posix_time::seconds(7));

  // work_for_io_service() will be called
  // when async operation (async_wait()) finishes
  // note: Though the async_wait() immediately returns
  //       but the callback function will be called when time expires
  timer1.async_wait(boost::bind(work_for_io_service, boost::asio::placeholders::error, 5));
  timer2.async_wait(boost::bind(work_for_io_service, boost::asio::placeholders::error, 6));
  timer3.async_wait(boost::bind(work_for_io_service, boost::asio::placeholders::error, 7));

  std::cout << " If we see this before the callback function, we know async_wait() returns immediately.\n This confirms async_wait() is non-blocking call!\n";

  // the callback function, work_for_io_service(), will be called
  // from the thread where io.run() is running.
  io.run();


  return 0;
}

