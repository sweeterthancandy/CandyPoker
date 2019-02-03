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
                        rank_{0},
                        cat_{HR_NotAHandRank},
                        flush_{false},
                        name_{"__invalid__"}
                {}
                explicit ranking_decl(ranking_t r, hand_rank_category cat, bool f, std::string n,
                                      rank_vector rv):
                        rank_{r},
                        cat_{cat},
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
                hand_rank_category category()const{ return cat_; }

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
                                << ", \"category\":" << self.category()
                                << ", \"is_flush\":" << self.is_flush()
                                << ", \"name\":" << self.name()
                                << ", \"rank_vec\":" << self.get_rank_vector()
                                << "}";
                }
        private:
                ranking_t rank_;
                hand_rank_category cat_;
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
