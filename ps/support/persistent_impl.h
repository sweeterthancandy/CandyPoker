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
