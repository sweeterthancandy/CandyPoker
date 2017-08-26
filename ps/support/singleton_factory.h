#ifndef PS_SUPPORT_SINGLETON_FACTORY_H
#define PS_SUPPORT_SINGLETON_FACTORY_H

#include <iostream>

#include <boost/type_index.hpp>
#include <boost/exception/all.hpp>
#include <iostream>
#include <map>
#include <functional>
#include <memory>
#include <mutex>
#include <type_traits>

namespace ps{
namespace support{
namespace detail{

        struct singleton_controller{
                using maker_t = 
                        std::function<
                        std::shared_ptr<void>()
                        >;
                void* get_or_null(boost::typeindex::type_index const& ti, std::string const& name);
                void register_(boost::typeindex::type_index const& ti, std::string name, maker_t maker);
                static singleton_controller* get_inst();
        private:

                struct item{
                        explicit item(maker_t maker)
                                :maker_(std::move(maker))
                        {}
                        void* get(){
                                if( ! ptr_ ){
                                        std::lock_guard<std::mutex> lock(mtx_);
                                        if( ! ptr_ ){
                                                ptr_ = maker_();
                                        }
                                }
                                return ptr_.get();
                        }
                private:
                        std::shared_ptr<void> ptr_;
                        maker_t maker_;
                        std::mutex mtx_;
                };
                std::map<
                        boost::typeindex::type_index,
                        std::map<
                                std::string,
                                item
                        >
                > type_inst_map_;
        };

} // detail

template<class T>
struct singleton_factory {

        using maker_t = detail::singleton_controller::maker_t;

        template<class U>
        static void register_(std::string const& name = "__default__", maker_t maker = [] { return std::make_shared<U>(); }) {
                static_assert(std::is_base_of<T, U>::value, "not a base");
                //std::cout << "registration of " << boost::typeindex::type_id<U>().pretty_name() << " as " << name << "\n";
                detail::singleton_controller::get_inst()->register_(boost::typeindex::type_id<T>(), std::move(name), std::move(maker));
        }
        static T* get_or_null(std::string const& name = "__default__") {
                //std::cout << "getting name = " << name << " for " << boost::typeindex::type_id<T>().pretty_name() << "\n";
                return reinterpret_cast<T*>(detail::singleton_controller::get_inst()->get_or_null(boost::typeindex::type_id<T>(), name));
        }
        static T& get(std::string const& name = "__default__") {
                auto ptr = get_or_null(name);
                if (ptr == nullptr)
                        BOOST_THROW_EXCEPTION(std::domain_error("key doesn't exist " + name));
                return *ptr;
        }

};

} // support
} // ps
#endif // PS_SUPPORT_SINGLETON_FACTORY_H
