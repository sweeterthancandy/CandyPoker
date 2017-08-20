#include <thread>
#include <atomic>
#include <boost/format.hpp>
#include "ps/base/cards.h"
#include "ps/base/board_combination_iterator.h"
#include "ps/eval/class_equity_evaluator.h"
#include "ps/eval/holdem_class_eval_cache.h"
#include "ps/base/holdem_class_vector.h"
#include "ps/support/index_sequence.h"

#include <boost/timer/timer.hpp>
#include <boost/asio.hpp>

using namespace ps;

struct create_class_cache_app{
        create_class_cache_app(){
                cache_ = &holdem_class_eval_cache_factory::get("main");
                eval_ =  &class_equity_evaluator_factory::get("principal");
        }
        void run(){
                std::vector<std::thread> tg;
                {
                        boost::asio::io_service::work w(io_);
                        for(size_t i=0;i!=std::thread::hardware_concurrency();++i){
                                tg.emplace_back( [this](){ io_.run(); } );
                        }
                        for( holdem_class_iterator iter(2),end;iter!=end;++iter){
                                ++total_;
                                io_.post( [vec=*iter,this]()
                                {
                                        calc_(vec);
                                });
                        }
                }
                for( auto& t : tg )
                        t.join();
                //cache_->save("result.bin");
        }
private:

        void calc_(holdem_class_vector const& vec){
                boost::timer::cpu_timer timer;
                auto ret = eval_->evaluate(vec);
                cache_->commit(vec, *ret);
                std::unique_lock<std::mutex> lock(mtx_);
                ++done_;
                std::string fmt = str(boost::format("%-11s took %%w seconds (%d/%d %.2f%%)")
                                      % vec % done_ % total_ % (static_cast<double>(done_)/total_*100));
                std::cout << timer.format(2, fmt) << "\n";
        }
        std::mutex mtx_;
        boost::asio::io_service io_;
        holdem_class_eval_cache* cache_;
        class_equity_evaluator* eval_;
        std::atomic_int total_{0};
        std::atomic_int done_{0};
};

int main(){
        try{
                create_class_cache_app app;
                app.run();
                return EXIT_SUCCESS;
        } catch(std::exception const& e){
                std::cerr << "Caught exception: " << e.what() << "\n";
        }
        return EXIT_FAILURE;
}
