#include "ps/eval/class_equity_evaluator_cache.h"

#if 0
#include <fstream>
#include <boost/archive/text_oarchive.hpp>
#include <boost/archive/text_iarchive.hpp>
#include <boost/serialization/map.hpp>
#include <boost/serialization/vector.hpp>
#include <boost/serialization/array.hpp>
#include <boost/serialization/shared_ptr.hpp>
#endif

namespace ps{
namespace{
        int reg = ( class_equity_evaluator_factory::register_<class_equity_evaluator_cache>("cached"), 0);
} // anon

#if 0
bool class_equity_evaluator_cache::load(std::string const& name){
        std::ifstream is(name);
        if( ! is.is_open() )
                return false;
        boost::archive::text_iarchive ia(is);
        ia >> *this;
        return true;
}
bool class_equity_evaluator_cache::save(std::string const& name)const{
        std::ofstream of(name);
        boost::archive::text_oarchive oa(of);
        oa << *this;
        return true;
}
#endif

} // ps
