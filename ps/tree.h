#ifndef PS_TREE_H
#define PS_TREE_H

#include "ps/frontend.h"


namespace ps{


        struct tree_hand{

                explicit tree_hand(std::vector<frontend::hand> const& players):players{players}{}

                std::vector<frontend::hand>      players;
        };

        struct tree_primitive_range{

                explicit tree_primitive_range(std::vector<frontend::primitive_t> const& players);

                std::vector<frontend::primitive_t> players;
                std::vector<tree_hand>             children;
        };

        struct tree_range{

                explicit tree_range(std::vector<frontend::range> const& players);

                void display()const;

                std::vector<frontend::range> players;
                std::vector<tree_primitive_range> children;
        };
                


} // ps

#endif // PS_TREE_H

