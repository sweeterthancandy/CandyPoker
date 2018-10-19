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

        template<class U>
        static void register_(std::string const& name = "__default__") {
                static_assert(std::is_base_of<T, U>() || std::is_same<void, T>(), "not a base");
                //std::cout << "registration of " << boost::typeindex::type_id<U>().pretty_name() << " as " << name << "\n";
                detail::singleton_controller::get_inst()->register_(boost::typeindex::type_id<T>(), std::move(name), [] { return std::make_shared<U>(); });
        }
        static T* get_or_null(std::string const& name = "__default__") {
                //std::cout << "getting name = " << name << " for " << boost::typeindex::type_id<T>().pretty_name() << "\n";
                return reinterpret_cast<T*>(detail::singleton_controller::get_inst()->get_or_null(boost::typeindex::type_id<T>(), name));
        }
        #if 0
        template<class U = T, class K = std::enable_if_t< std::is_same<void, U>::value == false> >
        static T& get(std::string const& name = "__default__") {
                auto ptr = get_or_null(name);
                if (ptr == nullptr)
                        BOOST_THROW_EXCEPTION(std::domain_error("key doesn't exist " + name));
                return *ptr;
        }
        #endif

};

using any_singleton_factory = singleton_factory<void>;

} // support
} // ps

#if 0
#include <boost/type_index.hpp>
#include <boost/exception/all.hpp>
#include <iostream>
#include <map>
#include <functional>
#include <memory>
#include <mutex>

namespace ps{
        namespace support{

                template<class T>
                        struct singleton_factory{
                        private:
                                using maker_t = 
                                        std::function<
                                        std::unique_ptr<void>()
                                        >;
                                struct item{
                                        explicit item(maker_t maker)
                                                :maker_(std::move(maker))
                                        {}
                                        T& get(){
                                                if( ! ptr_ ){
                                                        std::lock_guard<std::mutex> lock(mtx_);
                                                        if( ! ptr_ ){
                                                                ptr_ = maker_();
                                                        }
                                                }
                                                return *ptr_;
                                        }
                                private:
                                        std::unique_ptr<void> ptr_;
                                        maker_t maker_;
                                        std::mutex mtx_;
                                };
                                template<class U>
                                        void register_impl( std::string const& name ){
                                                m_.emplace(name, std::make_unique<item>([](){ return std::make_unique<U>(); }));
                                        }
                                T& get_impl(std::string const& name){
                                        auto iter = m_.find(name);
                                        if( iter == m_.end() )
                                                BOOST_THROW_EXCEPTION(std::domain_error("singlton " + name + " not registered"));
                                        return iter->second->get();
                                }
                        public:

                                static singleton_factory* get_inst(){
                                        static singleton_factory* mem = nullptr;
                                        if( mem == nullptr )
                                                mem = new singleton_factory;
                                        return mem;
                                }

                                template<class U>
                                        static void register_( std::string const& name  = "__default__"){
                                                std::cout << "registration of " << boost::typeindex::type_id<U>().pretty_name() << " as " << name << "\n";
                                                get_inst()->template register_impl<U>(name);
                                        }
                                static T& get(std::string const& name = "__default__"){
                                        std::cout << "getting name = " << name << " for " << boost::typeindex::type_id<T>().pretty_name() << "\n";
                                        return get_inst()->get_impl(name);
                                }


                        private:
                                std::map<
                                        std::string,
                                        std::unique_ptr<item>
                                                > m_;
                        };
        } // support
} // ps
#endif
#endif // PS_SUPPORT_SINGLETON_FACTORY_H
