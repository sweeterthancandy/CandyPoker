#include <thread>
#include <atomic>
#include <boost/format.hpp>
#include "ps/support/config.h"
#include "ps/base/frontend.h"
#include "ps/base/cards.h"
#include "ps/base/tree.h"
#include "ps/base/board_combination_iterator.h"
#include "ps/eval/class_equity_evaluator.h"
#include "ps/eval/holdem_class_eval_cache.h"
#include "ps/base/holdem_class_vector.h"
#include "ps/support/index_sequence.h"
#include "ps/eval/class_equity_evaluator_quick.h"
#include "ps/support/index_sequence.h"
#include "ps/eval/evaluator_6_card_map.h"
#include "app/pretty_printer.h"
#include "ps/base/algorithm.h"
#include "ps/base/instruction.h"
#include "ps/base/rank_hasher.h"
#include "ps/base/suit_hasher.h"
#include "ps/base/holdem_board_decl.h"

#include <boost/timer/timer.hpp>
#include <boost/asio.hpp>

#include <Eigen/Dense>

using namespace ps;

struct Command{
        virtual ~Command()=default;
        virtual int Execute()=0;
};

struct CommandDecl{
        CommandDecl(){
                World().push_back(this);
        }
        virtual std::shared_ptr<Command> TryMakeSingle(std::string const& name, std::vector<std::string> const& args)=0;
        virtual void PrintHelpSingle()const{
        }

        static std::vector<CommandDecl*>& World(){
                static std::vector<CommandDecl*>* ptr = new std::vector<CommandDecl*>{};
                return *ptr;
        }
        static std::shared_ptr<Command> TryMake(std::string const& name, std::vector<std::string> const& args){
                std::shared_ptr<Command> cmd;
                for( auto ptr : World()){
                        auto ret = ptr->TryMakeSingle(name, args);
                        if( ret ){
                                if( cmd ){
                                        BOOST_THROW_EXCEPTION(std::domain_error("not injective"));
                                }
                                cmd = ret;
                        }
                }
                return cmd;
        }
        static void PrintHelp(std::string const& exe_name){
                auto offset = exe_name.size();
                std::cout << exe_name << " <cmd> <option>...\n";
                for(auto ptr : World()){
                        ptr->PrintHelpSingle();
                }
        }
        static int Driver(int argc, char** argv){
                if( argc < 2 ){
                        PrintHelp(argv[0]);
                        return EXIT_FAILURE;
                }
                std::string cmd_s = argv[1];
                std::vector<std::string> args(argv+2, argv+argc);
                auto cmd = TryMake(cmd_s, args);
                if( ! cmd ){
                        std::cerr << "can't find cmd " << cmd_s << "\n";
                        PrintHelp(argv[0]);
                        return EXIT_FAILURE;
                }
                return cmd->Execute();
        }
};
template<class T>
struct TrivialCommandDecl : CommandDecl{
        explicit TrivialCommandDecl(std::string const& name):name_{name}{}
        virtual void PrintHelpSingle()const override{
                std::cout << "    " << name_ << "\n";
        }
        virtual std::shared_ptr<Command> TryMakeSingle(std::string const& name, std::vector<std::string> const& args)override{
                if( name != name_)
                        return std::shared_ptr<Command>{};
                return std::make_shared<T>(args);
        }
private:
        std::string name_;
};

struct PrintTree : Command{
        explicit
        PrintTree(std::vector<std::string> const& args):players_s_{args}{}
        virtual int Execute()override{
                std::vector<frontend::range> players;
                for(auto const& s : players_s_ ){
                        players.push_back( frontend::parse(s) );
                }
                tree_range root( players );
                root.display();

                return EXIT_SUCCESS;
        }
private:
        std::vector<std::string> const& players_s_;
};
static TrivialCommandDecl<PrintTree> PrintTreeDecl{"print-tree"};

struct StandardForm : Command{
        explicit
        StandardForm(std::vector<std::string> const& args):args_{args}{}
        virtual int Execute()override{

                holdem_class_vector cv;
                for(auto const& s : args_ ){
                        cv.push_back(s);
                }
                for( auto hvt : cv.to_standard_form_hands()){

                        std::cout << hvt << "\n";
                }

                return EXIT_SUCCESS;
        }
private:
        std::vector<std::string> const& args_;
};
static TrivialCommandDecl<StandardForm> StandardFormDecl{"standard-form"};

struct HandVectors : Command{
        explicit
        HandVectors(std::vector<std::string> const& args):args_{args}{}
        virtual int Execute()override{

                holdem_class_vector cv;
                for(auto const& s : args_ ){
                        cv.push_back(s);
                }
                for( auto hv : cv.get_hand_vectors()){
                        std::cout << "  " << hv << "\n";
                }

                return EXIT_SUCCESS;
        }
private:
        std::vector<std::string> const& args_;
};
static TrivialCommandDecl<HandVectors> HandVectorsDecl{"hand-vectors"};






        






struct SimpleCardEval : Command{
        explicit
        SimpleCardEval(std::vector<std::string> const& args):args_{args}{}
        virtual int Execute()override{
                equity_evaulator_principal eval;

                std::vector<frontend::range> players;
                for(auto const& s : args_ ){
                        players.push_back( frontend::parse(s) );
                }

                auto card_instr_list = frontend_to_card_instr(players);

                boost::timer::auto_cpu_timer at;
                equity_breakdown_matrix_aggregator agg(players.size());
                for(auto const& instr : card_instr_list ){
                        auto result = eval.evaluate(instr.get_vector());
                        agg.append_matrix(*result, instr.get_matrix());
                }

                pretty_print_equity_breakdown(std::cout, agg, args_);

                return EXIT_SUCCESS;
        }
private:
        std::vector<std::string> const& args_;
};
static TrivialCommandDecl<SimpleCardEval> SimpleCardEvalDecl{"eval"};



struct FrontendDbg : Command{
        explicit
        FrontendDbg(std::vector<std::string> const& args):players_s_{args}{}
        virtual int Execute()override{
                
                std::vector<std::string> title;
                title.push_back("literal");
                title.push_back("range");
                title.push_back("expanded");

                using namespace Pretty;
                std::vector< LineItem > lines;
                lines.push_back(title);
                lines.emplace_back(LineBreak);
                
                for(auto const& s : players_s_ ){
                        auto rng = frontend::parse(s);
                        auto expanded = expand(rng);
                        auto prim_rng = expanded.to_primitive_range();
                        std::vector<std::string> line;
                        line.push_back(s);
                        line.push_back(boost::lexical_cast<std::string>(rng));
                        line.push_back(boost::lexical_cast<std::string>(expanded));

                        try{
                                auto cv = expanded.to_class_vector();
                                line.push_back(boost::lexical_cast<std::string>(cv));
                        }catch(...){
                                line.push_back("error");
                        }
                        #if 0
                        try{
                                auto hv = expanded.to_holdem_vector();
                                line.push_back(boost::lexical_cast<std::string>(hv));
                        }catch(...){
                                line.push_back("error");
                        }
                        #endif

                        lines.push_back(line);
                }
                
                RenderTablePretty(std::cout, lines);
                
                return EXIT_SUCCESS;
        }
private:
        std::vector<std::string> const& players_s_;
};
static TrivialCommandDecl<FrontendDbg> FrontendDbgDecl{"frontend-dbg"};









namespace working{

struct evaluator_7_card_map : evaluator
{
        evaluator_7_card_map(){
                impl_ = &evaluator_factory::get("6_card_map");
                card_map_7_.resize(rhasher_.max());

                for(size_t i=0;i!=52;++i){
                        card_rank_device_[i] = card_decl::get(i).rank().id();
                }

                using iter_t = basic_index_iterator<
                        int, ordered_policy, rank_vector
                >;

                for(iter_t iter(7,13),end;iter!=end;++iter){
                        maybe_add_(*iter);
                }
        }
        ranking_t rank(long a, long b, long c, long d, long e)const override{
                return impl_->rank(a,b,c,d,e);
        }
        ranking_t rank(long a, long b, long c, long d, long e, long f)const override{
                return impl_->rank(a,b,c,d,e,f);
        }
        ranking_t rank(long a, long b, long c, long d, long e, long f, long g)const override{

                auto shash =  shasher_.create_from_cards(a,b,c,d,e,f,g);

                if( shasher_.has_flush(shash)){
                        //++miss;
                        return impl_->rank(a,b,c,d,e,f,g);
                }

                auto rhash = rhasher_.create_from_cards(a,b,c,d,e,f,g);
                auto ret = card_map_7_[rhash];

                return ret;
        }
        mutable std::atomic_int miss{0};
        mutable std::atomic_int hit{0};
        ranking_t rank(card_vector const& cv, size_t suit_hash, size_t rank_hash, long a, long b)const {

                if( shasher_.has_flush(suit_hash) ){
                        ++miss;
                        return 0;
                        return impl_->rank(a,b,cv[0], cv[1], cv[2], cv[3], cv[4]);
                }
                ++hit;
                auto ret = card_map_7_[rank_hash];
                return ret;
        }
private:
        ranking_t rank_from_rank_impl_(long a, long b, long c, long d, long e, long f, long g)const{
                return impl_->rank( card_decl::make_id(0,a),
                                    card_decl::make_id(0,b),
                                    card_decl::make_id(0,c),
                                    card_decl::make_id(0,d),
                                    card_decl::make_id(1,e),
                                    card_decl::make_id(1,f),
                                    card_decl::make_id(1,g) );
        }
        ranking_t rank_from_rank_(long a, long b, long c, long d, long e, long f, long g)const{
                return this->rank( card_decl::make_id(0,a),
                                   card_decl::make_id(0,b),
                                   card_decl::make_id(0,c),
                                   card_decl::make_id(0,d),
                                   card_decl::make_id(1,e),
                                   card_decl::make_id(1,f),
                                   card_decl::make_id(1,g) );
        }
        void maybe_add_(rank_vector const& b){
                // first check we don't have more than 4 of each card
                std::array<int, 13> aux = {0};
                for(size_t i=0;i!=7;++i){
                        ++aux[b[i]];
                }
                for(size_t i=0;i!=aux.size();++i){
                        if( aux[i] > 4 )
                                return;
                }
                auto hash = rhasher_.create( b[0], b[1], b[2], b[3], b[4], b[5], b[6] );

                auto val  = rank_from_rank_impl_( b[0], b[1], b[2], b[3], b[4], b[5], b[6] );

                //std::cout << detail::to_string(aux) << " - " << detail::to_string(b) << " => " << std::bitset<30>(static_cast<unsigned long long>(hash)).to_string() << "\n";
                //

                card_map_7_[hash] = val;
        }
        rank_hasher rhasher_;
        suit_hasher shasher_;
        evaluator* impl_;
        std::array<size_t, 52> card_rank_device_;
        std::vector<ranking_t> card_map_7_;
};


} // working






struct MaskEval : Command{
        explicit
        MaskEval(std::vector<std::string> const& args):args_{args}{}
        virtual int Execute()override{
                working::evaluator_7_card_map ev;
                holdem_board_decl w;
                rank_hasher rh;
                suit_hasher sh;

                std::vector<frontend::range> players;
                for(auto const& s : args_ ){
                        players.push_back( frontend::parse(s) );
                }
                
                auto card_instr_list = frontend_to_card_instr(players);

                boost::timer::auto_cpu_timer at;
                equity_breakdown_matrix_aggregator agg(players.size());
                for(auto const& instr : card_instr_list ){
                        auto const& hv   = instr.get_vector();
                        auto hv_mask = hv.mask();
                                
                        // put this here
                        std::vector<ranking_t> ranked(hv.size());

                        // cache stuff

                        std::vector<card_id> hv_first(hv.size());
                        std::vector<card_id> hv_second(hv.size());
                        std::vector<rank_id> hv_first_rank(hv.size());
                        std::vector<rank_id> hv_second_rank(hv.size());
                        std::vector<suit_id> hv_first_suit(hv.size());
                        std::vector<suit_id> hv_second_suit(hv.size());
                                
                        for(size_t i=0;i!=hv.size();++i){
                                auto const& hand{holdem_hand_decl::get(hv[i])};

                                hv_first[i]       = hand.first().id();
                                hv_first_rank[i]  = hand.first().rank().id();
                                hv_first_suit[i]  = hand.first().suit().id();
                                hv_second[i]      = hand.second().id();
                                hv_second_rank[i] = hand.second().rank().id();
                                hv_second_suit[i] = hand.second().suit().id();
                        }

                        auto sub = std::make_shared<equity_breakdown_matrix_aggregator>(players.size());
                        for(auto const& b : w ){

                                bool cond = (b.mask() & hv_mask ) == 0;
                                if(!cond){
                                        continue;
                                }
                                auto rank_proto = b.rank_hash();
                                auto suit_proto = b.suit_hash();


                                for(size_t i=0;i!=hv.size();++i){

                                        auto rank_hash = rank_proto;
                                        auto suit_hash = suit_proto;

                                        rank_hash = rh.append(rank_hash, hv_first_rank[i]);
                                        rank_hash = rh.append(rank_hash, hv_second_rank[i]);

                                        suit_hash = sh.append(suit_hash, hv_first_suit[i] );
                                        suit_hash = sh.append(suit_hash, hv_second_suit[i] );


                                        ranked[i] = ev.rank(b.board(),
                                                            suit_hash, rank_hash,
                                                            hv_first[i],
                                                            hv_second[i]);
                                }
                                detail::dispatch_ranked_vector{}(*sub, ranked);

                        }
                        agg.append_matrix(*sub, instr.get_matrix() );
                }

                pretty_print_equity_breakdown(std::cout, agg, args_);

                return EXIT_SUCCESS;
        }
private:
        std::vector<std::string> const& args_;
};
static TrivialCommandDecl<MaskEval> MaskEvalDecl{"mask-eval"};












int main(int argc, char** argv){
        try{
                CommandDecl::Driver(argc, argv);
        } catch(std::exception const& e){
                std::cerr << "Caught exception: " << e.what() << "\n";
        }
        return EXIT_FAILURE;
}

#if 0
struct create_class_cache_app{
        create_class_cache_app(){
                cache_ = &holdem_class_eval_cache_factory::get("main");
                eval_ = new class_equity_evaluator_quick;
        }
        void run(){
                boost::timer::auto_cpu_timer at;
                std::vector<std::thread> tg;
                {
                        boost::asio::io_service::work w(io_);
                        size_t num_threads = std::thread::hardware_concurrency();
                        for(size_t i=0;i!=num_threads;++i){
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
                cache_->save("result_3.bin");
        }
private:

        void calc_(holdem_class_vector const& vec){
                boost::timer::auto_cpu_timer at;
                boost::timer::cpu_timer timer;
                auto ret = eval_->evaluate(vec);
                cache_->lock();
                cache_->commit(vec, *ret);
                cache_->unlock();
                std::unique_lock<std::mutex> lock(mtx_);
                ++done_;
                std::string fmt = str(boost::format("%-11s took %%w seconds (%d/%d %.2f%%)")
                                      % vec % done_ % total_ % (static_cast<double>(done_)/total_*100));
                std::cout << timer.format(2, fmt) << "\n";
                std::cout << *ret << "\n";
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
#endif
