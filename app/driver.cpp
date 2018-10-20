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


struct frequency_table_builder{
private:
};

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

                auto* eval = evaluator_5_card_map::instance();
                
                rank_world rankdev;

                std::map<std::string, size_t> flop_rank_stat;

                for(board_combination_iterator iter(3),end;iter!=end;++iter){
                        auto const& b = *iter;

                        for(size_t idx=0;idx!=hv.size();++idx){
                                auto hd = hv.decl_at(idx);

                                if( b.mask() & hd.mask())
                                        continue;

                                auto full = b;
                                full.push_back(hd.first());
                                full.push_back(hd.second());

                                auto R = eval->rank(full[0],full[1],full[2],full[3],full[4]);

                                auto const& rd = rankdev[R];

                                #if 0
                                std::cout << hd << " x " << b << "(" << full << ") -> " << R << " : " << rd << "\n";

                                #endif
                                ++flop_rank_stat[rd.name()];

                        }
                }

                using namespace Pretty;

                std::vector< LineItem > lines;
                lines.emplace_back(std::vector<std::string>{"Rank", "Count", "%"});
                lines.emplace_back(LineBreak);
                
                std::vector<decltype(&*flop_rank_stat.cbegin())> aux;
                size_t sigma = 0;
                for(auto const& _ : flop_rank_stat){
                        aux.emplace_back(&_);
                        sigma += _.second;
                }
                std::sort(aux.begin(), aux.end(), [](auto const& l, auto const& r){
                        return l->second > r->second;
                });
                for(auto const& _ : aux){
                        auto pct = _->second * 100.0 / sigma;
                        std::vector<std::string> line;
                        line.push_back(_->first);
                        line.push_back(boost::lexical_cast<std::string>(_->second));
                        line.push_back(boost::lexical_cast<std::string>(pct));

                        lines.push_back(line);
                }

                RenderTablePretty(std::cout, lines);


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

