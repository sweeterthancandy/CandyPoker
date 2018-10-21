#include <thread>
#include <numeric>
#include <fstream>
#include <atomic>

#include <boost/format.hpp>
#include <boost/timer/timer.hpp>
#include <boost/log/trivial.hpp>

#include "app/pretty_printer.h"

#include "ps/base/algorithm.h"
#include "ps/base/board_combination_iterator.h"
#include "ps/base/cards.h"
#include "ps/base/frontend.h"
#include "ps/base/holdem_board_decl.h"
#include "ps/base/rank_hasher.h"
#include "ps/base/suit_hasher.h"
#include "ps/base/tree.h"

#include "ps/eval/class_cache.h"
#include "ps/eval/evaluator_6_card_map.h"
#include "ps/eval/instruction.h"
#include "ps/eval/pass_mask_eval.h"
#include "ps/support/config.h"
#include "ps/support/index_sequence.h"



#include "ps/support/command.h"
//#include <boost/program_options.hpp>

using namespace ps;

namespace {

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
        freq_obs_flop_rank(){
                static std::map<std::string, int> aux = {
                        {"Royal Flush"   ,10},
                        {"Straight Flush", 9},
                        {"Quads"         , 8},
                        {"Full House"    , 7},
                        {"Flush"         , 6},
                        {"Straight"      , 5},
                        {"Trips"         , 4},
                        {"Two pair"      , 3},
                        {"One pair"      , 2},
                        {"High Card"     , 1},
                };
                auto f = [_=aux](std::string const& name){
                        return _.find(name)->second;
                };
                order_with(f);
        }
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
struct freq_obs_flop_rank_user : frequency_table_observer_excluse_stat{
        freq_obs_flop_rank_user(){
                static std::map<std::string, int> aux = {
                        {"Royal Flush"        ,11},
                        {"Straight Flush"     ,10},
                        {"Quads"              , 9},
                        {"Full House"         , 8},
                        {"Flush"              , 7},
                        {"Straight"           , 6},
                        {"Trips"              , 5},
                        {"Two pair"           , 4},
                        {"One pair"           , 3},
                        {"High Card - 2 Overs", 2},
                        {"High Card - 1 Over", 1},
                        {"High Card - Unders" , 0},
                };
                auto f = [_=aux](std::string const& name){
                        return _.find(name)->second;
                };
                order_with(f);
        }
        virtual void observe(card_vector const& cv, ranking_t r)override{
                BOOST_ASSERT(cv.size()==5 && "precondition failed");
                static rank_world rankdev;
                auto rd = rankdev[r];
                auto name = [&]()->std::string{
                        switch(rd.category()){
                        case HR_HighCard:
                        {
                                auto max_board_rank = std::max({card_rank_from_id(cv[0]),
                                                                card_rank_from_id(cv[1]),
                                                                card_rank_from_id(cv[2])});
                                auto max_hand_rank = std::max({card_rank_from_id(cv[3]),
                                                               card_rank_from_id(cv[4])});
                                auto min_hand_rank = std::min({card_rank_from_id(cv[3]),
                                                               card_rank_from_id(cv[4])});
                                if( max_board_rank < min_hand_rank ){
                                        return "High Card - 2 Overs";
                                } else if( max_board_rank < max_hand_rank ){
                                        return "High Card - 1 Over";
                                } else{
                                        return "High Card - Unders";
                                }
                                break;
                        }
                        default:
                                return rd.name();
                        }
                }();
                choose(name);
        }
        virtual std::string stat_name()const override{ return "Detailed Rank"; }
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
                lines.emplace_back(LineBreak);
                bool first = true;
                for(auto& _ : obs_){
                        if( ! first ){
                                lines.emplace_back(LineBreak);
                        }
                        _->emit(lines);
                        first = false;
                }
                lines.emplace_back(LineBreak);
                RenderTablePretty(std::cout, lines);
        }
private:
        evaluator_6_card_map* eval = evaluator_6_card_map::instance();
        std::vector<std::shared_ptr<frequency_table_observer> > obs_;
};


struct FlopZilla : Command{
        enum{ Debug = 1 };
        explicit
        FlopZilla(std::vector<std::string> const& args):args_{args}{}
        virtual int Execute()override{



                enum{ DefaultDealCount = 3};
                size_t deal_count = DefaultDealCount;
                card_vector board;
                holdem_hand_vector hv; 
                frontend::range front_range;
                
                int arg_ptr = 0;
                // first parse options
                for(;arg_ptr < args_.size();){
                        switch(args_.size()-arg_ptr){
                        default:
                        case 2:
                        if( args_[arg_ptr] == "--deal" ){
                                deal_count = boost::lexical_cast<size_t>(args_[arg_ptr+1]);
                                if( ! (3 <= deal_count && deal_count <= 5 ) ){
                                        std::cerr << "bad deal count\n";
                                        return EXIT_FAILURE;
                                }
                                arg_ptr += 2;
                                continue;
                        }
                        if( args_[arg_ptr] == "--board"){
                                std::string board_s = args_[arg_ptr+1];
                                board = card_vector::parse(board_s);
                                if( ! (3 <= board.size() && board.size() <= 5 ) ){
                                        std::cerr << "unable to parse board (" << board_s << ")\n";
                                        return EXIT_FAILURE;
                                }
                                arg_ptr += 2;
                                continue;
                        }
                        case 1:
                                // fallback to implicit option
                        front_range = frontend::parse(args_[arg_ptr]);
                        hv = expand(front_range).to_holdem_vector();
                        ++arg_ptr;
                        }
                }
                if( hv.empty()){
                        std::cerr << "need range\n";
                        return EXIT_FAILURE;
                }
                if( arg_ptr != args_.size()){
                        std::cerr << "unable to parse options at " << args_[arg_ptr] << "\n";
                        return EXIT_FAILURE;
                }
                

                
                if( Debug ){
                        std::cout << "board => " << board << "\n"; // __CandyPrint__(cxx-print-scalar,board)
                        std::cout << "front_range => " << front_range << "\n"; // __CandyPrint__(cxx-print-scalar,front_range)
                        std::cout << "expand(front_range) => " << expand(front_range) << "\n"; // __CandyPrint__(cxx-print-scalar,expand(front_range))
                        std::cout << "hv => " << hv << "\n"; // __CandyPrint__(cxx-print-scalar,hv)
                }

                frequency_table_builder freq_table;
                freq_table
                        .use_observer(std::make_shared<freq_obs_flop_rank>())
                        .use_observer(std::make_shared<freq_obs_flush_draw>())
                        .use_observer(std::make_shared<freq_obs_straight_draw>())
                        .use_observer(std::make_shared<freq_obs_flop_rank_user>())
                ;

                if( board.size() ){
                        auto const& b = board;
                        for(size_t idx=0;idx!=hv.size();++idx){
                                auto hd = hv.decl_at(idx);

                                if( b.mask() & hd.mask())
                                        continue;

                                auto full = b;
                                full.push_back(hd.first());
                                full.push_back(hd.second());

                                freq_table.add(full);
                        }
                } else {
                        for(board_combination_iterator iter(deal_count),end;iter!=end;++iter){
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



/*
        Command that can rank all hands
 */
struct HandRanking : Command{
        explicit
        HandRanking(std::vector<std::string> const& args):args_{args}{}
        virtual int Execute()override{


                enum{ Debug = true };
                
                holdem_hand_vector hv; 
                frontend::range front_range;
                card_vector board;

                boost::optional<holdem_id> specific_hand;

                int arg_ptr = 0;
                // first parse options
                for(;arg_ptr < args_.size();){
                        switch(args_.size()-arg_ptr){
                        default:
                        case 2:
                        if( args_[arg_ptr] == "--board"){
                                std::string board_s = args_[arg_ptr+1];
                                board = card_vector::parse(board_s);
                                if( ! (3 <= board.size() && board.size() <= 5 ) ){
                                        std::cerr << "unable to parse board (" << board_s << ")\n";
                                        return EXIT_FAILURE;
                                }
                                arg_ptr += 2;
                                continue;
                        }
                        if( args_[arg_ptr] == "--hand"){
                                std::string s = args_[arg_ptr+1];
                                specific_hand = holdem_hand_decl::parse(s).id();
                                arg_ptr += 2;
                                continue;
                        }
                        case 1:
                                // fallback to implicit option
                        front_range = frontend::parse(args_[arg_ptr]);
                        hv = expand(front_range).to_holdem_vector();
                        ++arg_ptr;
                        }
                }
                if( hv.empty()){
                        hv = expand(frontend::parse("100%")).to_holdem_vector();
                }
                if( Debug ){
                        std::cout << "board => " << board << "\n"; // __CandyPrint__(cxx-print-scalar,board)
                        std::cout << "hv => " << hv << "\n"; // __CandyPrint__(cxx-print-scalar,hv)
                        std::cout << "hv.size() => " << hv.size() << "\n"; // __CandyPrint__(cxx-print-scalar,hv.size())
                }

                evaluator_6_card_map* eval = evaluator_6_card_map::instance();

                std::map<holdem_id, std::vector<ranking_t> > M;

                auto emit = [&](holdem_id id, auto const& cv){
                        // assume that cv makes sense, no doubles etc
                        ranking_t R = eval->rank(board[0],cv[1],cv[2],cv[3],cv[4],cv[5],cv[6]);
                        evaluator_6_card_map* eval = evaluator_6_card_map::instance();
                        M[id].push_back(R);
                };

                card_vector full_board = board;
                full_board.resize(7);
                for(board_combination_iterator iter(2, board),end;iter!=end;++iter){
                        auto const& deal = *iter;

                        if( board.mask() & deal.mask() )
                                continue;
                        auto mask = board.mask() | deal.mask();

                        full_board[3] = deal[0];
                        full_board[4] = deal[1];


                        for(size_t idx=0;idx!=hv.size();++idx){
                                auto hd = hv.decl_at(idx);

                                if( mask & hd.mask())
                                        continue;

                                full_board[5] = hd.first();
                                full_board[6] = hd.second();

                                emit(hv[idx], full_board);
                        }
                }

                std::list<std::tuple<double, holdem_hand_vector > > avg_view;

                for(auto const& p : M ){
                        auto average = std::accumulate(p.second.begin(), p.second.end(), 0) * 1.0 / p.second.size();
                        //std::cout << p.first << "->" << detail::to_string(p.second) << "\n";
                        //std::cout << p.first << "->" << average << "\n";

                        avg_view.emplace_back(average, holdem_hand_vector{p.first});
                }
                avg_view.sort([](auto const& l, auto const& r){
                        return std::get<0>(l) < std::get<0>(r);
                });

                if( avg_view.size()){
                        auto head = avg_view.begin();
                        auto iter = head;
                        ++iter;
                        auto end = avg_view.end();
                        std::vector<decltype(iter)> to_erase;
                        for(;iter!=end;++iter){
                                constexpr double epsilon = 0.001;
                                bool merge_with_head = ( std::fabs(std::get<0>(*head) - std::get<0>(*iter)) < epsilon );
                                if( merge_with_head ){
                                        to_erase.push_back(iter);
                                        std::get<1>(*head).push_back(std::get<1>(*iter).back());
                                } else{
                                        head = iter;
                                }
                        }
                        for(auto const& iter : to_erase ){
                                avg_view.erase(iter);
                        }
                }


                for(auto const& _ : avg_view ){
                        std::cout << std::get<1>(_) << "->" << std::get<0>(_) << "\n";
                }

                using namespace Pretty;
                std::vector<LineItem> lines;
                lines.emplace_back(LineBreak);
                if( specific_hand ){
                        lines.emplace_back(std::vector<std::string>{"Hands", "Ranking", "P", "?"});
                } else {
                        lines.emplace_back(std::vector<std::string>{"Hands", "Ranking", "P"});
                }
                lines.emplace_back(LineBreak);
                size_t total_hands = M.size();
                size_t running_total = 0;
                for(auto const& _ : avg_view ){
                        running_total += std::get<1>(_).size();
                        double pct = running_total * 100.0 / total_hands;
                        std::vector<std::string> line{std::get<1>(_).to_string(),
                                boost::lexical_cast<std::string>(std::get<0>(_)),
                                boost::lexical_cast<std::string>(pct)};
                        if( specific_hand ){
                                std::string tag;
                                auto const& v = std::get<1>(_);
                                if( std::find(v.begin(),v.end(),specific_hand.get()) != v.end()){
                                        tag = "x";
                                }
                                line.push_back(tag);
                        }
                        lines.push_back(std::move(line));
                }
                lines.emplace_back(LineBreak);
                RenderTablePretty(std::cout, lines);


                

                return EXIT_SUCCESS;
        }
private:
        std::vector<std::string> args_;
};
static TrivialCommandDecl<HandRanking> HandRankingDecl{"hand-ranking"};

} // end namespace anon


