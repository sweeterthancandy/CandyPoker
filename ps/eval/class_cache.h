#ifndef PS_EVAL_CLASS_CACHE_H
#define PS_EVAL_CLASS_CACHE_H

#include <vector>
#include <boost/functional/hash.hpp>
#include <unordered_map>
#include <map>

#include "ps/base/cards.h"
#include "ps/support/persistent.h"

/*
 * This case is just to store the result of computation as a vector of equity.
 * Computing all-in EV is very slow, so for solving simulations we want to 
 * store the cable to all-in ev for several players.
 */

namespace boost{
namespace serialization{
        struct access;
} // end namespace serialization
} // end namespace boost

namespace ps{


struct class_cache : boost::noncopyable{
        void add(std::vector<holdem_class_id> vec, std::vector<double> equity){
                cache_.emplace(std::move(vec), std::move(equity));
        }
        size_t size()const{ return cache_.size(); }
	std::vector<double> const* Lookup(std::vector<holdem_class_id> const& vec)const{
                auto iter = cache_.find(vec);
                if( iter == cache_.end())
                        return nullptr;
                return &iter->second;
	}
        std::vector<double> const* fast_lookup_no_perm(holdem_class_vector const& vec)const{
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

        static void create(size_t n, class_cache* cache, std::string const& file_name, size_t num_threads);

        using implementation_type = std::map<std::vector<holdem_class_id>, std::vector<double> >;
        using iterator = implementation_type::const_iterator;

        iterator begin()const{ return cache_.begin(); }
        iterator end  ()const{ return cache_.end  (); }
	
private:
        friend class boost::serialization::access;
        template<class Archive>
        void serialize(Archive & ar, const unsigned int version){
                ar & cache_;
        }
private:
        std::map<std::vector<holdem_class_id>, std::vector<double> > cache_;
};

extern support::persistent_memory_decl<class_cache> Memory_ClassCache;

// TODO, look at this again
//

// I saw only a 3% increate with this
struct hash_class_cache{
        hash_class_cache(){
                std::string cache_name{".cc.bin"};
                cc.load(cache_name);
                for(auto const& p : cc){
                        //S.emplace(p.first, p.second);
                        S[p.first] = p.second;
                }
        }
        std::vector<double> const* fast_lookup_no_perm(holdem_class_vector const& vec)const{
                auto iter = S.find(vec);
                if( iter == S.end())
                        return nullptr;
                #if 0
                if( iter->first != vec ){
                        throw std::domain_error("ne");
                }
                if( iter->second != *cc.fast_lookup_no_perm(vec) )
                        throw std::domain_error("nee");
                #endif
                return &iter->second;
        }
private:
        std::unordered_map<
                holdem_class_vector,
                std::vector<double>,
                boost::hash<holdem_class_vector>
        > S;
        class_cache cc;
};



} // end namespace ps

#endif // PS_EVAL_CLASS_CACHE_H
