#ifndef PS_CARD_TRAITS_H
#define PS_CARD_TRAITS_H

#include <string>
#include <cassert>

struct card_traits{
        long make(std::string const& h)const{
                assert(h.size()==2 && "preconditon failed");
                return make(h[0], h[1]);
        }
        long make(char r, char s)const{
                return map_suit(s) + map_rank(r)*4;
        }
        long suit(long c)const{
                return c % 4;
        }
        long rank(long c)const{
                return c / 4;
        }
        long map_suit(char s)const{
                switch(s){
                case 'h': case 'H': return 0;
                case 'd': case 'D': return 1;
                case 'c': case 'C': return 2;
                case 's': case 'S': return 3;
                default:
                                    assert( 0 && "precondtion failed");
                                    return -1;
                }
        }
        long map_rank(char s)const{
                switch(s){
                case '2': return 0;
                case '3': return 1;
                case '4': return 2;
                case '5': return 3;
                case '6': return 4;
                case '7': return 5;
                case '8': return 6;
                case '9': return 7;
                case 'T': case 't': return 8;
                case 'J': case 'j': return 9;
                case 'Q': case 'q': return 10;
                case 'K': case 'k': return 11;
                case 'A': case 'a': return 12;
                default: return -1;
                }
        }
        std::string suit_to_string(long s)const{
                switch(s){
                case 0: return "H";
                case 1: return "D";
                case 2: return "C";
                case 3: return "S";
                default:
                        assert(0 && "precondition failed");
                        return "_";
                }
        }
        std::string rank_to_string(long r)const{
                switch(r){
                case  0: return "2";
                case  1: return "3";
                case  2: return "4";
                case  3: return "5";
                case  4: return "6";
                case  5: return "7";
                case  6: return "8";
                case  7: return "9";
                case  8: return "T";
                case  9: return "J";
                case 10: return "Q";
                case 11: return "K";
                case 12: return "A";
                default:
                        assert(0 && "precondition failed");
                        return "_";
                }
        }
        std::string to_string(long c){
                return rank_to_string(rank(c)) + suit_to_string(suit(c));
        }
};

#endif // PS_CARD_TRAITS_H
