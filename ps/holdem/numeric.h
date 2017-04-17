#ifndef PS_NUMERIC_H
#define PS_NUMERIC_H

#include <thread>
#include <mutex>

namespace ps{
        namespace numeric{
                
                struct result_type{
                        explicit result_type(size_t n):
                                nat_mat(n,3,0),
                                real_mat(n,1,0)
                        {}
                        friend std::ostream& operator<<(std::ostream& ostr, result_type const& self){
                                return ostr << "nat_mat = " << self.nat_mat << ", real = " << self.real_mat;
                        }

                private:
                        
                        bnu::matrix<std::uint64_t> nat_mat;
                        bnu::matrix<long double>   real_mat;
                };

                using matrix_type = bnu::matrix<std::uint64_t>;

                struct underlying_work{

                        explicit underlying_work(std::vector<frontend::hand> const& hands)
                                : hands_{hands}
                                , factor_(hands_.size(), hands_.size(), 0)
                                , result_(hands_.size(), 4, 0)
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
                                        << " = " << self.result_
                                        << " # " << symbolic_primitive::make_hash(self.hands_);
                        }
                        auto get(){
                                ps::equity_context ctx;
                                ps::equity_calc eq;

                                for( auto h : hands_ ){
                                        ctx.add_player(h);
                                }
                                eq.run( ctx );

                                size_t idx{0};

                                matrix_type A( hands_.size(), 4 );
                                for( size_t i{0}; i!= ctx.get_players().size(); ++i){
                                        auto const& p{ctx.get_players()[i]};
                                        A(i, 0) = p.wins();
                                        A(i, 1) = p.draws();
                                        A(i, 2) = p.sigma();
                                        A(i, 3) = p.equity() * p.sigma() * 1000;
                                }
                                axpy_prod(factor_, A, result_, false);


                                return result_;
                        }

                        void debug(){
                                PRINT(*this);

                        }
                private:
                        std::vector<frontend::hand> hands_;
                        matrix_type factor_;
                        matrix_type term_;
                        matrix_type result_;
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
                        matrix_type compute(){
                                std::vector<std::function<void()> > work;
                                matrix_type result{num_players_, 4, 0};
                                std::mutex mtx;
                                std::vector<std::thread> workers;
                                for( auto& w: children_ ){
                                        auto _ = [&]()mutable{
                                                auto tmp{ w.second.get() };
                                                mtx.lock();
                                                result += tmp;
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
                                        result(i,3) = result(i,3) / result(i,2);
                                }
                                return result;
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
