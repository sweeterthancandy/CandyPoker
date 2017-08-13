#ifndef EQUITY_EVAL_RESULT_H
#define EQUITY_EVAL_RESULT_H

#include <vector>
#include <ostream>
#include <numeric>

#include "ps/eval/equity_breakdown.h"
#include "ps/support/array_view.h"

#include <boost/archive/tmpdir.hpp>

#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/text_oarchive.hpp>

#include <boost/serialization/base_object.hpp>
#include <boost/serialization/utility.hpp>
#include <boost/serialization/list.hpp>
#include <boost/serialization/assume_abstract.hpp>

namespace ps{

struct equity_breakdown_player_matrix : equity_breakdown_player{
        explicit equity_breakdown_player_matrix(size_t n, size_t sigma, support::array_view<size_t> data);

        double equity()const override;
        // nwin(0) -> wins
        // nwin(1) -> draws to split pot 2 ways
        // nwin(2) -> draws to split pot 3 ways
        // ...
        size_t nwin(size_t idx)const override;
        size_t win()const override;
        size_t draw()const override;
        size_t lose()const override;
        size_t sigma()const override;
private:
        size_t n_;
        size_t sigma_;
        support::array_view<size_t> data_;
};


struct equity_breakdown_matrix : equity_breakdown{

        equity_breakdown_matrix();
        
        equity_breakdown_matrix(equity_breakdown_matrix const& that);
        equity_breakdown_matrix(equity_breakdown_matrix&&)=delete;
        equity_breakdown_matrix& operator=(equity_breakdown_matrix const&)=delete;
        equity_breakdown_matrix& operator=(equity_breakdown_matrix&&)=delete;

        // only if all convertiable to size_t
        equity_breakdown_matrix(size_t n);
        // copy
        equity_breakdown_matrix(equity_breakdown const& that);
        equity_breakdown_matrix(equity_breakdown const& that, std::vector<int> const& perm);
        size_t const& data_access(size_t i, size_t j)const;
        size_t& data_access(size_t i, size_t j);
        size_t const* data()const;


        size_t sigma()const override;
        size_t& sigma();

        equity_breakdown_player const& player(size_t idx)const override;

        size_t n()const override;
        
        template<class Archive>
        void save(Archive &ar, const unsigned int version)const
        {
                ar & sigma_;
                ar & n_;
                ar & data_;
        }
        template<class Archive>
        void load(Archive &ar, const unsigned int version)
        {
                ar & sigma_;
                ar & n_;
                ar & data_;
                players_.clear();
                for(size_t i=0;i!=n_;++i){
                        players_.emplace_back(n_,
                                              sigma_,
                                              support::array_view<size_t>(&data_[0] + i * n_, n_ ));
                }

        }
        BOOST_SERIALIZATION_SPLIT_MEMBER()
        
private:
        size_t sigma_ = 0;
        size_t n_;

        // access float, so can have a static view for all 
        //  2,3,...9 etc without injeritace (see view_t)
        std::vector<size_t> data_;

        // need to allocate these
        std::vector< equity_breakdown_player_matrix> players_;
};
        
struct equity_breakdown_matrix_aggregator : equity_breakdown_matrix{

        explicit equity_breakdown_matrix_aggregator(size_t n);

        void append(equity_breakdown const& breakdown);
        void append_perm(equity_breakdown const& breakdown, std::vector<int> const& perm);
        void append_matrix(equity_breakdown const& breakdown, std::vector<int> const& mat);
};


} // ps

#endif // EQUITY_EVAL_RESULT_H
