#ifndef PS_EVAL_RANKING_DECL_H
#define PS_EVAL_RANKING_DECL_H


#include "ps/base/cards.h"
#include <boost/noncopyable.hpp>

#include "ps/support/singleton_factory.h"
#include "ps/eval/rank_decl.h"

namespace ps{

        using ranking_t = std::uint16_t;

        struct ranking_decl{
                // need to be able to create dummy ones
                ranking_decl():
                        rank_{1},
                        flush_{false},
                        name_{"__invalid__"}
                {}
                explicit ranking_decl(ranking_t r, bool f, std::string n,
                                      rank_vector rv):
                        rank_{r},
                        flush_{f},
                        name_{std::move(n)},
                        rv_{std::move(rv)}
                {}
                ranking_decl& assign(ranking_t r, bool f, std::string n){
                        rank_ = r;
                        flush_ = f;
                        name_ = std::move(n);
                        return *this;
                }
                auto rank()const{ return rank_; }
                auto is_flush()const{ return flush_; }
                auto const& name()const{ return name_; }
                auto const& get_rank_vector()const{ return rv_; }

                #if 0
                bool operator<(ranking_decl const& l_param,
                               ranking_decl const& r_param){
                        return l_param.rank() < r_param.rank();
                }
                #endif

                operator int()const{ return this->rank(); }

                friend std::ostream& operator<<(std::ostream& ostr, ranking_decl const& self){
                        return ostr 
                                << "{ \"rank\":" << self.rank() 
                                << ", \"is_flush\":" << self.is_flush()
                                << ", \"name\":" << self.name()
                                << ", \"rank_vec\":" << self.get_rank_vector()
                                << "}";
                }
        private:
                ranking_t rank_;
                bool flush_;
                std::string name_;
                rank_vector rv_;
        };
        
        struct rank_world : boost::noncopyable{
                rank_world();
                auto const& operator[](size_t idx)const{
                        return world_.at(idx);
                        //return world_[idx];
                }
                auto begin()const{ return world_.begin(); }
                auto end()const{ return world_.end(); }
        private:
                std::vector<ranking_decl> world_;
        };

        using rank_word_factory = support::singleton_factory<rank_world>;

} // ps

#endif // PS_EVAL_RANKING_DECL_H
