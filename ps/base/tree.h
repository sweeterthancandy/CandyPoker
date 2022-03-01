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
#ifndef PS_TREE_H
#define PS_TREE_H

#include <boost/optional.hpp>
#include "ps/base/frontend.h"

namespace ps{
        /*
                This a helper structure, to map the range vs range into a tree structure
                of 

                                             <range>
                            /                   |                     \
                 <primitive_range>      <primitive_range>    <primitive_range> 
                /       |       \
           <hand>    <hand>    <hand>


                This makes evalu
         */


        struct tree_hand{

            explicit tree_hand(std::vector<holdem_id> const& players_)
                : players{ players_ }
            {}
                friend std::ostream& operator<<(std::ostream& ostr, tree_hand const& self){
                        for(size_t i{0};i!=self.players.size();++i){
                                if( i != 0 ) ostr << " vs ";
                                //ostr << holdem_hand_decl::get(self.players[i]);
                        }
                        return ostr;
                }

                std::vector<holdem_id> players;
        };

        struct tree_primitive_range{

                explicit tree_primitive_range(std::vector<frontend::class_range> const& players);
                
                friend std::ostream& operator<<(std::ostream& ostr, tree_primitive_range const& self){
                        for(size_t i{0};i!=self.players.size();++i){
                                if( i != 0 ) ostr << " vs ";
                                //ostr << self.players[i];
                        }
                        return ostr;
                }


                std::vector<frontend::class_range> players;

                std::vector<tree_hand> const& get_children()const;
        private:
                mutable boost::optional<std::vector<tree_hand> > children_;
        };

        struct tree_range{

                explicit tree_range(std::vector<frontend::range> const& players);
                
                friend std::ostream& operator<<(std::ostream& ostr, tree_range const& self){
                        for(size_t i{0};i!=self.players.size();++i){
                                if( i != 0 ) ostr << " vs ";
                                //ostr << self.players[i];
                        }
                        return ostr;
                }

                void display()const;

                std::vector<frontend::range> players;
                std::vector<tree_primitive_range> children;
        };
                


} // ps


#endif // PS_TREE_H

