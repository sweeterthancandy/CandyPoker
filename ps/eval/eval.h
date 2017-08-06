#ifndef PS_EVAL_H
#define PS_EVAL_H

#include <array>


#include "ps/base/cards.h"
#include "ps/support/singleton_factory.h"

namespace ps{

struct ranking{
        ranking():
                rank_{0},
                name_{"__invalid__"}
        {}
        explicit ranking(int r, std::string n):
                rank_{r},
                name_{std::move(n)}
        {}
        ranking& assign(int r, std::string n){
                rank_ = r;
                name_ = std::move(n);
                return *this;
        }
        auto rank()const{ return rank_; }
        auto const& name()const{ return name_; }

        #if 0
        bool operator<(ranking const& l_param,
                       ranking const& r_param){
                return l_param.rank() < r_param.rank();
        }
        #endif

        operator int()const{ return this->rank(); }

        friend std::ostream& operator<<(std::ostream& ostr, ranking const& self){
                return ostr 
                        << "{ \"rank\":" << self.rank() 
                        << ", \"name\":" << self.name()
                        << "}";
        }
private:
        int rank_;
        std::string name_;
};

struct evaluater{
        virtual ~evaluater()=default;
        //virtual ranking const& rank(std::vector<long> const& cards)const;
        virtual ranking const& rank(long a, long b, long c, long d, long e)const=0;
        virtual ranking const& rank(long a, long b, long c, long d, long e, long f)const=0;
        virtual ranking const& rank(long a, long b, long c, long d, long e, long f, long g)const=0;
};

using evaluater_factory = support::singleton_factory<evaluater>;

} // namespace ps

#endif // #ifndef PS_EVAL_H
