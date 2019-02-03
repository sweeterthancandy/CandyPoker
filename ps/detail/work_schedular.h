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
#ifndef PS_DETAIL_WORK_SCHEDULER_H
#define PS_DETAIL_WORK_SCHEDULER_H

#include <functional>
#include <thread>
#include <mutex>
#include <vector>

namespace ps{
namespace detail{
        // toy schedular
        struct work_scheduler{
                void decl( std::function<void()> work){
                        work_.emplace_back(std::move(work));
                }
                auto run(){
                        std::mutex mtx;
                        std::vector<std::thread> workers;
                        size_t done{0};

                        auto proto = [&]()mutable{
                                for(;;){
                                        mtx.lock();
                                        if( work_.empty()){
                                                mtx.unlock();
                                                break;
                                        }
                                        auto w = work_.back();
                                        work_.pop_back();
                                        mtx.unlock();
                                        w();
                                        mtx.lock();
                                        std::cerr << "Done " << ++done << ", " << work_.size() << " Left\n";
                                        mtx.unlock();
                                }
                        };
                        for(size_t i=0;i!= std::thread::hardware_concurrency();++i){
                                workers.emplace_back(proto);
                        }
                        for( auto& t : workers){
                                t.join();
                        }
                }
        private:
                std::vector<std::function<void()> > work_;
        };

} /* ps */
} /* detail */

#endif // PS_DETAIL_WORK_SCHEDULER_H
