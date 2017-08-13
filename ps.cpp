#include <iostream>
#include "ps/eval/equity_future_eval.h"


#include <boost/timer/timer.hpp>

using namespace ps;






int main(){
        boost::timer::auto_cpu_timer at;
        holdem_class_range_vector players;
        //holdem_class_vector players;

        #if 0
        // Hand 0:  30.845%   28.23%  02.61%      3213892992  297127032.00   { TT+, AQs+, AQo+ }
        // Hand 1:  43.076%   40.74%  02.33%      4637673516  265584984.00   { QQ+, AKs, AKo }
        // Hand 2:  26.079%   25.68%  00.40%      2923177728   45324924.00   { TT }
        players.push_back(" TT+, AQs+, AQo+ ");
        players.push_back(" QQ+, AKs, AKo ");
        players.push_back("TT");
        #elif 1
        // Hand 0:  20.371%   17.93%  02.44%     30818548800  4197937635.60   { AKo }
        // Hand 1:  16.316%   15.71%  00.60%     27008469180  1037843085.60   { KQs }
        // Hand 2:  12.791%   12.43%  00.36%     21362591328  624688383.60   { Q6s-Q4s }
        // Hand 3:  12.709%   10.43%  02.28%     17933453280  3911711199.60   { ATo+ }
        // Hand 4:  37.813%   37.72%  00.09%     64837718124  160168887.60   { TT-77 }
        players.push_back(" AKo ");
        players.push_back(" KQs ");
        players.push_back(" Q6s-Q4s ");
        players.push_back(" ATo+ ");
        players.push_back(" TT-77 ");
        #elif 1
        // Hand 0:  59.954%   59.49%  00.46%     19962172212  155993282.00   { KK+, AKs }
        // Hand 1:  15.769%   15.22%  00.55%      5106357048  185284538.00   { 55+, A2s+, K5s+, Q8s+, J8s+, T8s+, 98s, A7o+, K9o+, Q9o+, JTo }
        // Hand 2:  24.277%   24.07%  00.21%      8076681624   69710696.00   { QQ }
        players.push_back("KK+, AKs ");
        players.push_back(" 55+, A2s+, K5s+, Q8s+, J8s+, T8s+, 98s, A7o+, K9o+, Q9o+, JTo ");
        players.push_back("QQ");
        #else
        // Hand 0:  81.547%   81.33%  00.22%        50134104     133902.00   { AA }
        // Hand 1:  18.453%   18.24%  00.22%        11241036     133902.00   { QQ }
        players.push_back("AA");
        players.push_back("QQ");
        #endif

        auto cache = std::make_shared<holdem_class_eval_cache>();
        //cache->load("cache.bin");
        cache->display();

        boost::asio::io_service io;
        //for(int i=0;i!=3;++i){
                equity_future_eval b;
                b.inject_cache(cache);
                auto ret = b.foo(io, players);
                std::vector<std::thread> tg;
                for(size_t i=0;i!=std::thread::hardware_concurrency();++i){
                        tg.emplace_back( [&](){ io.run(); } );
                }
                for( auto& t : tg)
                        t.join();
                std::cout << *ret.get() << "\n";
        //}
        std::cout << "cache\n";

        //cache->display();
        //cache->save("cache.bin");
        

}
