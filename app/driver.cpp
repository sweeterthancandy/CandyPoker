#include <thread>
#include <numeric>
#include <atomic>
#include <boost/format.hpp>
#include "ps/support/config.h"
#include "ps/base/frontend.h"
#include "ps/base/cards.h"
#include "ps/base/tree.h"
#include "ps/base/board_combination_iterator.h"
#include "ps/support/index_sequence.h"
#include "ps/eval/evaluator_5_card_map.h"
#include "ps/eval/evaluator_6_card_map.h"
#include "app/pretty_printer.h"
#include "ps/base/algorithm.h"
#include "ps/eval/instruction.h"
#include "ps/base/rank_hasher.h"
#include "ps/base/suit_hasher.h"
#include "ps/base/holdem_board_decl.h"
#include "ps/eval/pass_mask_eval.h"
#include "ps/eval/class_cache.h"

#include <boost/timer/timer.hpp>

#include <boost/log/trivial.hpp>

#include <Eigen/Dense>
#include <fstream>

#include "ps/support/command.h"
//#include <boost/program_options.hpp>

using namespace ps;

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



#if 0
struct SimpleCardEval : Command{
        explicit
        SimpleCardEval(std::vector<std::string> const& args):args_{args}{}
        virtual int Execute()override{

                std::vector<frontend::range> players;
                for(auto const& s : args_ ){
                        players.push_back( frontend::parse(s) );
                }

                auto instr_list = frontend_to_instruction_list(players);

                auto comp = std::make_shared<eval_computer>();

                computation_context comp_ctx{players.size()};
                auto result = comp->compute(comp_ctx, instr_list);

                pretty_print_equity_breakdown(std::cout, *result, args_);

                return EXIT_SUCCESS;
        }
private:
        std::vector<std::string> const& args_;
};
static TrivialCommandDecl<SimpleCardEval> SimpleCardEvalDecl{"eval"};
#endif



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








struct MaskEval : Command{
        explicit
        MaskEval(std::vector<std::string> const& args):args_{args}{}
        virtual int Execute()override{

                std::vector<frontend::range> players;
                for(auto const& s : args_ ){
                        players.push_back( frontend::parse(s) );
                }
                bool debug = false;
                


                computation_context comp_ctx{players.size()};

                computation_pass_manager mgr;
                if( debug )
                        mgr.add_pass<pass_print>();
                mgr.add_pass<pass_permutate>();
                mgr.add_pass<pass_sort_type>();
                mgr.add_pass<pass_collect>();
                if( debug )
                        mgr.add_pass<pass_print>();
                mgr.add_pass<pass_eval_hand_instr_vec>();
                if( debug )
                        mgr.add_pass<pass_print>();

                boost::timer::auto_cpu_timer at;

                instruction_list instr_list = frontend_to_instruction_list(players);
                auto result = mgr.execute(&comp_ctx, &instr_list);

                if( result ){
                        pretty_print_equity_breakdown_mat(std::cout, *result, args_);
                }

                return EXIT_SUCCESS;
        }
private:
        std::vector<std::string> const& args_;
};
static TrivialCommandDecl<MaskEval> MaskEvalDecl{"eval"};



struct IteratorDbg : Command{
        explicit
        IteratorDbg(std::vector<std::string> const& args):players_s_{args}{}
        virtual int Execute()override{
                enum{ MaxIter = 50 };

                std::cout << "holdem_hand_iterator\n";
                size_t n = 0;
                for(holdem_hand_iterator iter(5),end;iter!=end && n < MaxIter;++iter,++n){
                        std::cout << "    " <<  *iter << "\n";
                }

                std::cout << "holdem_class_iterator\n";
                n = 0;
                for(holdem_class_iterator iter(3),end;iter!=end && n < MaxIter;++iter,++n){
                        std::cout << "    " <<  *iter << "\n";
                }

                std::cout << "board_combination_iterator\n";
                n = 0;
                for(board_combination_iterator iter(3),end;iter!=end && n < MaxIter;++iter,++n){
                        std::cout << "    " <<  *iter << "\n";
                }

                return EXIT_SUCCESS;
        }
private:
        std::vector<std::string> const& players_s_;
};
static TrivialCommandDecl<IteratorDbg> IteratorDbgDecl{"iterator-dbg"};


struct PrintRanks : Command{
        explicit
        PrintRanks(std::vector<std::string> const& args):args_{args}{}
        virtual int Execute()override{
                rank_world rankdev;
                for(auto const& rd : rankdev ){
                        std::cout << rd << "\n";
                }

                return EXIT_SUCCESS;
        }
private:
        std::vector<std::string> const& args_;
};
static TrivialCommandDecl<PrintRanks> PrintRanksDecl{"print-ranks"};


struct frequency_table_observer{
        virtual ~frequency_table_observer()=default;
        virtual void observe(card_vector const& cv, ranking_t r)=0;
        virtual void emit(std::vector<Pretty::LineItem >& lines)const=0;
};
struct frequency_table_observer_excluse_stat : frequency_table_observer{
        using self_type = frequency_table_observer_excluse_stat;
        using order_fun_type = std::function<size_t(std::string const&)>;
        virtual void emit(std::vector<Pretty::LineItem >& lines)const override{
                using namespace Pretty;
                lines.emplace_back(std::vector<std::string>{stat_name(), "Count", "Prob", "Cum"});
                lines.emplace_back(LineBreak);
                
                std::vector<decltype(&*freq_table_.cbegin())> aux;
                size_t sigma = 0;
                for(auto const& _ : freq_table_){
                        aux.emplace_back(&_);
                        sigma += _.second;
                }
                if( order_fun_){
                        std::sort(aux.begin(), aux.end(), [this](auto const& l, auto const& r){
                                return order_fun_(l->first) > order_fun_(r->first);
                        });
                } else {
                        std::sort(aux.begin(), aux.end(), [](auto const& l, auto const& r){
                                return l->second > r->second;
                        });
                }
                size_t cum_total = 0;
                for(auto const& _ : aux){
                        cum_total += _->second;
                        auto pct = _->second * 100.0 / sigma;
                        auto cum_pct = cum_total * 100.0 / sigma;

                        std::vector<std::string> line;
                        line.push_back(_->first);
                        line.push_back(boost::lexical_cast<std::string>(_->second));
                        line.push_back(boost::lexical_cast<std::string>(pct));
                        line.push_back(boost::lexical_cast<std::string>(cum_pct));

                        lines.push_back(line);
                }
        }
protected:
        virtual std::string stat_name()const{ return "Rank"; }
        void choose(std::string const& tag){
                ++freq_table_[tag];
        }
        self_type& order_with(order_fun_type f){
                order_fun_ = f;
                return *this;
        }
        self_type& decl_row(std::string const& name){
                freq_table_[name];
                return *this;
        }
private:

        order_fun_type order_fun_;
        std::map<std::string, size_t> freq_table_;
};

struct freq_obs_flop_rank : frequency_table_observer_excluse_stat{
        virtual void observe(card_vector const& cv, ranking_t r)override{
                static rank_world rankdev;
                auto rd = rankdev[r];
                choose(rd.name());
        }
};
struct freq_obs_flush_draw : frequency_table_observer_excluse_stat{
        freq_obs_flush_draw(){
                auto f = [](std::string const& name){
                        return name == "Flush"       ? 3 :
                               name == "Four-Flush"  ? 2 :
                               name == "Three-Flush" ? 1 :
                                                       0
                        ;
                };
                order_with(f);
                decl_row("Flush");
                decl_row("Four-Flush");
                decl_row("Three-Flush");
        }
        virtual void observe(card_vector const& cv, ranking_t r)override{
                auto suit_val = suit_hasher::create(cv);
                auto name = [suit_val](){
                        if( suit_hasher::has_flush(suit_val)){
                                return "Flush";
                        } else if( suit_hasher::has_four_flush(suit_val)){
                                return "Four-Flush";
                        } else if( suit_hasher::has_three_flush(suit_val)){
                                return "Three-Flush";
                        } else {
                                return "None";
                        }
                }();
                choose(name);
        }
        virtual std::string stat_name()const override{ return "Flush Draw"; }
};
namespace permutations{
        static constexpr std::array<size_t, 4> choose_4_5_0 = {  1,2,3,4};
        static constexpr std::array<size_t, 4> choose_4_5_1 = {0,  2,3,4};
        static constexpr std::array<size_t, 4> choose_4_5_2 = {0,1,  3,4};
        static constexpr std::array<size_t, 4> choose_4_5_3 = {0,1,2,  4};
        static constexpr std::array<size_t, 4> choose_4_5_4 = {0,1,2,3  };
        static constexpr std::array<std::array<size_t, 4>, 5> choose_4_5 = {
                choose_4_5_0, choose_4_5_1, choose_4_5_2, choose_4_5_3, choose_4_5_4
        };
} // end namespace permutations

struct freq_obs_straight_draw : frequency_table_observer_excluse_stat{
        freq_obs_straight_draw(){
                // 01234
                for(size_t idx=0;idx+4<13;++idx){
                        auto proto_val = prime_rank_map::create(idx,idx+1,idx+2,idx+3,idx+4);
                        straight_.insert(proto_val);
                }
                // 0123
                for(size_t idx=0;idx+3<13;++idx){
                        auto proto_val = prime_rank_map::create(idx,idx+1,idx+2,idx+3);
                        four_straight_.insert(proto_val);
                }
                //  0_234_6
                for(size_t idx=0;idx+6<13;++idx){
                        auto proto_val = prime_rank_map::create(idx,idx+2,idx+3,idx+4,idx+6);
                        double_gutter_.insert(proto_val);
                }
                
                auto f = [](std::string const& name){
                        return name == "Straight"       ? 3 :
                               name == "Four-Straight"  ? 2 :
                               name == "Double-Gutter"  ? 1 :
                                                       0
                        ;
                };
                order_with(f);
                decl_row("Straight");
                decl_row("Four-Straight");
                decl_row("Double-Gutter");
        }
        virtual void observe(card_vector const& cv, ranking_t r)override{
                BOOST_ASSERT(cv.size() == 5 );

                auto name = [&](){
                        auto R = prime_rank_map::create(cv);
                        if( straight_.count(R)){
                                return "Straight";
                        }
                        for(size_t idx=0;idx!=5;++idx){
                                auto const& p = permutations::choose_4_5[idx];
                                auto val = prime_rank_map::create_from_cards(cv[p[0]],
                                                                             cv[p[1]],
                                                                             cv[p[2]],
                                                                             cv[p[3]]);
                                if( four_straight_.count(val) ){
                                        return "Four-Straight";
                                }
                        }
                        if( double_gutter_.count(R) ){
                                return "Double-Gutter";
                        }
                        return "None";
                }();
                choose(name);
        }
        virtual std::string stat_name()const override{ return "Straight Draw"; }
private:
        std::set<prime_rank_map::prime_rank_t> straight_;
        std::set<prime_rank_map::prime_rank_t> four_straight_;
        std::set<prime_rank_map::prime_rank_t> double_gutter_;
};

struct frequency_table_builder{
        frequency_table_builder(){
        }
        frequency_table_builder& use_observer(std::shared_ptr<frequency_table_observer> ptr){
                obs_.push_back(ptr);
                return *this;
        }
        void add(card_vector const& cv){
                ranking_t R;
                switch(cv.size()){
                case 5:
                        R = eval->rank(cv[0],cv[1],cv[2],cv[3],cv[4]);
                        break;
                case 6:
                        R = eval->rank(cv[0],cv[1],cv[2],cv[3],cv[4],cv[5]);
                        break;
                case 7:
                        R = eval->rank(cv[0],cv[1],cv[2],cv[3],cv[4],cv[5],cv[6]);
                        break;
                default:
                        R = 0;
                }

                for(auto& _ : obs_){
                        _->observe(cv, R);
                }
        }
        void display(std::string const& title)const{
                using namespace Pretty;

                std::vector< LineItem > lines;
                bool first = true;
                for(auto& _ : obs_){
                        if( ! first ){
                                lines.emplace_back(LineBreak);
                        }
                        _->emit(lines);
                        first = false;
                }

                RenderTablePretty(std::cout, lines);
        }
private:
        evaluator_6_card_map* eval = evaluator_6_card_map::instance();
        std::vector<std::shared_ptr<frequency_table_observer> > obs_;
};

struct PokerProbability : Command{
        explicit
        PokerProbability(std::vector<std::string> const& args):args_{args}{}
        virtual int Execute()override{

                frequency_table_builder tab_5;
                tab_5.use_observer(std::make_shared<freq_obs_flop_rank>());
                frequency_table_builder tab_7;
                tab_7.use_observer(std::make_shared<freq_obs_flop_rank>());

                for(board_combination_iterator iter(5),end;iter!=end;++iter){
                        tab_5.add(*iter);
                }
                tab_5.display("5-Card Probability Table");
                for(board_combination_iterator iter(7),end;iter!=end;++iter){
                        tab_7.add(*iter);
                }
                tab_7.display("7-Card Probability Table");
                return EXIT_SUCCESS;
        }
private:
        std::vector<std::string> args_;
};
static TrivialCommandDecl<PokerProbability> PokerProbabilityDecl{"poker-prob"};

struct FlopZilla : Command{
        explicit
        FlopZilla(std::vector<std::string> const& args):args_{args}{}
        virtual int Execute()override{


                frontend::range front_range = frontend::parse(args_.at(0));
                auto hv = expand(front_range).to_holdem_vector();
                
                #if 0
                std::cout << "front_range => " << front_range << "\n"; // __CandyPrint__(cxx-print-scalar,front_range)
                std::cout << "expand(front_range) => " << expand(front_range) << "\n"; // __CandyPrint__(cxx-print-scalar,expand(front_range))
                std::cout << "hv => " << hv << "\n"; // __CandyPrint__(cxx-print-scalar,hv)
                #endif

                frequency_table_builder freq_table;
                freq_table
                        .use_observer(std::make_shared<freq_obs_flop_rank>())
                        .use_observer(std::make_shared<freq_obs_flush_draw>())
                        .use_observer(std::make_shared<freq_obs_straight_draw>());


                for(board_combination_iterator iter(3),end;iter!=end;++iter){
                        auto const& b = *iter;

                        for(size_t idx=0;idx!=hv.size();++idx){
                                auto hd = hv.decl_at(idx);

                                if( b.mask() & hd.mask())
                                        continue;

                                auto full = b;
                                full.push_back(hd.first());
                                full.push_back(hd.second());

                                freq_table.add(full);
                        }
                }

                std::stringstream title;
                title << "FlopZilla for " << args_.at(0);
                freq_table.display(title.str());
                return EXIT_SUCCESS;
        }
private:
        std::vector<std::string> args_;
};
static TrivialCommandDecl<FlopZilla> FlopZillaDecl{"flopzilla"};







int main(int argc, char** argv){
        try{
                CommandDecl::Driver(argc, argv);
        } catch(std::exception const& e){
                std::cerr << "Caught exception: " << e.what() << "\n";
        }
        return EXIT_SUCCESS;
}

