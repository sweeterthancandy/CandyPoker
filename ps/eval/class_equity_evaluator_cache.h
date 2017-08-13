#ifndef PS_EVAL_EQUITY_CALCULATOR_CACHE_H
#define PS_EVAL_EQUITY_CALCULATOR_CACHE_H

#include "ps/base/algorithm.h"
#include "ps/eval/equity_future.h"
#include "ps/eval/equity_evaluator.h"
#include "ps/eval/equity_breakdown_matrix.h"
#include "ps/eval/class_equity_evaluator.h"

#include <fstream>

#include <boost/serialization/map.hpp>
#include <boost/serialization/vector.hpp>
#include <boost/serialization/array.hpp>
#include <boost/serialization/shared_ptr.hpp>
#include <boost/serialization/export.hpp>

#include <boost/range/algorithm.hpp>

namespace ps{


struct class_equity_evaluator_cache : class_equity_evaluator{
        class_equity_evaluator_cache()
                : impl_{ &class_equity_evaluator_factory::get("proc") }
        {
        }
        std::shared_ptr<equity_breakdown> evaluate(holdem_class_vector const& players)const override{
                // first make permuation
                std::vector< std::tuple< size_t, holdem_class_id> > aux;
                for( size_t i = 0;i != players.size();++i){
                        aux.emplace_back( i, players[i]);
                }
                boost::sort( aux, [](auto const& l, auto const& r){
                        return std::get<1>(l) < std::get<1>(r);
                });
                std::vector<int> perm;
                holdem_class_vector players_perm;
                for( auto const& item : aux ){
                        perm.push_back( std::get<0>(item) );
                        players_perm.push_back( std::get<1>(item) );
                }
                auto iter = cache_.find(players_perm);
                if( iter != cache_.end() ){
                        return std::make_shared<equity_breakdown_permutation_view>(
                                iter->second,
                                std::move(perm)
                        );
                }
                auto ret = impl_->evaluate(players_perm);
                auto copy = std::make_shared<equity_breakdown_matrix>(*ret);
                cache_.emplace( players_perm, copy);
                return evaluate(players);
        }
        bool load(std::string const& name){
                std::ifstream is(name);
                if( ! is.is_open() )
                        return false;
                boost::archive::text_iarchive ia(is);
                ia >> *this;
                return true;
        }
        bool save(std::string const& name)const{
                std::ofstream of(name);
                if( ! of.is_open() )
                        return false;
                boost::archive::text_oarchive oa(of);
                oa << *this;
                return true;
        }
        template<class Archive>
        void serialize(Archive& ar, unsigned int){
                ar & cache_;
        }
private:
        mutable std::map< holdem_class_vector, std::shared_ptr<equity_breakdown_matrix> > cache_;
        class_equity_evaluator const* impl_;
};


} // ps
#endif // PS_EVAL_EQUITY_CALCULATOR_CACHE_H
