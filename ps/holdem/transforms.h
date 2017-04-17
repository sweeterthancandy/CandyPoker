#ifndef PS_TRANSFORMS_H
#define PS_TRANSFORMS_H


namespace ps{
        namespace transforms{

                struct to_lowest_permutation{
                        bool operator()(symbolic_computation::handle& ptr)const{
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

                                for( ;;){
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

                                PRINT_SEQ((from)(to));


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



                struct remove_suit_perms{
                        bool operator()(symbolic_computation::handle& ptr)const{
                                if( ptr->get_kind() != symbolic_computation::Kind_Symbolic_Suit_Perm )
                                        return false;
                                auto aux_ptr{ reinterpret_cast<symbolic_suit_perm*>(ptr.get()) };
                                assert( aux_ptr->get_children().size() == 1 && "unexpected");
                                auto child = aux_ptr->get_children().front();
                                ptr = child;
                                return true;
                        }
                };





        } // transforms
} // ps

#endif // PS_TRANSFORMS_H
