#ifndef PS_NUMERIC_H
#define PS_NUMERIC_H

#include <thread>
#include <mutex>
#include <vector>

#include "ps/equity_calc.h"

namespace ps{
        namespace numeric{

                struct underlying_work{

                        explicit underlying_work(std::vector<frontend::hand> const& hands)
                                : hands_{hands}
                                , factor_(hands_.size(), hands_.size(), 0)
                        {}


                        void append_permutation_matrix(std::vector<int> const& perm){
                                for(size_t i=0;i!=perm.size();++i){
                                        ++factor_(i, perm[i]);
                                }
                        }
                        friend std::ostream& operator<<(std::ostream& ostr, underlying_work const& self){
                                for(size_t i{0};i!=self.hands_.size();++i){
                                        if( i != 0 ) ostr << " vs ";
                                        ostr << self.hands_[i];
                                }
                                return ostr << " * " << self.factor_
                                        << " # " << symbolic_primitive::make_hash(self.hands_);
                        }
                        auto get(){
                                ps::equity_calc eq;
                                std::vector<id_type> players;

                                for( auto h : hands_ ){
                                        players.push_back(h.get());
                                }
                                result_type ret( hands_.size() );
                                eq.run( ret, players );

                                size_t idx{0};

                                axpy_prod(factor_, ret.nat_mat, ret.nat_mat, false);
                                axpy_prod(factor_, ret.rel_mat, ret.rel_mat, false);

                                return ret;
                        }

                        void debug(){
                                PRINT(*this);

                        }
                private:
                        std::vector<frontend::hand> hands_;
                        result_type::nat_matrix_type factor_;
                        result_type::nat_matrix_type term_;
                };

                struct work_scheduler{
                        explicit work_scheduler(size_t num_players):num_players_{num_players}{}
                        void decl( std::vector<int> const& perm, std::vector<frontend::hand>const& hands){
                                auto hash{ symbolic_primitive::make_hash(hands) };
                                if( children_.count(hash) == 0 ){
                                        children_.insert(std::make_pair(hash, underlying_work{hands} ) );
                                }
                                auto iter = children_.find(hash);
                                iter->second.append_permutation_matrix(perm);
                        }
                        void debug(){
                                for( auto & w: children_ ){
                                        w.second.debug();
                                }
                        }
                        auto compute(){
                                std::vector<std::function<void()> > work;
                                result_type ret( num_players_);
                                std::mutex mtx;
                                std::vector<std::thread> workers;
                                for( auto& w: children_ ){
                                        auto _ = [&]()mutable{
                                                auto tmp{ w.second.get() };
                                                mtx.lock();
                                                ret += tmp;
                                                mtx.unlock();
                                        };
                                        work.emplace_back(_);
                                }
                                for(size_t i=0;i!= std::thread::hardware_concurrency();++i){
                                        auto _ = [&]()mutable{
                                                for(;;){
                                                        mtx.lock();
                                                        if( work.empty()){
                                                                mtx.unlock();
                                                                break;
                                                        }
                                                        auto w = work.back();
                                                        work.pop_back();
                                                        mtx.unlock();
                                                        w();
                                                }
                                        };
                                        workers.emplace_back(_);
                                }
                                for( auto& t : workers){
                                        t.join();
                                }
                                for( size_t i{0}; i!= num_players_; ++i){
                                        ret.rel_mat(i,0) /= ret.nat_mat(i,2);
                                }
                                return ret;
                        }
                private:
                        size_t num_players_; 
                        std::map<std::string, underlying_work> children_;
                };
        } // numeric
        
        namespace transforms{
                struct work_generator{
                        explicit work_generator(numeric::work_scheduler& sched):sched_{&sched}{}
                        bool operator()(symbolic_computation::handle& ptr)const{
                                if( ptr->get_kind() != symbolic_computation::Kind_Symbolic_Player_Perm )
                                        return false;
                                auto aux_ptr{ reinterpret_cast<symbolic_player_perm*>(ptr.get()) };
                                assert( aux_ptr->get_children().size() == 1 && "unexpected");
                                auto child_ptr = aux_ptr->get_children().front();
                                auto aux_child_ptr{ reinterpret_cast<symbolic_primitive*>(child_ptr.get())};

                                sched_->decl(
                                        aux_ptr->get_player_perm(),
                                        aux_child_ptr->get_hands());
                                return false;
                        }
                private:
                        numeric::work_scheduler* sched_;
                };
        } // transforms
} // os

#endif // PS_NUMERIC_H
