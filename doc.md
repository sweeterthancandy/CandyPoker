
#Candy Poker Project

This is project with many goals, in particular

* replicate Flopzilla
* replicate Pokerstove
* replicate Holdem Manager
* replicate SnG Wizard
* Create an efficent library to build poker software
* Poker equity calculator
* N-player Push-Fold poker solver
* Game Theory orientated anaysis
* Pokerstars hand history parser
* Provide a framework for ad-hoc poker anaysis

#Framework 



        +-------------------------------------+
        |                                     |
        |              +---------------+      |
        |              |Hand Evaluation|      |
        |              +---------------+      |
        |                                     |
        |              +-----------------+    |
        |              |Equity Evaluation|    |
        |              +-----------------+    |
        |                                     |
        |              +----------------+     |
        |              |Class Evaluation|     |
        |              +----------------+     |
        | Evaluator                           |
        +-------------------------------------+
                          |
                  +----------------+
                  |Push Fold Solver|
                  +----------------+


## Evaulation

        |  Hand vs Hand  |      | Preflop   |
        | Range vs Range |      | Post flop |
        | class vs Class |


Poker Evaulation happens at several abstractions, at the lowest we have evaluating a 5-card poker hand C = {c0,c1,c2,c3,c4}, which means mapping the set to the natural numbers

        m : (c0,c1,c2,c3,c4) -> Z.

For practical purposes, I implement evalauation of a 5-card hand but creating 2 auxially arrays, one array when all the suits of C are the same,
        
        alpha : Z -> Z
        beta  : Z -> Z
        m : C -> Z
        m(C) = { alpha(zeta(C))    where C all same suit 
               { beta(zeta(C))     otherwise

I can then creating a mapping 

        zeta : C -> Z
        zeta(C) =  2 rank(c0) +
                   3 rank(c1) +
                   5 rank(c2) + 
                   7 rank(c3) + 
                  11 rank(c4)

I now have a unique mapping C->Z, such that C maps to 12^4\*11 = 228096 ( log_2(228096) = 17.799282a ).
I can then, by precomuting arrays alpha and beta, have an efficent algorithm for comparing n 5-card hands.

I can then evaluate a 7-card hand, by 

        rho : C -> Z
        rho(C) = min{ m(c) : c \in C'(C) }
        C' = { (a,b,c,d,e) \substr C }

ie, we find the lowest value of m : C -> Z, by taking 5 from 7.

On the next level of evaluation, we have range vs range evalation, which has to take into account the combinations of poker hands, for example there propotations of 6 pocket pairs, 4 suited hands, and 12 offsuit hands.


# Solver


                
#History
* 5-card evaluator       
* hand vs hand preflop evaulator
* Created mechinism to cache every hand vs hand preflop situation

#Future Work

* Increate evaulator to n-players
* Improve ps/cards.h and ps/frontend.h, to consolidate interface, and remove unneccary use of std::vector
* Improve tree to construct on the fly, will be needed for 9x9 simulation 
* Write combinatics functions for 1-n players for replace switch
* Remove unneccary memory allocation in eval
* Add sub-library for solving game trees (I think I can do this)

