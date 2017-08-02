#include <boost/lexical_cast.hpp>

#include "ps/base/tree.h"
#include "ps/base/algorithm.h"

#include "ps/detail/tree_printer.h"
#include "ps/detail/visit_sequence.h"

namespace ps{
        void tree_range::display()const{
                detail::tree_printer_detail p;
                do{
                        p.non_terminal(
                                boost::lexical_cast<std::string>(
                                        *this
                        ));
                }while(0);

                p.begin_children_size( children.size() );
                for( auto const& c : children ){

                        p.non_terminal(
                                boost::lexical_cast<std::string>(
                                        c
                        ));

                        p.begin_children_size( c.children.size() );
                        for( auto const& d : c.children ){
                                p.terminal(
                                        boost::lexical_cast<std::string>(
                                                d
                                ));

                                p.next_child();
                        }
                        p.end_children();

                        p.next_child();
                }
                p.end_children();
        }



        tree_range::tree_range(std::vector<frontend::range> const& players){
                std::vector<size_t> size_vec;
                std::vector<frontend::primitive_range> prims;
                for(auto const& rng : players){
                        prims.emplace_back( expand(rng).to_primitive_range());
                        assert( prims.back().size() != 0 && "precondition failed");
                        size_vec.emplace_back(prims.back().size()-1);
                }


                this->players = players;
                // XXX dispatch fr N
                switch(this->players.size()){
                case 2:
                        detail::visit_vector_combinations(
                                [&](auto... comb){
                                children.emplace_back(
                                        std::vector<frontend::primitive_t>{comb...}
                                        );
                                }, prims[0], prims[1]);
                        break;
                case 3:
                        detail::visit_vector_combinations(
                                [&](auto... comb){
                                children.emplace_back(
                                        std::vector<frontend::primitive_t>{comb...}
                                        );
                                }, prims[0], prims[1], prims[2]);
                        break;
                case 4:
                        detail::visit_vector_combinations(
                                [&](auto... comb){
                                children.emplace_back(
                                        std::vector<frontend::primitive_t>{comb...}
                                        );
                                }, prims[0], prims[1], prims[2], prims[3]);
                        break;
                case 5:
                        detail::visit_vector_combinations(
                                [&](auto... comb){
                                children.emplace_back(
                                        std::vector<frontend::primitive_t>{comb...}
                                        );
                                }, prims[0], prims[1], prims[2], prims[3], prims[4] );
                        break;
                case 6:
                        detail::visit_vector_combinations(
                                [&](auto... comb){
                                children.emplace_back(
                                        std::vector<frontend::primitive_t>{comb...}
                                        );
                                }, prims[0], prims[1], prims[2], prims[3], prims[4], prims[5]);
                        break;
                case 7:
                        detail::visit_vector_combinations(
                                [&](auto... comb){
                                children.emplace_back(
                                        std::vector<frontend::primitive_t>{comb...}
                                        );
                                }, prims[0], prims[1], prims[2], prims[3], prims[4], prims[5], prims[6]);
                        break;
                case 8:
                        detail::visit_vector_combinations(
                                [&](auto... comb){
                                children.emplace_back(
                                        std::vector<frontend::primitive_t>{comb...}
                                        );
                                }, prims[0], prims[1], prims[2], prims[3], prims[4], prims[5], prims[6], prims[7]);
                        break;
                case 9:
                        detail::visit_vector_combinations(
                                [&](auto... comb){
                                children.emplace_back(
                                        std::vector<frontend::primitive_t>{comb...}
                                        );
                                }, prims[0], prims[1], prims[2], prims[3], prims[4], prims[5], prims[6], prims[7], prims[8]);
                        break;
                default:
                        assert( 0 && " not implemented");
                }
        }

        tree_primitive_range::tree_primitive_range(std::vector<frontend::primitive_t> const& players){
                std::vector<size_t> size_vec;
                std::vector<std::vector<holdem_id> > aux;
                this->players = players;

                for( auto const& p : this->players){
                        aux.emplace_back( to_hand_vector(p));
                        size_vec.push_back( aux.back().size()-1);
                        auto cid =  to_class_id( p ) ;
                        if( cid != -1 )
                                opt_cplayers.push_back(cid);

                }
                // all or none
                if( opt_cplayers.size() != this->players.size())
                        opt_cplayers.clear();

                
                switch(this->players.size()){
                case 2:
                        detail::visit_exclusive_combinations<2>(
                                [&](auto a, auto b){
                                
                                // make sure disjoint

                                if( disjoint( holdem_hand_decl::get(aux[0][a]),
                                              holdem_hand_decl::get(aux[1][b]) ) )
                                {
                                        children.emplace_back(
                                                       std::vector<frontend::hand>{
                                                                frontend::hand{aux[0][a]},
                                                                frontend::hand{aux[1][b]}});
                                }
                        }, detail::true_, size_vec);
                        break;
                case 3:
                        detail::visit_exclusive_combinations<3>(
                                [&](auto a, auto b, auto c){
                                
                                // make sure disjoint

                                if( disjoint( holdem_hand_decl::get(aux[0][a]),
                                              holdem_hand_decl::get(aux[1][b]),
                                              holdem_hand_decl::get(aux[2][c]) ) )
                                {
                                        children.emplace_back(
                                                       std::vector<frontend::hand>{
                                                                frontend::hand{aux[0][a]},
                                                                frontend::hand{aux[1][b]},
                                                                frontend::hand{aux[2][c]}});
                                }
                        }, detail::true_, size_vec);
                        break;
                case 4:
                        detail::visit_exclusive_combinations<4>(
                                [&](auto a, auto b, auto c, auto d){
                                
                                // make sure disjoint

                                if( disjoint( holdem_hand_decl::get(aux[0][a]),
                                              holdem_hand_decl::get(aux[1][b]),
                                              holdem_hand_decl::get(aux[2][c]),
                                              holdem_hand_decl::get(aux[3][d])
                                            ) )
                                {
                                        children.emplace_back(
                                                       std::vector<frontend::hand>{
                                                                frontend::hand{aux[0][a]},
                                                                frontend::hand{aux[1][b]},
                                                                frontend::hand{aux[2][c]},
                                                                frontend::hand{aux[3][d]}
                                                                });
                                }
                        }, detail::true_, size_vec);
                        break;
                case 5:
                        detail::visit_exclusive_combinations<5>(
                                [&](auto a, auto b, auto c, auto d, auto e){
                                
                                // make sure disjoint

                                if( disjoint( holdem_hand_decl::get(aux[0][a]),
                                              holdem_hand_decl::get(aux[1][b]),
                                              holdem_hand_decl::get(aux[2][c]),
                                              holdem_hand_decl::get(aux[3][d]),
                                              holdem_hand_decl::get(aux[4][e])
                                            ) )
                                {
                                        children.emplace_back(
                                                       std::vector<frontend::hand>{
                                                                frontend::hand{aux[0][a]},
                                                                frontend::hand{aux[1][b]},
                                                                frontend::hand{aux[2][c]},
                                                                frontend::hand{aux[3][d]},
                                                                frontend::hand{aux[4][e]}
                                                                });
                                }
                        }, detail::true_, size_vec);
                        break;
                case 6:
                        detail::visit_exclusive_combinations<6>(
                                [&](auto a, auto b, auto c, auto d, auto e, auto f){
                                
                                // make sure disjoint

                                if( disjoint( holdem_hand_decl::get(aux[0][a]),
                                              holdem_hand_decl::get(aux[1][b]),
                                              holdem_hand_decl::get(aux[2][c]),
                                              holdem_hand_decl::get(aux[3][d]),
                                              holdem_hand_decl::get(aux[4][e]),
                                              holdem_hand_decl::get(aux[5][f])
                                            ) )
                                {
                                        children.emplace_back(
                                                       std::vector<frontend::hand>{
                                                                frontend::hand{aux[0][a]},
                                                                frontend::hand{aux[1][b]},
                                                                frontend::hand{aux[2][c]},
                                                                frontend::hand{aux[3][d]},
                                                                frontend::hand{aux[4][e]},
                                                                frontend::hand{aux[5][f]}
                                                                });
                                }
                        }, detail::true_, size_vec);
                        break;
                case 7:
                        detail::visit_exclusive_combinations<7>(
                                [&](auto a, auto b, auto c, auto d, auto e, auto f, auto g){
                                
                                // make sure disjoint

                                if( disjoint( holdem_hand_decl::get(aux[0][a]),
                                              holdem_hand_decl::get(aux[1][b]),
                                              holdem_hand_decl::get(aux[2][c]),
                                              holdem_hand_decl::get(aux[3][d]),
                                              holdem_hand_decl::get(aux[4][e]),
                                              holdem_hand_decl::get(aux[5][f]),
                                              holdem_hand_decl::get(aux[6][g])
                                            ) )
                                {
                                        children.emplace_back(
                                                       std::vector<frontend::hand>{
                                                                frontend::hand{aux[0][a]},
                                                                frontend::hand{aux[1][b]},
                                                                frontend::hand{aux[2][c]},
                                                                frontend::hand{aux[3][d]},
                                                                frontend::hand{aux[4][e]},
                                                                frontend::hand{aux[5][f]},
                                                                frontend::hand{aux[6][g]}
                                                                });
                                }
                        }, detail::true_, size_vec);
                        break;
                case 8:
                        detail::visit_exclusive_combinations<8>(
                                [&](auto a, auto b, auto c, auto d, auto e, auto f, auto g, auto h){
                                
                                // make sure disjoint

                                if( disjoint( holdem_hand_decl::get(aux[0][a]),
                                              holdem_hand_decl::get(aux[1][b]),
                                              holdem_hand_decl::get(aux[2][c]),
                                              holdem_hand_decl::get(aux[3][d]),
                                              holdem_hand_decl::get(aux[4][e]),
                                              holdem_hand_decl::get(aux[5][f]),
                                              holdem_hand_decl::get(aux[6][g]),
                                              holdem_hand_decl::get(aux[7][h])
                                            ) )
                                {
                                        children.emplace_back(
                                                       std::vector<frontend::hand>{
                                                                frontend::hand{aux[0][a]},
                                                                frontend::hand{aux[1][b]},
                                                                frontend::hand{aux[2][c]},
                                                                frontend::hand{aux[3][d]},
                                                                frontend::hand{aux[4][e]},
                                                                frontend::hand{aux[5][f]},
                                                                frontend::hand{aux[6][g]},
                                                                frontend::hand{aux[7][h]}
                                                                });
                                }
                        }, detail::true_, size_vec);
                        break;
                case 9:
                        detail::visit_exclusive_combinations<9>(
                                [&](auto a, auto b, auto c, auto d, auto e, auto f, auto g, auto h, auto i){
                                
                                // make sure disjoint

                                if( disjoint( holdem_hand_decl::get(aux[0][a]),
                                              holdem_hand_decl::get(aux[1][b]),
                                              holdem_hand_decl::get(aux[2][c]),
                                              holdem_hand_decl::get(aux[3][d]),
                                              holdem_hand_decl::get(aux[4][e]),
                                              holdem_hand_decl::get(aux[5][f]),
                                              holdem_hand_decl::get(aux[6][g]),
                                              holdem_hand_decl::get(aux[7][h]),
                                              holdem_hand_decl::get(aux[8][i])
                                            ) )
                                {
                                        children.emplace_back(
                                                       std::vector<frontend::hand>{
                                                                frontend::hand{aux[0][a]},
                                                                frontend::hand{aux[1][b]},
                                                                frontend::hand{aux[2][c]},
                                                                frontend::hand{aux[3][d]},
                                                                frontend::hand{aux[4][e]},
                                                                frontend::hand{aux[5][f]},
                                                                frontend::hand{aux[6][g]},
                                                                frontend::hand{aux[7][h]},
                                                                frontend::hand{aux[8][i]}
                                                                });
                                }
                        }, detail::true_, size_vec);
                        break;
                default:
                        assert( 0 && " not implemented");
                }
        }
} // ps
