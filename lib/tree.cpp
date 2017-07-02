#include <boost/lexical_cast.hpp>
#include "ps/tree.h"

#include "ps/detail/tree_printer.h"

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
                // XXX dispatch for N
                switch(this->players.size()){
                case 2:
                        detail::visit_exclusive_combinations<2>(
                                [&](auto a, auto b){
                                children.emplace_back(
                                       std::vector<frontend::primitive_t>{prims[0][a], prims[1][b]}
                                );
                        }, detail::true_, size_vec);
                        break;
                case 3:
                        detail::visit_exclusive_combinations<3>(
                                [&](auto a, auto b, auto c){
                                children.emplace_back(
                                       std::vector<frontend::primitive_t>{prims[0][a], prims[1][b], prims[2][c]}
                                );
                        }, detail::true_, size_vec);
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
                        auto cid{ to_class_id( p ) };
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
                default:
                        assert( 0 && " not implemented");
                }
        }
} // ps
