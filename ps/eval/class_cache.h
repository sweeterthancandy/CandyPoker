#ifndef PS_EVAL_CLASS_CACHE_H
#define PS_EVAL_CLASS_CACHE_H

#include <vector>
#include "ps/base/cards.h"
#include "ps/eval/instruction.h"
#include "ps/eval/computer.h"
#include "ps/eval/computer_mask.h"

namespace boost{
namespace serialization{
        struct access;
} // end namespace serialization
} // end namespace boost

namespace ps{


struct class_cache{
        void add(std::vector<holdem_class_id> vec, std::vector<double> equity){
                cache_.emplace(std::move(vec), std::move(equity));
        }
	std::vector<double> const* Lookup(std::vector<holdem_class_id> const& vec)const{
                auto iter = cache_.find(vec);
                if( iter == cache_.end())
                        return nullptr;
                return &iter->second;
	}
        Eigen::VectorXd LookupVector(holdem_class_vector const& vec)const{
                #if 0
                if( std::is_sorted(vec.begin(), vec.end()) ){
                        Eigen::VectorXd tmp(vec.size());
                        auto ptr = Lookup(vec);
                        BOOST_ASSERT(pyt);
                        for(size_t idx=0;idx!=vec.size();++idx){
                                tmp(idx) = vec[idx];
                        }
                        return tmp;
                }
                #endif
                // find the permuation index
                std::array<
                        std::tuple<size_t, holdem_class_id>
                        , 9
                > aux;
                for(size_t idx=0;idx!=vec.size();++idx){
                        aux[idx] = std::make_tuple(idx, vec[idx]);
                }
                std::sort(aux.begin(), aux.begin() + vec.size(), [](auto const& l, auto const& r){
                          return std::get<1>(l) < std::get<1>(r);
                });

                // I think this is quicker than copying from aux
                auto copy = vec;
                std::sort(copy.begin(), copy.end());
                        
                // find the underlying
                auto ptr = Lookup(copy);
                BOOST_ASSERT(ptr);

                // copy to a vector
                Eigen::VectorXd tmp(vec.size());
                for(size_t idx=0;idx!=vec.size();++idx){
                        auto mapped_idx = std::get<0>(aux[idx]);
                        tmp(mapped_idx) = (*ptr)[idx];
                }
                return tmp;
        }
	void save(std::string const& filename);
	void load(std::string const& filename);

        static void create(size_t n, class_cache* cache, std::string const& file_name);
	
private:
        friend class boost::serialization::access;
        template<class Archive>
        void serialize(Archive & ar, const unsigned int version){
                ar & cache_;
        }
private:
        std::map<std::vector<holdem_class_id>, std::vector<double> > cache_;
};

} // end namespace ps

#endif // PS_EVAL_CLASS_CACHE_H
