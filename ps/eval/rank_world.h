#ifndef PS_RANK_WORLD_H
#define PS_RANK_WORLD_H

#include <boost/noncopyable.hpp>

#include "ps/support/singleton_factory.h"
#include "ps/eval/rank_decl.h"

namespace ps{


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

#endif // PS_RANK_WORLD_H
