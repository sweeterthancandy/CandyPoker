/*

CandyPoker
https://github.com/sweeterthancandy/CandyPoker

MIT License

Copyright (c) 2019 Gerry Candy

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.

*/
#ifndef PS_SUPPORT_PROC_H
#define PS_SUPPORT_PROC_H

#include "ps/support/push_pull.h"

#include <list>

namespace ps{
namespace support{


struct single_processor{
        using work_t = std::function<void()>;
        single_processor()
                :work_{static_cast<size_t>(-1)}
                ,wh_(work_.make_work_handle())
        {
        }
        ~single_processor(){
        }
        void add(work_t w){
                work_.push(std::move(w));
        }
        void join(){
                wh_.unlock();
                for( auto& t : threads_ )
                        t.join();
                threads_.clear();
        }
        void run(){
                for(;;){
                        auto work = work_.pull_no_wait();
                        if( ! work )
                                break;
                        work.get()();
                }
        }
        void spawn(){
                threads_.emplace_back(
                        [this]()mutable{
                        __main__();
                });
        }
private:
        void __main__(){
                for(;;){
                        auto work = work_.pull() ;
                        if( ! work )
                                break;
                        work.get()();
                }
        }
        support::push_pull<work_t>         work_;
        support::push_pull<work_t>::handle wh_;
        std::vector<std::thread> threads_;

};

} // support
} // ps

#endif // PS_SUPPORT_PROC_H
