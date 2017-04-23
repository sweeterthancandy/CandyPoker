#ifndef PS_TRANSFORMS_H
#define PS_TRANSFORMS_H

#include <future>
#include <boost/timer/timer.hpp>

#include "ps/detail/print.h"
#include "ps/detail/work_schedular.h"

#include "ps/symbolic.h"

namespace ps{

        struct symbolic_transform{

                explicit symbolic_transform(std::string name):name_{name}{}

                using handle = symbolic_computation::handle;

                virtual ~symbolic_transform()=default;

                virtual void begin(){}
                virtual void end(){}
                virtual bool apply(handle&)=0;
                virtual void debug(std::ostream& ostr){}
                virtual std::string get_name(){ return name_; }
        private:
                std::string name_;
        };

        struct symbolic_computation::transform_schedular{

                using transform_t = std::function<bool(handle&)>;

                enum TransformKind{
                        TransformKind_BottomUp,
                        TransformKind_TopDown,
                        TransformKind_OnlyTerminals
                };

                enum {
                        Element_Kind,
                        Element_Transform
                };

                void decl( TransformKind kind, std::shared_ptr<symbolic_transform> transform){
                        decl_.emplace_back( kind, std::move(transform));
                }

                bool execute( handle& root)const{
                        bool result{false};

                        enum class opcode{
                                yeild_or_apply,
                                apply
                        };

                        for( auto const& t  : decl_){
                                auto transform{ std::get<Element_Transform>(t)};
                                boost::timer::auto_cpu_timer at( "    " + transform->get_name() +  " took %w seconds\n");
                                std::vector<std::pair<opcode, handle*>> stack;
                                stack.emplace_back(opcode::yeild_or_apply, &root);
                                transform->begin();
                                for(;stack.size();){
                                        auto p{stack.back()};
                                        stack.pop_back();
                                        
                                        switch(p.first){
                                        case opcode::yeild_or_apply:
                                                if( (*p.second)->is_non_terminal()){
                                                        stack.emplace_back( opcode::apply, p.second );
                                                        auto c{ reinterpret_cast<symbolic_non_terminal*>(p.second->get()) };
                                                        for( auto iter{c->begin()}, end{c->end()}; iter!=end;++iter){
                                                                stack.emplace_back( opcode::yeild_or_apply, &*iter );
                                                        }
                                                        break;
                                                }
                                                // fallthought
                                        case opcode::apply:{
                                                bool ret{ transform->apply( *p.second )};
                                                result = result || ret;
                                        }
                                                break;
                                        }
                                }
                                transform->debug(std::cerr);
                                transform->end();
                                std::cout.flush();
                        }
                        return result;
                }

        private:
                std::vector< std::tuple< TransformKind, std::shared_ptr<symbolic_transform> > > decl_;
        };



        namespace transforms{

                struct permutate_for_the_better : symbolic_transform{
                        permutate_for_the_better():symbolic_transform{"permutate_for_the_better"}{}
                        bool apply(symbolic_computation::handle& ptr)override{
                                if( ptr->get_kind() != symbolic_computation::Kind_Symbolic_Primitive )
                                        return false;
                                
                                auto hands{ reinterpret_cast<symbolic_primitive*>(ptr.get())->get_hands()};
                                std::vector< std::tuple< size_t, std::string> > player_perm;
                                for(size_t i=0;i!=hands.size();++i){
                                        auto h{ holdem_hand_decl::get( hands[i].get() ) };
                                        player_perm.emplace_back(i, h.first().rank().to_string() +
                                                                    h.second().rank().to_string() );
                                }
                                boost::sort(player_perm, [](auto const& left, auto const& right){
                                        return std::get<1>(left) < std::get<1>(right);
                                });
                                std::vector<int> perm;
                                std::array< int, 4> suits{0,1,2,3};
                                std::array< int, 4> rev_suit_map{-1,-1,-1,-1};
                                int suit_iter = 0;

                                std::stringstream from, to;
                                for(size_t i=0;i!=hands.size();++i){
                                        perm.emplace_back( std::get<0>(player_perm[i]) );
                                }
                                for(size_t i=0;i!=hands.size();++i){
                                        auto h{ holdem_hand_decl::get( hands[perm[i]].get() ) };

                                        // TODO pocket pair
                                        if(     rev_suit_map[h.first().suit()] == -1 )
                                                rev_suit_map[h.first().suit()] = suit_iter++;
                                        if(     rev_suit_map[h.second().suit()] == -1 )
                                                rev_suit_map[h.second().suit()] = suit_iter++;
                                }
                                for(size_t i=0;i != 4;++i){
                                        if(     rev_suit_map[i] == -1 )
                                                rev_suit_map[i] = suit_iter++;
                                }
                                std::vector< int> suit_perms;
                                for(size_t i=0;i != 4;++i){
                                        suit_perms.emplace_back(rev_suit_map[i]);
                                }
                                
                                std::vector<frontend::hand> perm_hands;
                                for(size_t i=0;i != hands.size();++i){
                                        auto h{ holdem_hand_decl::get( hands[perm[i]].get() ) };
                                        perm_hands.emplace_back( 
                                                holdem_hand_decl::make_id(
                                                        h.first().rank(),
                                                        suit_perms[h.first().suit()],
                                                        h.second().rank(),
                                                        suit_perms[h.second().suit()]));
                                }
                                ptr = std::make_shared<symbolic_player_perm>( 
                                        perm,
                                        std::make_shared<symbolic_suit_perm>(
                                                suit_perms,
                                                std::make_shared<symbolic_primitive>(
                                                        perm_hands
                                                )
                                        )
                                );
                                return true;
                        }
                };



                struct remove_suit_perms : symbolic_transform{
                        remove_suit_perms():symbolic_transform{"remove_suit_perms"}{}
                        bool apply(symbolic_computation::handle& ptr)override{
                                if( ptr->get_kind() != symbolic_computation::Kind_Symbolic_Suit_Perm )
                                        return false;
                                auto aux_ptr{ reinterpret_cast<symbolic_suit_perm*>(ptr.get()) };
                                assert( aux_ptr->get_children().size() == 1 && "unexpected");
                                auto child = aux_ptr->get_children().front();
                                ptr = child;
                                return true;
                        }
                };
                
                
                struct consolidate_dup_prim : symbolic_transform{
                        consolidate_dup_prim():symbolic_transform{"consolidate_dup_prim"}{}
                        bool apply(symbolic_computation::handle& ptr)override{
                                if( ptr->get_kind() != symbolic_computation::Kind_Symbolic_Primitive )
                                        return false;
                                auto aux_ptr{ reinterpret_cast<symbolic_primitive*>(ptr.get()) };
                                auto hash{ aux_ptr->get_hash() };
                                auto iter{ m_.find(hash) };
                                ++sigma_;
                                if( iter != m_.end() ){
                                        ++hits_;
                                        //PRINT_SEQ((sigma_)(hits_));
                                        ptr = iter->second;
                                        return true;
                                } else {
                                        //PRINT_SEQ((sigma_)(hits_));
                                        m_[hash] = ptr;
                                        return false;
                                }
                        }
                        void debug(std::ostream& ostr)override{
                                ostr << "{ sigma = " << sigma_ << ", hits = " << hits_ << "}\n";
                        }
                private:
                        size_t sigma_ = 0;
                        size_t hits_ = 0;
                        std::map<std::string, symbolic_computation::handle > m_;
                };

                
                struct calc_primitive : symbolic_transform{
                        explicit calc_primitive(calculation_context& ctx):symbolic_transform{"calc_primitive"},ctx_{&ctx}{}
                        bool apply(symbolic_computation::handle& ptr)override{
                                if( ptr->get_kind() != symbolic_computation::Kind_Symbolic_Primitive )
                                        return false;

                                prims_.insert( reinterpret_cast<symbolic_primitive*>(ptr.get()) );
                                return true;
                        }
                        void end()override{
                                detail::work_scheduler sch;
                                for( auto ptr : prims_){
                                        auto j = [this,ptr](){
                                                ptr->calculate( *ctx_ );
                                        };
                                        sch.decl( std::move(j) );
                                }
                                sch.run();
                        }
                private:
                        calculation_context* ctx_;
                        std::set< symbolic_primitive* > prims_;
                };






        } // transforms
} // ps

#endif // PS_TRANSFORMS_H
