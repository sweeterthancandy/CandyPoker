# CandyPoker

This is a C++ poker project authored by Gerry Candy, which aims to be the de-factor C++ poker library. This project was started in 2017 with the original goals
* Be the fastest evaluation library
* Solve three-player push-fold
* Provide an framework for other poker software

## Poker Evaluation

The poker evaulation doesn't support any boards, as this is mostly noise in the code as the preflop all-in with 5 cards to be dealt is the target case.

        ./candy-poker eval TT+,AQs+,AQo+ QQ+,AKs,AKo TT
        |    range    | equity |   wins   | draw_1  | draw_2 | draw equity |   sigma   |
        +-------------+--------+----------+---------+--------+-------------+-----------+
        |TT+,AQs+,AQo+|30.8450%|3213892992|566688312|41308668|297113712.00%|11382741216|
        | QQ+,AKs,AKo |43.0761%|4637673516|503604216|41308668|265571664.00%|11382741216|
        |     TT      |26.0789%|2923177728|63084096 |41308668|45311604.00% |11382741216|
         3.611110s wall, 3.580000s user + 0.010000s system = 3.590000s CPU (99.4%)


## Two Player Push Fold EV


For solving push/fold games, a distinction has to be made between mixed solutions and minimally-mixed solutions. From a game theory perspective there should exist a GTO solution where each decision has only one holdem hand type mixed, meaning we can partition a strategy into {PUSH,FOLD,MIXED}, where MIXED has only one holdem hand type like 68s etc. What this means is that, it we are only looking for any solution, we can run a algorithm of

        Stategry FindAnyGTO(Stategry S){
                double factor = 0.05;
                for(; SomeCondition(S); ){
                        auto counter = CounterStrategy(S);
                        S = S * ( 1- factor ) + counter * factor;
                }
                return S;
        }

The above will converge to a GTO solution, if you where to program a computer to play poker, however we want to find a minimally mixed solution. This is a much hard problem, as there is no simple algorithm which would converge to a minially mixed solution because for any solution, there are going to be a subset of hands which are almost indifferent to being wither PUSH or FOLD, causing computation problems.

Another consideration when looking for solutions, is that although most solvers are compoutaitionally tangible for two player, three player push fold is much much slower, we have to create a suitable algorith. 

The solution to these algorithms, was to chain several different algorithms together, with special metrics. For this I developed these concepts

### Solution.Gamma

The Gamma for a solution is the number of hands, for which the counterstrategy is different. From the game theory we know that their exists a GTO mixed solution, and also we know that there exists a GTO solution with each player only having one mixed card (I think thats right). Gamma is an array of sets corresponding to each player. For example for HU, we might have a gamma of {{Q5s}, {64o,QJo}}, which would indicate that for the SB the hand Q5s is pretty indifferent to PUSH/FOLD, and also for BB 64o and QJo is indifferent to PUSH/FOLD.

### Solution.Level, Solution.Total

The Level of a solution is the maximum number of Gamma cards for each player. So a gamma vector of {{Q5s}, {64o,QJo}} would correspond to (1,2), so the level of the solution is 2, and the total is 3.

### Solution Sequence

Find GTO poker solutions is a stochastic process, as we are finding the find the best solution possible. This means that we are basically producing a sequence of candidate solutions {A0,A1,A2,...}, and from this we create a best to date sequence {B0,B1,B2,...}. 

We have three solvers
* simple-numeric
* single-permutation
* permutation

#### simple-numeric

This is the most basic solver, and is the basic FindAnyGTO() function that has been discussed. As an implementation detail we create an infinite loop, keeping taking the linear product of the two solutions, with a time to live variable of say 10, then if we have 10 iterations with a new best-to-date solution, we return that solution. 


### single-permutation

This is an algebraic solver, each takes the trail solution S, and creates a set with each gamma cards (a particilar card for with S is different from CounterStrategy(S)), with the trail solutions card replaced with either PUSH or FOLD, with all other cards the same. This means that with a gamma vector of {{Q5s}, {64o,QJo}} we would have 2^3 candidate solutions { S with Q5s for player 1 FOLD,  S with Q5s for player 1 PUSH, S with 64o for player 2 PUSH, ...}. We then see if any of these new strategies is a "better" solution, if this is the case we restart the algorith. This algorithm would find the specific solution, depending on the path taken by the best-to-date sequence. 

### permutation

This is another algebraic solver, which tries to replace take the Gamma vector, and replace all but 1 card per decision with a mixed solution, and discretized the mixed solution with a grid. 

For eaxmple with a Gamma vector of {{Q5s}, {64o,QJo}}, we would have gamma vectors of 
                (m)(ff)
                (m)(pf)
                (m)(fp)
                (m)(pp)
                (f)(mf)
                (p)(mf)
                (f)(mp)
                (p)(mp)
                (f)(fm)
                (p)(fm)
                (f)(pm)
                (p)(pm),
where in the above, we replace each m with one of (0,1/n,2/m,..,(n-1)/n,n). We then take the best solution




For a quick example, we can run the command

        ./driver scratch --game-tree two-player-push-fold --solver pipeline --eff-lower 2 --eff-upper 20 --eff-inc 1 --cum-table

                    sb push/fold

            A    K    Q    J    T    9    8    7    6    5    4    3    2
          +-----------------------------------------------------------------
        A |20.0 20.0 20.0 20.0 20.0 20.0 20.0 20.0 20.0 20.0 20.0 20.0 20.0
        K |20.0 20.0 20.0 20.0 20.0 20.0 20.0 20.0 20.0 20.0 20.0 19.8 19.3
        Q |20.0 20.0 20.0 20.0 20.0 20.0 20.0 20.0 20.0 19.9 16.3 13.5 12.7
        J |20.0 20.0 20.0 20.0 20.0 20.0 20.0 20.0 18.6 15.3 13.5 10.6 8.5
        T |20.0 20.0 20.0 20.0 20.0 20.0 20.0 20.0 20.0 11.9 10.6 7.5  6.5
        9 |20.0 20.0 20.0 20.0 20.0 20.0 20.0 20.0 20.0 14.4 6.5  4.9  3.7
        8 |20.0 18.0 13.0 12.9 17.4 20.0 20.0 20.0 20.0 18.8 10.0 2.7  2.5
        7 |20.0 16.1 10.3 8.5  9.0  10.8 15.2 20.0 20.0 20.0 13.9 2.5  2.1
        6 |20.0 15.0 9.6  6.5  5.7  5.2  7.0  10.7 20.0 20.0 16.3 7.1  2.0
        5 |20.0 14.2 8.9  6.0  4.1  3.4  3.0  2.6  2.4  20.0 20.0 13.3 2.0
        4 |20.0 13.1 7.9  5.4  3.8  2.7  2.3  2.1  2.0  2.1  20.0 9.9   0
        3 |20.0 12.1 7.5  5.0  3.4  2.5   0    0    0    0    0   20.0  0
        2 |20.0 11.6 7.0  4.6  2.9  2.2   0    0    0    0    0    0   20.0

                    bb call/fold, given bb push

            A    K    Q    J    T    9    8    7    6    5    4    3    2
          +-----------------------------------------------------------------
        A |20.0 20.0 20.0 20.0 20.0 20.0 20.0 20.0 20.0 20.0 20.0 20.0 20.0
        K |20.0 20.0 20.0 20.0 20.0 20.0 17.6 15.3 14.3 13.2 12.0 11.4 10.7
        Q |20.0 20.0 20.0 20.0 20.0 16.0 13.0 10.5 9.9  8.9  8.4  7.8  7.2
        J |20.0 20.0 19.1 20.0 18.1 13.3 10.6 8.8  7.0  6.9  6.1  5.8  5.5
        T |20.0 20.0 14.8 12.6 20.0 11.5 9.2  7.4  6.3  5.2  5.1  4.8  4.5
        9 |20.0 16.9 11.6 9.5  8.3  20.0 8.3  6.9  5.8  5.0  4.3  4.1  3.9
        8 |20.0 13.7 9.7  7.6  6.6  6.0  20.0 6.5  5.6  4.8  4.1  3.6  3.5
        7 |20.0 12.2 7.9  6.3  5.4  4.9  4.7  20.0 5.4  4.8  4.1  3.6  3.3
        6 |20.0 10.9 7.3  5.3  4.6  4.2  4.1  4.0  20.0 4.9  4.3  3.8  3.3
        5 |20.0 10.2 6.8  5.1  4.0  3.7  3.6  3.6  3.7  20.0 4.6  4.0  3.6
        4 |18.2 9.1  6.2  4.7  3.8  3.3  3.2  3.2  3.3  3.5  20.0 3.8  3.4
        3 |16.4 8.6  5.8  4.4  3.6  3.1  2.9  2.9  2.9  3.1  3.0  20.0 3.3
        2 |15.6 8.1  5.6  4.2  3.5  3.0  2.8  2.6  2.7  2.8  2.7  2.6  15.0


What is actually much more of a problem is finding a solution with the least number of mixed solutions. Taking BB of 10 for the example, it's not too difficul to implement a solver which finds a game-theoritic solution to the push fold solution
        
        ./driver scratch --no-memory --game-tree two-player-push-fold --solver  --eff-lower 2 --eff-upper 20 --eff-inc 1 --cum-table

### Pre-flop all in-EV


## FlopZilla
        ./candy-poker flopzilla ATs+,AJo+,88+,QJs,KQo
        +-------------------+-------+----------------------+----------------------+
        |       Rank        | Count |         Prob         |         Cum          |
        +-------------------+-------+----------------------+----------------------+
        |    Royal Flush    |  20   |0.00092764378478664194|0.00092764378478664194|
        |  Straight Flush   |   8   |0.00037105751391465676|0.0012987012987012987 |
        |       Quads       | 2152  | 0.099814471243042671 | 0.10111317254174397  |
        |    Full House     | 9288  |  0.4307977736549165  | 0.53191094619666046  |
        |       Flush       | 3272  | 0.15176252319109462  | 0.68367346938775508  |
        |     Straight      | 5604  | 0.25992578849721709  | 0.94359925788497212  |
        |       Trips       |109648 |  5.0857142857142854  |  6.0293135435992582  |
        |     Two pair      |186912 |   8.66938775510204   |  14.698701298701298  |
        |     One pair      |1129920|  52.408163265306122  |  67.106864564007424  |
        |     High Card     |709176 |  32.893135435992576  |         100          |
        +-------------------+-------+----------------------+----------------------+
        |    Flush Draw     | Count |         Prob         |         Cum          |
        +-------------------+-------+----------------------+----------------------+
        |       Flush       | 3300  | 0.15306122448979592  | 0.15306122448979592  |
        |    Four-Flush     | 82500 |  3.8265306122448979  |  3.9795918367346941  |
        |    Three-Flush    |683100 |  31.683673469387756  |  35.663265306122447  |
        |       None        |1387100|  64.336734693877546  |         100          |
        +-------------------+-------+----------------------+----------------------+
        |   Straight Draw   | Count |         Prob         |         Cum          |
        +-------------------+-------+----------------------+----------------------+
        |     Straight      | 5632  | 0.26122448979591839  | 0.26122448979591839  |
        |   Four-Straight   | 66112 |  3.0664192949907236  |  3.327643784786642   |
        |   Double-Gutter   | 2560  | 0.11873840445269017  |  3.446382189239332   |
        |       None        |2081696|  96.55361781076067   |         100          |
        +-------------------+-------+----------------------+----------------------+
        |   Detailed Rank   | Count |         Prob         |         Cum          |
        +-------------------+-------+----------------------+----------------------+
        |    Royal Flush    |  20   |0.00092764378478664194|0.00092764378478664194|
        |  Straight Flush   |   8   |0.00037105751391465676|0.0012987012987012987 |
        |       Quads       | 2152  | 0.099814471243042671 | 0.10111317254174397  |
        |    Full House     | 9288  |  0.4307977736549165  | 0.53191094619666046  |
        |       Flush       | 3272  | 0.15176252319109462  | 0.68367346938775508  |
        |     Straight      | 5604  | 0.25992578849721709  | 0.94359925788497212  |
        |       Trips       |109648 |  5.0857142857142854  |  6.0293135435992582  |
        |     Two pair      |186912 |   8.66938775510204   |  14.698701298701298  |
        |     One pair      |1129920|  52.408163265306122  |  67.106864564007424  |
        |High Card - 2 Overs|501780 |  23.273654916512058  |  90.380519480519482  |
        |High Card - 1 Over |153696 |  7.1287569573283855  |  97.509276437847873  |
        |High Card - Unders | 53700 |  2.4907235621521338  |         100          |
        +-------------------+-------+----------------------+----------------------+


        ./candy-poker flopzilla 100%
        +-------------------+--------+----------------------+----------------------+
        |       Rank        | Count  |         Prob         |         Cum          |
        +-------------------+--------+----------------------+----------------------+
        |    Royal Flush    |   40   |0.00015390771693292701|0.00015390771693292701|
        |  Straight Flush   |  360   |0.0013851694523963432 |0.0015390771693292702 |
        |       Quads       |  6240  | 0.024009603841536616 | 0.025548681010865885 |
        |    Full House     | 37440  | 0.14405762304921968  | 0.16960630406008556  |
        |       Flush       | 51080  |  0.1965401545233478  | 0.36614645858343337  |
        |     Straight      | 102000 | 0.39246467817896391  | 0.75861113676239722  |
        |       Trips       | 549120 |  2.112845138055222   |  2.8714562748176196  |
        |     Two pair      |1235520 |  4.7539015606242501  |  7.6253578354418687  |
        |     One pair      |10982400|  42.256902761104442  |  49.88226059654631   |
        |     High Card     |13025400|  50.11773940345369   |         100          |
        +-------------------+--------+----------------------+----------------------+
        |    Flush Draw     | Count  |         Prob         |         Cum          |
        +-------------------+--------+----------------------+----------------------+
        |       Flush       | 51480  | 0.19807923169267708  | 0.19807923169267708  |
        |    Four-Flush     |1115400 |  4.2917166866746701  |  4.4897959183673466  |
        |    Three-Flush    |8477040 |  32.617046818727488  |  37.106842737094837  |
        |       None        |16345680|  62.893157262905163  |         100          |
        +-------------------+--------+----------------------+----------------------+
        |   Straight Draw   | Count  |         Prob         |         Cum          |
        +-------------------+--------+----------------------+----------------------+
        |     Straight      | 92160  | 0.35460337981346385  | 0.35460337981346385  |
        |   Four-Straight   | 890880 |  3.4278326715301506  |  3.7824360513436144  |
        |   Double-Gutter   | 71680  | 0.27580262874380523  |  4.0582386800874195  |
        |       None        |24934880|  95.941761319912587  |         100          |
        +-------------------+--------+----------------------+----------------------+
        |   Detailed Rank   | Count  |         Prob         |         Cum          |
        +-------------------+--------+----------------------+----------------------+
        |    Royal Flush    |   40   |0.00015390771693292701|0.00015390771693292701|
        |  Straight Flush   |  360   |0.0013851694523963432 |0.0015390771693292702 |
        |       Quads       |  6240  | 0.024009603841536616 | 0.025548681010865885 |
        |    Full House     | 37440  | 0.14405762304921968  | 0.16960630406008556  |
        |       Flush       | 51080  |  0.1965401545233478  | 0.36614645858343337  |
        |     Straight      | 102000 | 0.39246467817896391  | 0.75861113676239722  |
        |       Trips       | 549120 |  2.112845138055222   |  2.8714562748176196  |
        |     Two pair      |1235520 |  4.7539015606242501  |  7.6253578354418687  |
        |     One pair      |10982400|  42.256902761104442  |  49.88226059654631   |
        |High Card - 2 Overs|1302540 |  5.0117739403453685  |  54.894034536891681  |
        |High Card - 1 Over |3907620 |  15.035321821036106  |  69.929356357927787  |
        |High Card - Unders |7815240 |  30.070643642072213  |         100          |
        +-------------------+--------+----------------------+----------------------+
