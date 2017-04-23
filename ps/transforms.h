#ifndef PS_TRANSFORMS_H
#define PS_TRANSFORMS_H

#include <future>
#include <boost/timer/timer.hpp>

#include "ps/symbolic.h"

namespace ps{

        struct symbolic_transform{

                explicit symbolic_transform(std::string name):name_{name}{}

                using handle = symbolic_computation::handle;

                virtual ~symbolic_transform()=default;

                virtual void begin(){}
                virtual void end(){}
                virtual bool apply(handle&)=0;
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
                                transform->end();
                                std::cout.flush();
                        }
                        return result;
                }

        private:
                std::vector< std::tuple< TransformKind, std::shared_ptr<symbolic_transform> > > decl_;
        };



        namespace transforms{


                struct to_lowest_permutation : symbolic_transform{
                        to_lowest_permutation():symbolic_transform{"to_lowest_permutation"}{}
                        bool apply(symbolic_computation::handle& ptr)override{
                                if( ptr->get_kind() != symbolic_computation::Kind_Symbolic_Primitive )
                                        return false;
                                std::vector<
                                        std::tuple<
                                                std::string, std::vector<int>,
                                                std::vector<int>, std::vector<frontend::hand> 
                                        >
                                > aux;


                                auto hands{ reinterpret_cast<symbolic_primitive*>(ptr.get())->get_hands()};
                                
                                std::vector<int> player_perms;
                                for(int i=0;i!=hands.size();++i)
                                        player_perms.push_back(i);
                                std::vector<int> suit_perms{0,1,2,3};

                                for(;;){
                                        for(;;){
                                                std::string hash;
                                                std::vector<frontend::hand> mapped_hands;

                                                
                                                for( int pidx : player_perms ){
                                                        auto h { holdem_hand_decl::get(hands[pidx].get()) };

                                                        mapped_hands.emplace_back(
                                                                holdem_hand_decl::make_id(
                                                                        h.first().rank(),
                                                                        suit_perms[h.first().suit()],
                                                                        h.second().rank(),
                                                                        suit_perms[h.second().suit()]));

                                                }
                                                aux.emplace_back(symbolic_primitive::make_hash(mapped_hands),
                                                                 player_perms,
                                                                 suit_perms,
                                                                 std::move(mapped_hands));

                                                if( ! boost::next_permutation( suit_perms) )
                                                        break;
                                        }
                                        if( !boost::next_permutation( player_perms))
                                                break;
                                }
                                auto from{ std::get<0>(aux.front())};
                                boost::sort(aux, [](auto const& left, auto const& right){
                                        return std::get<0>(left) < std::get<0>(right);
                                });
                                auto to{std::get<0>(aux.front())};

                                ptr = std::make_shared<symbolic_player_perm>( 
                                        std::get<1>(aux.front()),
                                        std::make_shared<symbolic_suit_perm>(
                                                std::get<2>(aux.front()),
                                                std::make_shared<symbolic_primitive>(
                                                        std::get<3>(aux.front())
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
                                std::vector< std::future<void> > jobs;
                                for( auto ptr : prims_){
                                        auto j = [this,ptr](){
                                                ptr->calculate( *ctx_ );
                                        };
                                        jobs.emplace_back( std::async( std::launch::async, j) );
                                }
                                for( auto& f : jobs)
                                        f.get();
                        }
                private:
                        calculation_context* ctx_;
                        std::set< symbolic_primitive* > prims_;
                };






        } // transforms
} // ps

#endif // PS_TRANSFORMS_H
