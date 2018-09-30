/**
 * @file Worker.h
 * @author  Pooh Cook
 * @version 1.0
 * @created September 16, 2018, 1:20 PM
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

#ifndef Worker_H
#define Worker_H

#include <string>
#include <queue>
#include <mutex>
#include <thread>
#include <condition_variable>


using namespace std;

/**
 * @class Worker
 * @brief A simple Worker object to transfer processing of objects to a worker thread
 *
 * The class of the worker is specified as the templaet parameter and that class is assumed ot have a void process()
 * method. The Worker then provides a means of pushing objects of the template type to its queue and will
 * sequentially process each object on it's own managed thread. Options are provided to shutdown the thread without
 * waiting or by waiting for queued objects to be completed.
 *
 */
template<class T>
class Worker
{
private:
    queue<T> fifo;
    mutex worker_mutex;
    condition_variable next;
    std::thread worker_thread;
    bool keep_working = true;
    mutex command_mutex;

    bool waitNext(){
        unique_lock<mutex> lock(worker_mutex);
        next.wait(lock, [this]{return !fifo.empty() || !keep_working;});
        return keep_working;
    }

    T popNext(){
        lock_guard<mutex> lock(worker_mutex);
        T next_to_do = fifo.front();
        fifo.pop();
        return next_to_do;
    }

    void processWorkerObjects(){
        while (waitNext()){

            T to_do = popNext();
            to_do.process();

            next.notify_all();

        }
    }

public:
    /**
     * Default Constructor to create a worker with
     *
     */
    Worker(){
        worker_thread = std::thread(&Worker<T>::processWorkerObjects, this);
    }

    ~Worker(){
        shutdown(false);
    }

    /**
     * Push a to be done Template object to teh Queue for processing
     *
     */
    void push( T to_do){

        lock_guard<mutex> lock(command_mutex);

        if( !keep_working ){
            throw  runtime_error("attempt to push a todo onto a shutdown worker");
        }

        {
            lock_guard<mutex> lock(worker_mutex);
            fifo.push(to_do);
        }
        next.notify_all();
    }

    /**
     * Shutdown the worker object
     *
     * This will stop and join the worker managed thread. Optionally, this shutdown can be immediate or wait for
     * currently queued object to complete
     *
     * @param wait_complete  if true, the shutdown will await queue empty status prior to shutting down
     *
     */
    void shutdown(bool wait_complete){

        lock_guard<mutex> lock(command_mutex);
         if( keep_working ){
            if( wait_complete){
                unique_lock<mutex> lock(worker_mutex);
                next.wait(lock, [this]{return fifo.empty();});
             }

            {
                lock_guard<mutex> lock(worker_mutex);
                keep_working = false;
            }
            next.notify_all();
            worker_thread.join();
         }
     }

};


#endif   // Worker_H



