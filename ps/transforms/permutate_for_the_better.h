#ifndef PS_TRANSFORM_PERMUTATE_FOR_THE_BETTER_H
#define PS_TRANSFORM_PERMUTATE_FOR_THE_BETTER_H

namespace ps{
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
} // transform
} // ps
#endif // PS_TRANSFORM_PERMUTATE_FOR_THE_BETTER_H
