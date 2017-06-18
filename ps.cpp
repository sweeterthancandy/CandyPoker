#include "ps/heads_up.h"

int main(){
        using namespace ps;
        using namespace ps::frontend;
        range p0;
        range p1;

        p0 += _AKo;
        p1 += _55;

        equity_cacher ec;
        ec.load("cache.bin");

        /*
                +----+------+----------+-------+
                |Hand|Equity|   Wins   | Ties  |
                +----+------+----------+-------+
                |Ako |45.38%|55,669,464|562,284|
                | 55 |54.62%|67,054,140|562,284|
                +----+------+----------+-------+
        */

        tree_range root{ std::vector<frontend::range>{p0, p1} };
        root.display();
        hu_visitor agg;
        for( auto const& c : root.children ){
                for( auto d : c.children ){
                        auto ret{ ec.visit_boards(d.players) };
                        std::cout << d << " -> " << ret << "\n";
                        agg.append(ret);
                }
        }

        std::cout << "aggregate -> " << agg << "\n";

}
