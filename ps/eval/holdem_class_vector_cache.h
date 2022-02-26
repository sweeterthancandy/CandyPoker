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
#ifndef PS_EVAL_HOLDEM_CLASS_VECTOR_CACHE_H
#define PS_EVAL_HOLDEM_CLASS_VECTOR_CACHE_H

#include "ps/base/cards.h"
#include "ps/support/persistent.h"

namespace boost{
namespace serialization{
        class access;
} // end namespace serialization
} // end namespace boost

namespace ps{

struct holdem_class_vector_cache_item{
        friend std::ostream& operator<<(std::ostream& ostr, holdem_class_vector_cache_item const& self){
                ostr << "cv = " << self.cv;
                ostr << ", count = " << self.count;
                ostr << ", prob = " << self.prob;
                ostr << ", ev = " << detail::to_string(self.ev);
                return ostr;
        }
        holdem_class_vector cv;
        size_t count{0};
        double prob{0};
        std::vector<double> ev;
private:
        friend class boost::serialization::access;
        template<class Archive>
        void serialize(Archive & ar, const unsigned int version){
                ar & cv;
                ar & count;
                ar & prob;
                ar & ev;
        }
};
struct holdem_class_vector_cache_item_pair{
        holdem_class_id cid;
        std::vector<holdem_class_vector_cache_item> vec;
private:
        friend class boost::serialization::access;
        template<class Archive>
        void serialize(Archive & ar, const unsigned int version){
                ar & cid;
                ar & vec;
        }
};
using holdem_class_vector_cache = std::vector<holdem_class_vector_cache_item>;

using holdem_class_vector_pair_cache = std::vector<holdem_class_vector_cache_item_pair>;

//extern support::persistent_memory_decl<holdem_class_vector_pair_cache> Memory_ThreePlayerClassVector;
//extern support::persistent_memory_decl<holdem_class_vector_pair_cache> Memory_TwoPlayerClassVector;

support::persistent_memory_decl<holdem_class_vector_pair_cache> const& get_Memory_ThreePlayerClassVector();
support::persistent_memory_decl<holdem_class_vector_pair_cache> const& get_Memory_TwoPlayerClassVector();

} // end namespace ps



#endif // PS_EVAL_HOLDEM_CLASS_VECTOR_CACHE_H
