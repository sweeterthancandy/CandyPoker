#include "ps/eval/holdem_class_eval_cache_impl.h"

#include <fstream>
#include <boost/serialization/map.hpp>
#include <boost/serialization/vector.hpp>
#include <boost/serialization/array.hpp>
#include <boost/serialization/shared_ptr.hpp>
#include <boost/serialization/export.hpp>

namespace ps{
        bool holdem_class_eval_cache_impl::load(std::string const& name){
                std::ifstream is(name);
                if( ! is.is_open() ){
                        std::cerr << "Unable to open " << name << "\n";
                        return false;
                }
                boost::archive::text_iarchive ia(is);
                this->lock();
                ia >> *this;
                this->unlock();
                return true;
        }
        bool holdem_class_eval_cache_impl::save(std::string const& name){
                std::ofstream of(name);
                if( ! of.is_open() ){
                        std::cerr << "Unable to open " << name << "\n";
                        return false;
                }
                boost::archive::text_oarchive oa(of);
                this->lock();
                oa << *this;
                this->unlock();
                return true;
        }
namespace {
        int reg = ( holdem_class_eval_cache_factory::register_<holdem_class_eval_cache_impl>("main"),0);
} // anon
} // ps
