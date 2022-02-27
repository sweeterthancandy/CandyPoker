/*

CandyPoker
https://github.com/sweeterthancandy/CandyPoker

MIT License

Copyright (c) 2019 Gerry Candy

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.

*/

#include <boost/lexical_cast.hpp>

#include "ps/base/tree.h"
#include "ps/base/algorithm.h"

#include "ps/detail/tree_printer.h"
#include "ps/detail/visit_sequence.h"
#include "ps/detail/visit_combinations.h"

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
                std::vector<std::vector<frontend::class_range> > prims;
                for(auto const& rng : players){
                        prims.emplace_back( rng.expand().to_class_range_vector());
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
                                        std::vector<frontend::class_range>{comb...}
                                        );
                                }, prims[0], prims[1]);
                        break;
                case 3:
                        detail::visit_vector_combinations(
                                [&](auto... comb){
                                children.emplace_back(
                                        std::vector<frontend::class_range>{comb...}
                                        );
                                }, prims[0], prims[1], prims[2]);
                        break;
                case 4:
                        detail::visit_vector_combinations(
                                [&](auto... comb){
                                children.emplace_back(
                                        std::vector<frontend::class_range>{comb...}
                                        );
                                }, prims[0], prims[1], prims[2], prims[3]);
                        break;
                case 5:
                        detail::visit_vector_combinations(
                                [&](auto... comb){
                                children.emplace_back(
                                        std::vector<frontend::class_range>{comb...}
                                        );
                                }, prims[0], prims[1], prims[2], prims[3], prims[4] );
                        break;
                case 6:
                        detail::visit_vector_combinations(
                                [&](auto... comb){
                                children.emplace_back(
                                        std::vector<frontend::class_range>{comb...}
                                        );
                                }, prims[0], prims[1], prims[2], prims[3], prims[4], prims[5]);
                        break;
                case 7:
                        detail::visit_vector_combinations(
                                [&](auto... comb){
                                children.emplace_back(
                                        std::vector<frontend::class_range>{comb...}
                                        );
                                }, prims[0], prims[1], prims[2], prims[3], prims[4], prims[5], prims[6]);
                        break;
                case 8:
                        detail::visit_vector_combinations(
                                [&](auto... comb){
                                children.emplace_back(
                                        std::vector<frontend::class_range>{comb...}
                                        );
                                }, prims[0], prims[1], prims[2], prims[3], prims[4], prims[5], prims[6], prims[7]);
                        break;
                case 9:
                        detail::visit_vector_combinations(
                                [&](auto... comb){
                                children.emplace_back(
                                        std::vector<frontend::class_range>{comb...}
                                        );
                                }, prims[0], prims[1], prims[2], prims[3], prims[4], prims[5], prims[6], prims[7], prims[8]);
                        break;
                default:
                        assert( 0 && " not implemented");
                }
        }

        tree_primitive_range::tree_primitive_range(std::vector<frontend::class_range> const& players){
                std::vector<size_t> size_vec;
                std::vector<std::vector<holdem_id> > aux;
                this->players = players;

                for( auto const& p : this->players){
                        aux.emplace_back( p.to_holdem_vector());
                        size_vec.push_back( aux.back().size()-1);
                        opt_cplayers.push_back(p.get_class_id());

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
                                                       std::vector<holdem_id>{
                                                                holdem_id{aux[0][a]},
                                                                holdem_id{aux[1][b]}});
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
                                                       std::vector<holdem_id>{
                                                                holdem_id{aux[0][a]},
                                                                holdem_id{aux[1][b]},
                                                                holdem_id{aux[2][c]}});
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
                                                       std::vector<holdem_id>{
                                                                holdem_id{aux[0][a]},
                                                                holdem_id{aux[1][b]},
                                                                holdem_id{aux[2][c]},
                                                                holdem_id{aux[3][d]}
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
                                                       std::vector<holdem_id>{
                                                                holdem_id{aux[0][a]},
                                                                holdem_id{aux[1][b]},
                                                                holdem_id{aux[2][c]},
                                                                holdem_id{aux[3][d]},
                                                                holdem_id{aux[4][e]}
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
                                                       std::vector<holdem_id>{
                                                                holdem_id{aux[0][a]},
                                                                holdem_id{aux[1][b]},
                                                                holdem_id{aux[2][c]},
                                                                holdem_id{aux[3][d]},
                                                                holdem_id{aux[4][e]},
                                                                holdem_id{aux[5][f]}
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
                                                       std::vector<holdem_id>{
                                                                holdem_id{aux[0][a]},
                                                                holdem_id{aux[1][b]},
                                                                holdem_id{aux[2][c]},
                                                                holdem_id{aux[3][d]},
                                                                holdem_id{aux[4][e]},
                                                                holdem_id{aux[5][f]},
                                                                holdem_id{aux[6][g]}
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
                                                       std::vector<holdem_id>{
                                                                holdem_id{aux[0][a]},
                                                                holdem_id{aux[1][b]},
                                                                holdem_id{aux[2][c]},
                                                                holdem_id{aux[3][d]},
                                                                holdem_id{aux[4][e]},
                                                                holdem_id{aux[5][f]},
                                                                holdem_id{aux[6][g]},
                                                                holdem_id{aux[7][h]}
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
                                                       std::vector<holdem_id>{
                                                                holdem_id{aux[0][a]},
                                                                holdem_id{aux[1][b]},
                                                                holdem_id{aux[2][c]},
                                                                holdem_id{aux[3][d]},
                                                                holdem_id{aux[4][e]},
                                                                holdem_id{aux[5][f]},
                                                                holdem_id{aux[6][g]},
                                                                holdem_id{aux[7][h]},
                                                                holdem_id{aux[8][i]}
                                                                });
                                }
                        }, detail::true_, size_vec);
                        break;
                default:
                        assert( 0 && " not implemented");
                }
        }
} // ps

