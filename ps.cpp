#include "ps/holdem/simulation.h"
#include "ps/core/cards.h"
#include "ps/holdem/frontend.h"

namespace ps{

        /*
        
        The idea here is what when computing range
        vs range evaluation, there are a number of
        results which are linearly related, because
        the way the suits are distribution evently
        when can essentially creat a bijective
        mapping (or permutation)
                f: {h,d,s,c} -> {h',d',s',c'},
        such such that there are dupliucate 
        computations. 
                Consider the computeration

                        AKs vs 98s
                =>
                        AhKh  9h8h 
                        AcKc  9c8c
                        AsKs  9s8s
                        AdKd  9d8d

                =>
                        AhKh   9h8h
                        AhKh   9c8c
                        AhKh   9s8s
                        AhKh   9d8d
                        AcKc   9h8h
                        AcKc   9c8c
                        AcKc   9s8s
                        AcKc   9d8d
                        AsKs   9h8h
                        AsKs   9c8c
                        AsKs   9s8s
                        AsKs   9d8d
                        AdKd   9h8h
                        AdKd   9c8c
                        AdKd   9s8s
                        AdKd   9d8d

        For there 16 computations there exists a
        bijective mapping that the above results in
                
                +--+------------+
                |N |Computation |
                +--+------------+
                |12|AhKh vs 9c8c|
                |4 |AhKh vs 9h8h|
                +--+------------+
                   figure 1

                Another optimization to consider is
        the computation set
               
                  +----------------------+-------------+
                  |     Computation      | Permutation |
                  +----------------------+-------------+
                  | AhKh vs TcTh vs 2s2c |    0 1 2    |
                  | AhKh vs 2s2c vs TcTh |    0 2 1    |
                  | TcTh vs AhKh vs 2s2c |    1 0 2    |
                  | 2s2c vs AhKh vs TcTh |    1 2 0    |
                  | TcTh vs 2s2c vs AhKh |    2 0 1    |
                  | 2s2c vs TcTh vs AhKh |    2 1 0    |
                  +----------------------+-------------+
                            figure 2

                This again can be represented by creating an
        auxillary computation set, and then the result will
        be a linear combintation of this.
                To compute fig 2, we have the below
                
                Let Comp = AhKh vs TcTh vs 2s2c 

                        |S_AhKh|   | w_0 d_0 s_0 e_0 |
                Let S = |S_TcTh| = | w_1 d_1 s_1 e_1 |
                        |S_2s2C|   | w_2 d_2 s_2 e_2 |

                where
                    = Eval(AhKh vs TcTh vs 2s2c)
                      Eval(Comp) 
                    = S

                This implies that
                                                     | 1 0 0 |
                        S_{AhKh vs 2s2c vs TcTh} = S | 0 0 1 |
                                                     | 0 1 0 |

                And thus a computeration set can be reduced
        to a linear combintations of permutation matrixis.
                        
                
                Another optimization that will be applied is
        cached values. For this, consider the computation
                        AA vs TT,
        if I allow this to be represented in the computation
        symbolically, rather than expand to suits, and using
        the fact that
                        log(52^4) + 15.804975 + \epsilon,
        i can precalculate all the heads up probilities
        for heads up calculations

                                                     



        there will be lots of redundant computtaion 


                For this I need to abstract the way computation
        results are stored, to allow the freedom of how the 
        result is calculated

         */

        #if 0
        struct computation_result{
        };


        struct computation_item{
        };

        struct computation{
        };

        struct computation_builder{
                computation make(std::vector<frontend::range> const& players){
                }
        };
        #endif


}

int main(){

        using namespace ps;

        using namespace ps::frontend;

        range rng;
        rng += _KTs++;
        rng += _A5s-_A2s;
        rng += _99 - _TT;
        rng += _JJ - _77;


        PRINT(rng);
        PRINT(expand(rng));

        
}
