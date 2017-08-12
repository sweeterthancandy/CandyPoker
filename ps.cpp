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
struct class_range_equity_future{
        using result_t = std::shared_future<
                std::shared_ptr<equity_breakdown>
        >;
        class_range_equity_future()
        {
        }
        result_t schedual_group(support::processor::process_group& pg, holdem_class_range_vector const& players){
                
                std::vector< std::tuple< std::vector<int>, result_t > > items;

                for( auto hv : players.get_hand_vectors()){
                        auto p =  permutate_for_the_better(hv) ;
                        auto& perm = std::get<0>(p);
                        auto const& perm_players = std::get<1>(p);
                        auto fut = ef_.schedual_group(pg, perm_players);
                        items.emplace_back(perm, fut);
                }
                pg.sequence_point();
                auto task = std::make_shared<std::packaged_task<std::shared_ptr<equity_breakdown>()> >(
                        [n_=players.size(),items_=std::move(items),this](){
                        auto result = std::make_shared<equity_breakdown_matrix_aggregator>(n_);
                        for( auto& t : items_ ){
                                result->append(
                                        *std::make_shared<equity_breakdown_permutation_view>(
                                                std::get<1>(t).get(),
                                                std::get<0>(t)));
                        }
                        return result;
                });
                result_t fut = task->get_future();
                m_.emplace(players, fut);
                pg.push([task]()mutable{ (*task)(); });
                return fut;
        }
        result_t schedual_group(std::vector<holdem_range>const& players)const override{
                auto result = std::make_shared<equity_breakdown_matrix_aggregator>(players.size());

                detail::cross_product_vec([&](auto const& byclass){
                        detail::cross_product_vec([&](auto const& byhand){
                                holdem_hand_vector v;
                                for( auto iter : byhand ){
                                        v.push_back( (*iter).hand().id() );
                                }
                                result->append(*ec.evaluate( v ));
                        }, byclass);
                }, players);
                return result;
        }
private:
        mutable equity_future ef_;
        std::map< holdem_hand_vector, result_t > m_;
};
#endif

#if 0
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
#endif

struct processor{

        using work_t = std::function<void()>;

        enum Ctrl{
                Ctrl_Success,                    // When there is something to schedule
                Ctrl_NothingLeftButNotFinished,  // When everything is scheduled, but not finished
                Ctrl_NotReady,                   // When we're waiting for a task to finish first
                Ctrl_Finished                    // When everything has been scheduled and finished     
        };

        struct schedulable : std::enable_shared_from_this<schedulable>{
                virtual std::tuple<Ctrl, boost::optional<work_t> > select()=0;
        };

        struct single_task : schedulable{
                enum State{
                        State_ReadyToRun,
                        State_Selected,
                        State_Finished
                };
                explicit single_task(work_t w)
                        :w_{std::move(w)}
                {
                }
                std::tuple<Ctrl, boost::optional<work_t> > select()override{
                        switch(state_){
                        case State_ReadyToRun:
                                state_ = State_Selected;
                                return std::make_tuple(
                                        Ctrl_Success,
                                        work_t{[_w=std::move(w_),handle=shared_from_this(),this](){
                                                _w();
                                                state_ = State_Finished;
                                        }}
                                );
                        case State_Selected:
                                return std::make_tuple(
                                        Ctrl_NothingLeftButNotFinished,
                                        boost::none
                                );
                        case State_Finished:
                                return std::make_tuple(
                                        Ctrl_Finished,
                                        boost::none
                                );
                        }
                        __builtin_unreachable();
                }
        private:
                std::atomic_int state_{State_ReadyToRun};
                work_t w_;
        };


        struct unsequenced_group : schedulable{
                // Returns
                //      
                //      Ctrl_Success                    => one child has something to select()
                //      Ctrl_NotReady                   => no child had something to select(), 
                //                                         but one or more have something 
                //                                         soon
                //      Ctrl_NothingLeftButNotFinished  => have nothing left to select(), but
                //                                         some children are still running
                //      Ctrl_Finished                   => every child has finished
                //
                void push(std::shared_ptr<schedulable> ptr){
                        vec_.push_back(std::move(ptr));
                }

                std::tuple<Ctrl, boost::optional<work_t> > select()override{
                        // They get selected in order ie
                        //     {p0_Start, p1_start, p2_start,...}
                        
                        Ctrl default_reason = Ctrl_Finished;
                        for( auto& ptr : vec_){
                                // There might be something to schedule,
                                // but it might not be ready
                                auto ret = ptr->select();
                                switch(std::get<0>(ret)){
                                case Ctrl_Success:
                                        ++selected_;
                                        return std::make_tuple(
                                                Ctrl_Success,
                                                work_t{[_w=std::get<1>(ret).get(),handle=shared_from_this(),this](){
                                                        _w();
                                                        ++done_;
                                                }}
                                        );
                                case Ctrl_NotReady:
                                        default_reason = Ctrl_NotReady;
                                        continue;
                                case Ctrl_NothingLeftButNotFinished:
                                        // If there's already a reason, then the current
                                        // reason is better
                                        if( default_reason == Ctrl_Finished )
                                                default_reason = Ctrl_NothingLeftButNotFinished;
                                        continue;
                                case Ctrl_Finished:
                                        continue;
                                }
                                __builtin_unreachable();
                        }
                        return std::make_tuple(
                                default_reason,
                                boost::none
                        );
                }
        private:
                std::atomic_int selected_{0};
                std::atomic_int done_{0};
                std::vector<std::shared_ptr<schedulable> > vec_;
        }; 
        
        struct sequenced_group : schedulable{
                void push(std::shared_ptr<schedulable> ptr){
                        vec_.push_back(std::move(ptr));
                }
                std::tuple<Ctrl, boost::optional<work_t> > select()override{
                        // They get selected in order ie
                        //     {p0_Start, p0_end, p1_start, p1_end,...}
                        
                        for(;iter_ < vec_.size();){

                                auto ret = vec_[iter_]->select();
                                switch(std::get<0>(ret)){
                                case Ctrl_Success:
                                        ++selected_;
                                        return std::make_tuple(
                                                Ctrl_Success,
                                                work_t{[_w=std::get<1>(ret).get(),handle=shared_from_this(),this](){
                                                        _w();
                                                        ++done_;
                                                }}
                                        );
                                case Ctrl_NotReady:
                                        return std::move(ret);
                                case Ctrl_NothingLeftButNotFinished:
                                        return std::make_tuple(
                                                ( iter_ + 1 == vec_.size() ?
                                                        Ctrl_NothingLeftButNotFinished :
                                                        Ctrl_NotReady ),
                                                boost::none
                                        );
                                case Ctrl_Finished:
                                        ++iter_;
                                        continue;
                                }
                                __builtin_unreachable();
                        }
                        // We only get here by a sequence of zero or
                        // more Ctrl_Finished
                        return std::make_tuple(
                                Ctrl_Finished,
                                boost::none
                        );
                }
        private:
                size_t iter_{0};
                std::atomic_int selected_{0};
                std::atomic_int done_{0};
                std::vector<std::shared_ptr<schedulable> > vec_;
        }; 

        // This is just for creationable purposes,
        // because we only every create process_groups in
        // the form
        //
        //      auto pg0 = std::make_shared<process_group>();
        //      pg0->add(f);
        //      pg0->add(g);
        //      pg0->sequence_point();
        //      pg0->add(h);
        //      auto pg1 = std::make_shared<process_group>();
        //      pg1->add(pg1);
        //      pg1->add(i);
        //      pg1->sequence_point();
        //      pg1->add(j);
        //
        //      proc.accept(pg1);
        //      
        struct process_group{
                process_group()
                        :group_{std::make_shared<sequenced_group>()}
                {}

                /////////////////////////////////////////////////////////
                //
                //           Creational interface
                //
                /////////////////////////////////////////////////////////
                void sequence_point(){
                        if( working_stack_.empty())
                                return;
                        auto g = std::make_shared<unsequenced_group>();
                        for( auto& ptr : working_stack_ ){
                                g->push( std::move(ptr) );
                        }
                        group_->push(std::move(g));
                        working_stack_.clear();
                }
                void push(work_t w){
                        working_stack_.push_back(
                                std::make_shared<single_task>(std::move(w))
                        );
                }
                auto compile(){
                        this->sequence_point();
                        return std::move(group_);
                }
                void push(std::unique_ptr<process_group> pg){
                        working_stack_.push_back(
                                pg->compile()
                        );
                }
        private:
                std::shared_ptr<sequenced_group> group_;
                std::vector< std::shared_ptr<schedulable> > working_stack_;
        };

        processor()
                :head_{std::make_shared<unsequenced_group>()}
        {
        }
        ~processor(){
                join(); // maybe
        }
        void join(){
                //std::cout << "join()\n";
                running_ = false;
                no_work_barrier_.notify_all();
                if( threads_.size() ){
                        for( auto& t : threads_ )
                                t.join();
                        threads_.clear();
                }
        }
        void accept( std::unique_ptr<process_group> group ){
                //std::cout << "accept()\n";
                std::unique_lock<std::mutex> lock(mtx_);
                head_->push(group->compile());
                no_work_barrier_.notify_all();
        }
        void spawn(){
                //std::cout << "spawn()\n";
                threads_.emplace_back(
                        [this]()mutable{
                        __main__();
                });
        }
private:
        void __main__(){
                //std::cout << "__main__()\n";
                for(;;){
                        decltype(this->head_->select()) ret;
                        {
                                //std::cout << "locking()\n";
                                std::unique_lock<std::mutex> lock(mtx_);
                                ret = head_->select();
                                switch(std::get<0>(ret)){
                                case Ctrl_Success:
                                        break;
                                case Ctrl_NotReady:
                                case Ctrl_NothingLeftButNotFinished:
                                case Ctrl_Finished:
                                        if( ! running_ )
                                                return;
                                        no_work_barrier_.wait(lock);
                                        continue;
                                }
                        }
                        std::get<1>(ret).get()();
                        std::unique_lock<std::mutex> lock(mtx_);
                        no_work_barrier_.notify_all();
                }
        }
        std::mutex mtx_;
        std::condition_variable no_work_barrier_;
        std::thread schedular_thread_;
        std::vector<std::thread> threads_;
        std::atomic_bool running_{true};
        std::shared_ptr<unsequenced_group> head_;
};


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
        #if 1
        for(int i=0;i!=100;++i)
                proc.spawn();
                #endif
        proc.spawn();
        #if 1
        auto g = std::make_unique<processor::process_group>();
        g->push( make_proto("a"));
        g->push( make_proto("b"));
        g->sequence_point();
        g->push( make_proto("c"));
        auto h = std::make_unique<processor::process_group>();
        h->push(std::move(g));
        h->sequence_point();
        h->push([]{std::cout << "Finished\n"; });

        proc.accept(std::move(g));
        #endif
        proc.join();
}












