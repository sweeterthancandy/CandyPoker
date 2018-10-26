#ifndef PS_SUPPORT_PERSISTENT_HPP
#define PS_SUPPORT_PERSISTENT_HPP

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
struct persistent_memory_decl{
        using te_maker_type = std::function<std::shared_ptr<T>()>;
        template<class F>
        explicit persistent_memory_decl(std::string const& name, F&& f)
                :maker_{f}
                ,name_{".persistent." + name}
        {
        }
        std::shared_ptr<T> load(std::string const& path)const{
                using archive_type = boost::archive::text_iarchive;
                auto ptr = std::make_shared<T>();
                std::ifstream ofs(path);
                archive_type oa(ofs);
                oa >> *ptr;
                return ptr;
        }
        void save(std::shared_ptr<T> ptr, std::string const& path)const{
                using archive_type = boost::archive::text_oarchive;
                auto typed = reinterpret_cast<T*>(ptr.get());
                std::ofstream ofs(path);
                archive_type oa(ofs);
                oa << *typed;
        }
        T const* operator->()const{
                return ptr_();
        }
        T const& operator*()const{
                return *ptr_();
        }
private:
        T const* ptr_()const{
                if( loaded_ ){
                        return memory_.get();
                }
                std::lock_guard<std::mutex> lock(mtx_);
                if( loaded_ ){
                        return memory_.get();
                }
                try{
                        memory_ = load(name_);
                } catch(...){
                        memory_ = maker_();
                        save(memory_, name_);
                }
                loaded_ = true;
                return memory_.get();
        }

        te_maker_type maker_;
        std::string name_;
        mutable bool loaded_{false};
        mutable std::shared_ptr<T> memory_;
        mutable std::mutex mtx_;
};
} // end namespace support
} // end namespace ps

#endif // PS_SUPPORT_PERSISTENT_HPP
