#include "ps/ps.h"
#include "ps/detail/print.h"
#include "ps/equity_calc.h"

using namespace ps;
namespace{

void eval_test(){
        driver d;

        PRINT( d.eval_5("AhKhQhJhTh") );
        PRINT( d.eval_5("AsKsQsJsTs") );
        PRINT( d.eval_5("AcKcQcJcTc") );
        PRINT( d.eval_5("AhAcAcAd2c") );
        PRINT( d.eval_5("AhAcAcAd2d") );

}

void ec_test(){
        equity_calc ec;
        card_traits t;
        std::vector<equity_player> players;
        std::vector<long> board;




        /*
        
        +----+------+-------+------+
        |Hand|Equity| Wins  | Ties |
        +----+------+-------+------+
        |ahkh|50.08%|852,207|10,775|
        |2s2c|49.92%|849,322|10,775|
        +----+------+-------+------+

        */
        players.emplace_back( t.make("Ah"), t.make("Kh") );
        players.emplace_back( t.make("2d"), t.make("2c") );

        ec.calc( players );
        for( auto const& p : players){
                std::cout << p << "\n";
        } 

        /*
        +----+------+-------+-----+
        |Hand|Equity| Wins  |Ties |
        +----+------+-------+-----+
        |ahkh|42.12%|574,928|7,155|
        |2s2c|25.22%|343,287|7,155|
        |5c6c|32.67%|445,384|7,155|
        +----+------+-------+-----+
        */
        players.clear();
        std::cout << "---------------------------------------\n";
        players.emplace_back( t.make("Ah"), t.make("Kh") );
        players.emplace_back( t.make("2d"), t.make("2c") );
        players.emplace_back( t.make("5c"), t.make("6c") );
        ec.calc( players );
        for( auto const& p : players){
                std::cout << p << "\n";
        } 
        
        /*

        */
        players.clear();
        std::cout << "---------------------------------------\n";
        players.emplace_back( t.make("Ah"), t.make("Kh") );
        players.emplace_back( t.make("2s"), t.make("2c") );
        players.emplace_back( t.make("5c"), t.make("6c") );
        board = std::vector<long>{t.make( "8d"), t.make("9d"), t.make("js") };
        ec.calc( players, board);
        for( auto const& p : players){
                std::cout << p << "\n";
        } 

}



void generate_test(){
        printer p;
        generate<card_traits>(p);

}
}

int main(){
        //generate_test();
        //traits_test();
        //eval_test();
        ec_test();
}
