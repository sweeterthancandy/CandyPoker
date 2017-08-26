#include "ps/support/singleton_factory.h"

namespace ps{
namespace support{
namespace detail{

        void* singleton_controller::get_or_null(boost::typeindex::type_index const& ti, std::string const& name){
                // find maker for the specific type
                auto ti_iter = type_inst_map_.find(ti);
                if( ti_iter == type_inst_map_.end())
                        return nullptr;
                // find item for specific name
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
