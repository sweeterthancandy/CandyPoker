#include <iostream>
#include <string>
#include <type_traits>
#include <chrono>
#include "ps/base/holdem_class_vector.h"
#include "ps/base/holdem_class_range_vector.h"
#include "ps/eval/class_equity_evaluator_cache.h"
#include "ps/eval/class_range_equity_evaluator.h"
#include "ps/eval/class_equity_future.h"

using namespace ps;


#if 0
auto make_proto(std::string const& tok){
        auto a = std::make_unique<processor::process_group>();
        a->push( [tok](){
                std::cout << tok << "::1\n";
        });
        a->push( [tok](){
                std::cout << tok << "::2\n";
        });
        a->sequence_point();
        a->sequence_point();
        a->push( [tok](){
                std::cout << tok << "::3\n";
        });
        return std::move(a);
}

int main(){
        processor proc;
        for(int i=0;i!=100;++i)
                proc.spawn();
        #if 1
        proc.accept(make_proto("a"));
        proc.accept(make_proto("b"));
        proc.accept(make_proto("c"));
        #endif
        proc.join();
}
#endif

int main(){
        support::processor proc;
        for( size_t i=0; i!= std::thread::hardware_concurrency();++i){
                proc.spawn();
        }
        holdem_class_vector players;
        players.push_back("AKs");
        players.push_back("KQs");
        players.push_back("QJs");
        class_equity_future ef;
        auto pg = std::make_unique<support::processor::process_group>();
        auto ret = ef.schedual_group(*pg, players);
        proc.accept(std::move(pg));
        proc.join();
        std::cout << *(ret.get()) << "\n";
        #if 0
        holdem_class_vector players;
        players.push_back("99");
        players.push_back("55");
        class_equity_evaluator_cache ec;

        ec.load("cache.bin");
        std::cout << *ec.evaluate(players) << "\n";
        ec.save("cache.bin");
        #endif
        #if 0
        holdem_class_vector players;
        players.push_back("AA");
        players.push_back("QQ");
        class_equity_evaluator_cache ec;
        std::cout << *ec.evaluate(players) << "\n";
        #endif

}
