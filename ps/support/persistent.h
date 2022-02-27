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
#ifndef PS_SUPPORT_PERSISTENT_HPP
#define PS_SUPPORT_PERSISTENT_HPP


#include <functional>
#include <list>
#include <vector>
#include "ps/base/cards_fwd.h"
#include "ps/detail/reinterpret_pointer_cast.h"
#include <boost/iterator/indirect_iterator.hpp>

namespace ps{
namespace support{

struct persistent_memory_base{

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
                        PS_LOG(trace) << "begin_load(" << path;
                }
                virtual void end_load(std::string const& path){
                        PS_LOG(trace) << "end_load(" << path;
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
        using memory_list = std::list<persistent_memory_base*>;
        static memory_list* get_memory_list(){
                static auto ptr = new memory_list;
                return ptr;
        }
        using decl_iterator = boost::indirect_iterator<memory_list::iterator>;
        static decl_iterator begin_decl(){ return get_memory_list()->begin(); }
        static decl_iterator end_decl(){ return get_memory_list()->end(); }
        persistent_memory_base()
        {
                get_memory_list()->push_back(this);
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
