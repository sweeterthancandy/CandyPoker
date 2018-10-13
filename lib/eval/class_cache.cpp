#include "ps/eval/class_cache.h"


#include <fstream>
#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/text_oarchive.hpp>
#include <boost/serialization/base_object.hpp>
#include <boost/serialization/utility.hpp>
#include <boost/serialization/map.hpp>
#include <boost/serialization/vector.hpp>
#include <boost/serialization/assume_abstract.hpp>
	
namespace ps{
void class_cache::save(std::string const& filename){
        // make an archive
        std::ofstream ofs(filename);
        boost::archive::text_oarchive oa(ofs);
        oa << *this;
}

void class_cache::load(std::string const& filename)
{
        // open the archive
        std::ifstream ifs(filename);
        boost::archive::text_iarchive ia(ifs);

        // restore the schedule from the archive
        cache_.clear();
        ia >> *this;
}
} // end namespace ps
