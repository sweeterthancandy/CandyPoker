#ifndef PS_TREE_H
#define PS_TREE_H

#include "ps/frontend.h"


namespace ps{


        struct tree_hand{

                explicit tree_hand(std::vector<frontend::hand> const& players):players{players}{}

                friend std::ostream& operator<<(std::ostream& ostr, tree_hand const& self){
                        for(size_t i{0};i!=self.players.size();++i){
                                if( i != 0 ) ostr << " vs ";
                                ostr << self.players[i];
                        }
                        return ostr;
                }

                std::vector<frontend::hand>      players;
        };

        struct tree_primitive_range{

                explicit tree_primitive_range(std::vector<frontend::primitive_t> const& players);
                
                friend std::ostream& operator<<(std::ostream& ostr, tree_primitive_range const& self){
                        for(size_t i{0};i!=self.players.size();++i){
                                if( i != 0 ) ostr << " vs ";
                                ostr << self.players[i];
                        }
                        return ostr;
                }

                std::vector<frontend::primitive_t> players;
                std::vector<tree_hand>             children;
        };

        struct tree_range{

                explicit tree_range(std::vector<frontend::range> const& players);
                
                friend std::ostream& operator<<(std::ostream& ostr, tree_range const& self){
                        for(size_t i{0};i!=self.players.size();++i){
                                if( i != 0 ) ostr << " vs ";
                                ostr << self.players[i];
                        }
                        return ostr;
                }

                void display()const;

                std::vector<frontend::range> players;
                std::vector<tree_primitive_range> children;
        };
                


} // ps

#endif // PS_TREE_H

