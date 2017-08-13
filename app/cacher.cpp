#include <thread>
#include <atomic>
#include <boost/format.hpp>
#include "ps/base/cards.h"
#include "ps/base/board_combination_iterator.h"
#include "ps/eval/class_equity_evaluator.h"
#include "ps/eval/holdem_class_eval_cache.h"

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
                        for(holdem_class_id i=0;i!=holdem_class_decl::max_id;++i){
                                for(holdem_class_id j=i;j!=holdem_class_decl::max_id;++j){
                                        ++total_;
                                        io_.post( [i,j,this]()
                                        {
                                                main_(i,j);
                                        });
                                }
                        }
                }
                for( auto& t : tg )
                        t.join();
                cache_->save("result.bin");
        }
private:

        void main_(holdem_class_id i, holdem_class_id j){
                holdem_class_vector vec{i,j};
                PRINT( vec.is_standard_form() );
                auto ret = eval_->evaluate(vec);
                PRINT(vec);
                std::cout << *ret << "\n";
                cache_->commit(vec, *ret);
                ++done_;
                std::cout << boost::format("%s/%s (%.2f%%)") % done_ % total_ % ( static_cast<double>(done_)/total_*100) << std::endl;
        }
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
