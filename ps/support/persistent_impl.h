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
#ifndef PS_SUPPORT_PERSISTENT_IMPL_H
#define PS_SUPPORT_PERSISTENT_IMPL_H

#include <mutex>
#include <fstream>
#include "ps/support/persistent.h"

#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/text_oarchive.hpp>
#include <boost/archive/binary_iarchive.hpp>
#include <boost/archive/binary_oarchive.hpp>
#include <boost/serialization/base_object.hpp>
#include <boost/serialization/utility.hpp>
#include <boost/serialization/map.hpp>
#include <boost/serialization/vector.hpp>
#include <boost/serialization/assume_abstract.hpp>

namespace ps{
namespace support{
                
        template<class T>
        struct persistent_memory_impl_serializer : persistent_memory_base::impl_base{
                virtual std::shared_ptr<void> load(std::string const& path)const{
                        auto ctrl = persistent_memory_base::get_controller();
                        ctrl->begin_load(path);
                        using archive_type = boost::archive::text_iarchive;
                        auto ptr = std::make_shared<T>();
                        std::ifstream ofs(path);
                        archive_type oa(ofs);
                        oa >> *ptr;
                        ctrl->end_load(path);
                        return ptr;
                }
                virtual void save(std::shared_ptr<void> ptr, std::string const& path)const {
                        using archive_type = boost::archive::text_oarchive;
                        auto typed = reinterpret_cast<T*>(ptr.get());
                        std::ofstream ofs(path);
                        archive_type oa(ofs);
                        oa << *typed;
                }
                virtual void const* ptr()const{
                        if( loaded_ ){
                                return memory_.get();
                        }
                        std::lock_guard<std::mutex> lock(mtx_);
                        auto ctrl = persistent_memory_base::get_controller();
                        auto path = ctrl->alloc_path(name());
                        if( loaded_ ){
                                return memory_.get();
                        }
                        try{
                                memory_ = ps::detail::reinterpret_pointer_cast<T>(load(name()));
                        } catch(...){
                                memory_ = make();
                                save(memory_, name());
                        }
                        loaded_ = true;
                        return memory_.get();
                }
                virtual std::shared_ptr<T> make()const=0;
        private:
                mutable bool loaded_{false};
                mutable std::shared_ptr<T> memory_;
                mutable std::mutex mtx_;
        };

} // end namespace support
} // end namespace ps

#endif // PS_SUPPORT_PERSISTENT_IMPL_H
