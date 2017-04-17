#ifndef PS_NUMERIC_H
#define PS_NUMERIC_H


namespace ps{
        namespace numeric{

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

                                bnu::matrix<int> A( hands_.size(), 4 );
                                for( size_t i{0}; i!= ctx.get_players().size(); ++i){
                                        auto const& p{ctx.get_players()[i]};
                                        A(i, 0) = p.wins();
                                        A(i, 1) = p.draws();
                                        A(i, 2) = p.sigma();
                                }
                                axpy_prod(factor_, A, result_, false);
                                return result_;
                        }

                        void debug(){
                                eval();
                                PRINT(*this);

                        }
                private:
                        std::vector<frontend::hand> hands_;
                        bnu::matrix<int> factor_;
                        bnu::matrix<int> term_;
                        bnu::matrix<int> result_;
                };

                struct work_scheduler{
                        explicit work_scheduler(size_t num_players):num_players_{num_players}{}
                        void decl( std::vector<int> const& perm, std::vector<frontend::hand>const& hands){
                                auto hash{ symbolic_primitive::make_hash(hands) };
                                if( work_.count(hash) == 0 ){
                                        work_.insert(std::make_pair(hash, underlying_work{hands} ) );
                                }
                                auto iter = work_.find(hash);
                                iter->second.append_permutation_matrix(perm);
                        }
                        void debug(){
                                for( auto & w: work_ ){
                                        w.second.debug();
                                }
                        }
                        bnu::matrix<int> compute(){
                                bnu::matrix<int> result{num_players_, 4, 0};
                                for( auto & w: work_ ){
                                        result += w.second.get();
                                }
                                return result;
                        }
                private:
                        size_t num_players_; 
                        std::map<std::string, underlying_work> work_;
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
