#ifndef PS_SUPPORT_PERSISTENT_HPP
#define PS_SUPPORT_PERSISTENT_HPP


#include <functional>
#include <vector>
#include <boost/intrusive/list.hpp>
#include "ps/detail/reinterpret_pointer_cast.h"

namespace ps{
namespace support{

struct persistent_memory_base : boost::intrusive::list_base_hook<>{

        ///////////////////////////////////////////////////////////////////////////////////////////
        // impl base 
        struct impl_base{
                virtual ~impl_base()=default;
                virtual std::string name()const=0;
                #if 0
                virtual std::shared_ptr<void> make()const=0;
                virtual std::shared_ptr<void> load(std::string const& path)=0;
                virtual void save(std::shared_ptr<void> ptr, std::string const& path)const=0;
                #endif
                virtual void const* ptr()const=0;
                virtual void display(std::ostream& ostr)const{}
        };

        ///////////////////////////////////////////////////////////////////////////////////////////
        // controller
        struct controller{
                virtual ~controller()=default;
                virtual std::string alloc_path(std::string const& token)=0;
                virtual void begin_load(std::string const& path)=0;
                virtual void end_load(std::string const& path)=0;
        };
        struct default_controller : controller{
                virtual std::string alloc_path(std::string const& token)override{
                        return ".persistent." + token;
                }
                virtual void begin_load(std::string const& path){
                        std::cerr << "begin_load(" << path << ")\n";
                }
                virtual void end_load(std::string const& path){
                        std::cerr << "end_load(" << path << ")\n";
                }
        };

        using controller_vector = std::vector<std::shared_ptr<controller> >;
        static controller* get_default_controller(){
                static auto ptr = new default_controller;
                return ptr;
        }
        static controller_vector* get_controller_vec(){
                static auto ptr = new controller_vector;
                ptr->push_back(std::shared_ptr<controller>(get_default_controller(),[](auto){}));
                return ptr;
        }
        static void push_controller(std::shared_ptr<controller> ctrl){
                get_controller_vec()->push_back(ctrl);
        }
        static controller* get_controller(){
                return get_controller_vec()->back().get();
        }

        
        
        ///////////////////////////////////////////////////////////////////////////////////////////
        // memory list
        using memory_list = boost::intrusive::list<persistent_memory_base>;
        static memory_list* get_memory_list(){
                static auto ptr = new memory_list;
                return ptr;
        }
        using decl_iterator = memory_list::iterator;
        static decl_iterator begin_decl(){ return get_memory_list()->begin(); }
        static decl_iterator end_decl(){ return get_memory_list()->end(); }
        persistent_memory_base()
        {
                get_memory_list()->push_back(*this);
        }

        virtual std::string name()const=0;
        virtual void display(std::ostream& ostr = std::cout)const=0;
};

// treat T as an incomplete type
template<class T>
struct persistent_memory_decl : persistent_memory_base{
        using te_maker_type = std::function<std::shared_ptr<T>()>;

        explicit
        persistent_memory_decl(std::unique_ptr<impl_base> pimpl)
                : pimpl_{std::move(pimpl)}
        {}
        T const* get()const{
                return reinterpret_cast<T const*>(pimpl_->ptr());
        }
        T const* operator->()const{
                return reinterpret_cast<T const*>(pimpl_->ptr());
        }
        T const& operator*()const{
                return *reinterpret_cast<T const*>(pimpl_->ptr());
        }

        virtual std::string name()const override{
                return pimpl_->name();
        }
        virtual void display(std::ostream& ostr)const override{
                pimpl_->display(ostr);
        }
private:
        std::unique_ptr<impl_base> pimpl_;
};
} // end namespace support
} // end namespace ps

#endif // PS_SUPPORT_PERSISTENT_HPP
