#ifndef PS_EVAL_BINARY_STRATEGY_DESCRIPTION_H
#define PS_EVAL_BINARY_STRATEGY_DESCRIPTION_H

#include "ps/base/cards.h"
#include <memory>
#include <vector>
#include <string>
#include <boost/iterator/indirect_iterator.hpp>

namespace ps{

        /*
                Need a way to abstract the strategy represention.
                
                For a hu game, we have a representation of a 2-vector,
                each of size 169, which a value \in [0,1] which represents
                the probabilty of pushing.

                For a three player game, we have a 6-vector of 169-vectors,
                which each value \in [0,1] which reprents of the probaily
                of pushing, given the previous action
         */
        struct binary_strategy_description{
                using strategy_impl_t = std::vector<Eigen::VectorXd>;

                // 
                virtual double sb()const=0;
                virtual double bb()const=0;
                virtual double eff()const=0;
                virtual size_t num_players()const=0;
                virtual size_t strat_vector_size()const=0;

                virtual std::string string_representation()const{
                        std::stringstream sstr;
                        sstr << num_players() << "-players ";
                        sstr << sb() << ":" << bb() << ":" << eff();
                        return sstr.str();
                }



                virtual strategy_impl_t make_inital_state()const=0;
                /*
                 *  We are constraied to only events which can be represented by a key,
                 *  for push/fold solving this is appropraite.
                 */

                struct event_decl{
                        virtual ~event_decl()=default;
                        virtual std::string key()const=0;
                        virtual void expected_value_given_event(Eigen::VectorXd& out, holdem_class_vector const& cv, double p = 1.0)const=0;
                        virtual double expected_value_of_event(binary_strategy_description const* desc, Eigen::VectorXd& out, holdem_class_vector const& cv, strategy_impl_t const& impl)const{
                                auto p = desc->probability_of_event(key(), cv, impl);
                                expected_value_given_event(out, cv, p);
                                return p;
                        }
                        double probability_of_event(binary_strategy_description* desc, holdem_class_vector const& cv, strategy_impl_t const& impl)const{
                                return desc->probability_of_event(key(), cv, impl);
                        }
                        virtual std::string to_string()const=0;
                };

                struct event_set : std::vector<event_decl const*>{
                        using event_iterator = boost::indirect_iterator<const_iterator>;
                        event_iterator event_begin()const{ return begin(); }
                        event_iterator event_end()const{ return end(); }
                };

                using event_vector = std::vector<std::shared_ptr<event_decl> >;
                using event_iterator = boost::indirect_iterator<event_vector::const_iterator>;
                event_iterator begin_event()const{ return events_.begin(); }
                event_iterator end_event()const{ return events_.end(); }

                // the ev GIVEN a certain deal
                //    result = \sum P(q) * E[q]
                Eigen::VectorXd expected_value_of_vector(event_set const& es, holdem_class_vector const& cv, strategy_impl_t const& impl)const{
                        Eigen::VectorXd vec(num_players()+1);
                        vec.fill(0);
                        double sigma = 0.0;
                        for(auto iter(es.begin()),end(es.end());iter!=end;++iter){
                                sigma += (*iter)->expected_value_of_event(this, vec, cv, impl);
                        }
                        vec /= sigma;
                        return vec;
                }
                virtual Eigen::VectorXd expected_value_by_class_id(size_t player_idx, strategy_impl_t const& impl)const=0;
                virtual double expected_value_for_class_id(size_t player_idx, holdem_class_id class_id, strategy_impl_t const& impl)const{
                        return expected_value_for_class_id_es(aux_event_set_, player_idx, class_id, impl);
                }
                virtual double expected_value_for_class_id_es(event_set const& es, size_t player_idx, holdem_class_id class_id, strategy_impl_t const& impl)const{
                        throw std::domain_error("not implemented");
                }
                virtual Eigen::VectorXd expected_value(strategy_impl_t const& impl)const=0;

                virtual double probability_of_event(std::string const& key, holdem_class_vector const& cv, strategy_impl_t const& impl)const=0;

                void check_probability_of_event(holdem_class_vector const& cv, strategy_impl_t const& impl)const{
                        double sigma = 0.0;
                        for(auto iter(begin_event()),end(end_event());iter!=end;++iter){
                                sigma += probability_of_event(iter->key(), cv, impl);
                        }
                        double epsilon = 1e-4;
                        if( std::fabs(sigma - 1.0) > epsilon ){
                                std::stringstream sstr;
                                sstr << std::fixed;
                                sstr << "bad events sigma=" << sigma;
                                throw std::domain_error(sstr.str());
                        }
                }

                struct strategy_decl{
                        strategy_decl(binary_strategy_description* self, size_t vec_idx, size_t player_idx, std::string const& desc, std::string const& action,
                                      std::vector<double> const& given)
                                :self_{self},
                                vec_idx_(vec_idx),
                                player_idx_(player_idx),
                                desc_(desc),
                                action_(action),
                                given_(given)
                        {
                                for(auto ei(self_->begin_event()),ee(self_->end_event());ei!=ee;++ei){
                                        if( ei->key().size() < action_.size()){
                                                continue;
                                        }
                                        if( ei->key().substr(0, action_.size()) == action_ ){
                                                events_.push_back(&*ei);
                                        }
                                }
                        }
                        size_t vector_index()const{ return vec_idx_; }
                        size_t player_index()const{ return player_idx_; }
                        std::string const& description()const{ return desc_; }
                        std::string const& action()const{ return action_; }
                        strategy_impl_t make_all_fold(strategy_impl_t const& impl)const{
                                auto result = impl;
                                result[vec_idx_].fill(0);
                                return result;
                        }
                        strategy_impl_t make_all_push(strategy_impl_t const& impl)const{
                                auto result = impl;
                                result[vec_idx_].fill(1);
                                return result;
                        }
                        using self_type = strategy_decl;
                        #if 0
                        self_type& add_event(event_decl const& event){
                                events_.push_back(&event);
                                return *this;
                        }
                        #endif
                        using event_vector = std::vector<event_decl const*>;
                        using event_iterator = boost::indirect_iterator<event_vector::const_iterator>;
                        event_iterator begin_event()const{ return events_.begin(); }
                        event_iterator end_event()const{ return events_.end(); }


                        double expected_value_for_class_id(holdem_class_id class_id, strategy_impl_t const& impl)const{
                                return self_->expected_value_for_class_id_es(events_, player_idx_, class_id, impl);

                        }
                private:
                        binary_strategy_description* self_;
                        size_t vec_idx_;
                        size_t player_idx_;
                        std::string desc_;
                        std::string action_;
                        // the way that the strategy vector is layed out, we can make 
                        // some optimization on the game tree
                        std::vector<double> given_;

                        event_set events_;
                };
                using strategy_vector = std::vector<strategy_decl>;
                using strategy_iterator = strategy_vector::const_iterator;
                strategy_iterator begin_strategy()const{ return strats_.begin(); }
                strategy_iterator end_strategy()const{ return strats_.end(); }


                static std::shared_ptr<binary_strategy_description> make_hu_description(double sb, double bb, double eff);
                static std::shared_ptr<binary_strategy_description> make_three_player_description(double sb, double bb, double eff);

        protected:
                void finish(){
                        for(auto const& ev : events_){
                                aux_event_set_.push_back(ev.get());
                        }
                }
                event_vector events_;
                strategy_vector strats_;
                event_set aux_event_set_;

        };

} // end namespace ps

#endif // PS_EVAL_BINARY_STRATEGY_DESCRIPTION_H
