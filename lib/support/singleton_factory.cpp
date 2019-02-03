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
#include "ps/support/singleton_factory.h"

namespace ps{
namespace support{
namespace detail{

        void* singleton_controller::get_or_null(boost::typeindex::type_index const& ti, std::string const& name){
                auto ti_iter = type_inst_map_.find(ti);
                if( ti_iter == type_inst_map_.end())
                        return nullptr;
                auto maker_iter = ti_iter->second.find(name);
                if( maker_iter == ti_iter->second.end())
                        return nullptr;
                return maker_iter->second.get();
        }
        void singleton_controller::register_(boost::typeindex::type_index const& ti, std::string name, maker_t maker) {
                type_inst_map_[ti].emplace(std::move(name), std::move(maker));
        }
        singleton_controller* singleton_controller::get_inst() {
                static singleton_controller* mem = nullptr;
                if (mem == nullptr)
                        mem = new singleton_controller;
                return mem;
        }

} // detail
} // support
} // ps
