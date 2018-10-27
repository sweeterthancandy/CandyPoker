#ifndef PS_EVAL_HOLDEM_CLASS_VECTOR_CACHE_H
#define PS_EVAL_HOLDEM_CLASS_VECTOR_CACHE_H

#include "ps/base/cards.h"
#include "ps/support/persistent.h"

namespace boost{
namespace serialization{
        struct access;
} // end namespace serialization
} // end namespace boost

namespace ps{

struct holdem_class_vector_cache_item{
        friend std::ostream& operator<<(std::ostream& ostr, holdem_class_vector_cache_item const& self){
                ostr << "cv = " << self.cv;
                ostr << ", count = " << self.count;
                ostr << ", prob = " << self.prob;
                return ostr;
        }
        holdem_class_vector cv;
        size_t count{0};
        double prob{0};
private:
        friend class boost::serialization::access;
        template<class Archive>
        void serialize(Archive & ar, const unsigned int version){
                ar & cv;
                ar & count;
                ar & prob;
        }
};
using holdem_class_vector_cache = std::vector<holdem_class_vector_cache_item>;

extern support::persistent_memory_decl<holdem_class_vector_cache> Memory_ThreePlayerClassVector;

} // end namespace ps



#endif // PS_EVAL_HOLDEM_CLASS_VECTOR_CACHE_H
