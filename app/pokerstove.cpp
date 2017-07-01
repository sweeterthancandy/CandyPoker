#include "ps/tree.h"
#include "ps/frontend.h"

#include <type_traits>

#include <boost/timer/timer.hpp>
#include <boost/lexical_cast.hpp>

using namespace ps;

namespace{
        int pokerstove_driver(int argc, char** argv){
                using namespace ps::frontend;
                equity_cacher ec;
                ec.load("cache.bin");
                std::vector<frontend::range> players;
                for(int i=1; i!= argc; ++i){
                        players.emplace_back( frontend::parse(argv[i]) );
                }

                run(players);
                return 0;
        }
} // anon

int main(int argc, char** argv){
        return pokerstove_driver(argc, argv);
}
