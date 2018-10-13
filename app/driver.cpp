#include <thread>
#include <numeric>
#include <atomic>
#include <boost/format.hpp>
#include "ps/support/config.h"
#include "ps/base/frontend.h"
#include "ps/base/cards.h"
#include "ps/base/tree.h"
#include "ps/base/board_combination_iterator.h"
#include "ps/base/holdem_class_vector.h"
#include "ps/support/index_sequence.h"
#include "ps/support/index_sequence.h"
#include "ps/eval/evaluator_6_card_map.h"
#include "app/pretty_printer.h"
#include "ps/base/algorithm.h"
#include "ps/eval/instruction.h"
#include "ps/base/rank_hasher.h"
#include "ps/base/suit_hasher.h"
#include "ps/base/holdem_board_decl.h"
#include "ps/eval/computer_mask.h"
#include "ps/eval/computer_eval.h"
#include "ps/eval/class_cache.h"
#include <boost/archive/tmpdir.hpp>

#include <boost/timer/timer.hpp>
#include <boost/asio.hpp>

#include <boost/log/trivial.hpp>

#include <Eigen/Dense>
#include <fstream>

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




struct CreateCacheCmd : Command{
        explicit
        CreateCacheCmd(std::vector<std::string> const& args):args_{args}{}
        virtual int Execute()override{
                class_cache cc;
		class_cache_maker m;
	
                std::string cache_name{".cc.bin"};
                try{
                cc.load(cache_name);
                }catch(...){}
                boost::timer::auto_cpu_timer at;
                m.create(2, &cc, cache_name);

                return EXIT_SUCCESS;
        }
private:
        std::vector<std::string> const& args_;
};
static TrivialCommandDecl<CreateCacheCmd> CreateCacheCmdDecl{"create-cache"};

/*
 * 2 player
        
        player ev
        =========
 
        ---+---------------
        f_ | 0
        pf | sb+bb
        pp | 2*S*Ev - (S-sb)

        ---+---------------
        pf | 0
        pp | 2*S*Ev - (S-bb)
        

        total value
        ===========


        ---+---------------
        f_ | -sb
        pf | bb
        pp | 2*S*Ev - S
        
        ---+---------------
        pf | -sb
        pp | 2*S*Ev - S





 */

namespace gt{


        struct gt_context{
                gt_context(double eff, double sb, double bb)
                        :eff_(eff),
                        sb_(sb),
                        bb_(bb)
                {}
                double eff()const{ return eff_; }
                double sb()const{ return sb_; }
                double bb()const{ return bb_; }
                friend std::ostream& operator<<(std::ostream& ostr, gt_context const& self){
                        ostr << "eff_ = " << self.eff_;
                        ostr << ", sb_ = " << self.sb_;
                        ostr << ", bb_ = " << self.bb_;
                        return ostr;
                }
        private:
                double eff_;
                double sb_;
                double bb_;
        };

        // returns a vector each players hand value
        Eigen::VectorXd combination_value(gt_context const& ctx,
                                          class_cache const& cache,
                                          holdem_class_vector const& vec,
                                          Eigen::VectorXd const& s){
                Eigen::VectorXd result{2};
                result.fill(.0);
                Eigen::VectorXd One{2};
                One.fill(1.);

                double f_ = ( 1.0 - s[0] );
                double pf = s[0] * (1. - s[1]);
                double pp = s[0] *       s[1];

                result(0) -= ctx.sb() * f_;
                result(1) += ctx.sb() * f_;
                
                result(0) += ctx.bb() * pf;
                result(1) -= ctx.bb() * pf;

                auto ev = cache.LookupVector(vec);

                result += ( 2 * ev - One ) * ctx.eff() * pp;

                return result;
        }

        Eigen::VectorXd unilateral_detail(gt_context const& ctx,
                                          class_cache const& cache,
                                          size_t idx,
                                          std::vector<Eigen::VectorXd> const& S,
                                          double weighted = true)
        {
                Eigen::VectorXd result(169);
                result.fill(.0);

                for(holdem_class_perm_iterator iter(2),end;iter!=end;++iter){

                        auto const& cv = *iter;
                        #if 0
                        double p = 1.;
                        if( weighted ){
                                p = cv.prob();
                        }
                        #endif
                        auto p = cv.prob();
                        Eigen::VectorXd s(2);
                        s[0] = S[0][cv[0]];
                        s[1] = S[1][cv[1]];
                        result(cv[idx]) += p * combination_value(ctx, cache, *iter, s)[idx];
                }

                return result;
        }
        Eigen::VectorXd choose_push_fold(Eigen::VectorXd const& push, Eigen::VectorXd const& fold){
                Eigen::VectorXd result(169);
                result.fill(.0);
                for(holdem_class_id id=0;id!=169;++id){
                        if( push(id) >= fold(id) ){
                                result(id) = 1.0;
                        }
                }
                return result;
        }
        Eigen::VectorXd clamp(Eigen::VectorXd s){
                for(holdem_class_id idx=0;idx!=169;++idx){
                        s(idx) = ( s(idx) < .5 ? .0 : 1. );
                }
                return s;
        }
        
        static Eigen::VectorXd fold_s = Eigen::VectorXd::Zero(169);
        static Eigen::VectorXd push_s = Eigen::VectorXd::Ones(169);
        Eigen::VectorXd unilateral_sb_maximal_exploitable(gt_context const& ctx,
                                                           class_cache const& cache,
                                                           std::vector<Eigen::VectorXd> const& S)
        {
                auto copy = S;
                copy[0] = fold_s;
                auto fold = unilateral_detail(ctx, cache, 0, copy);
                copy[0] = push_s;
                auto push = unilateral_detail(ctx, cache, 0, copy);
                return choose_push_fold(push, fold);
        }
        Eigen::VectorXd unilateral_bb_maximal_exploitable(gt_context const& ctx,
                                                           class_cache const& cache,
                                                           std::vector<Eigen::VectorXd> const& S)
        {

                auto copy = S;
                copy[1] = fold_s;
                auto fold = unilateral_detail(ctx, cache, 1, copy);
                copy[1] = push_s;
                auto push = unilateral_detail(ctx, cache, 1, copy);
                return choose_push_fold(push, fold);
        }

        std::vector<Eigen::VectorXd>
        unilateral_maximal_explitable_step(gt_context const& ctx,
                                           class_cache const& cache,
                                           std::vector<Eigen::VectorXd> const& state)
        {
                double factor = 0.05;
                auto bb_counter = unilateral_bb_maximal_exploitable(ctx,
                                                                    cache,
                                                                    state);
                auto tmp = state;
                tmp[1] = bb_counter;
                auto sb_counter = unilateral_sb_maximal_exploitable(ctx,
                                                                    cache,
                                                                    tmp);
                auto copy = state;
                copy[0] *= ( 1 - factor );
                copy[0] +=  factor * sb_counter;
                copy[1]  = bb_counter;
                return copy;
        }


        #if NOT_DEFINED
        std::vector<Eigen::VectorXd>
        unilateral_chooser_step(gt_context const& ctx,
                                class_cache const& cache,
                                std::vector<Eigen::VectorXd> const& state)
        {
                auto bb_counter = unilateral_bb_maximal_exploitable(ctx,
                                                                    cache,
                                                                    state);

                auto head = unilateral_detail(ctx, cache, 0, state[0], bb_counter, false);
                auto fold = unilateral_detail(ctx, cache, 0, fold_s, bb_counter, false);
                auto push = unilateral_detail(ctx, cache, 0, push_s, bb_counter, false);

                using Operator = std::tuple<size_t, double, double, double>;

                std::vector<Operator> ops;

                auto add_cand = [&](size_t idx, double cand, double cand_val){
                        auto d = cand - head(idx);
                        if( d > 0.0 ){
                                ops.emplace_back(idx, cand, cand_val, d);
                        }
                };

                for(size_t idx=0;idx!=169;++idx){
                        add_cand(idx, fold(idx), 0.0);
                        add_cand(idx, push(idx), 1.0);
                }

                std::sort(ops.begin(), ops.end(), [](auto const& l, auto const& r){
                        return std::get<3>(l) > std::get<3>(r);
                });

                #if 0
                enum{ CandidateSetSz = 3 };
                if( ops.size() > CandidateSetSz ){
                        ops.resize(CandidateSetSz);
                }
                std::vector<std::tuple<Eigen::VectorXd, double> > candidates;
                for(auto const& t : ops){
                        auto cand_strat = state[0];
                        cand_strat[std::get<0>(t)] = std::get<2>(t);
                        auto counter = unilateral_bb_maximal_exploitable(ctx,
                                                                         cache,
                                                                         cand_strat);
                        candidates.emplace_back(cand_strat, unilateral_detail(ctx, cache, 0, cand_strat, counter).sum());

                        std::cout << "holdem_class_decl::get(std::get<0>(t)) => " << holdem_class_decl::get(std::get<0>(t)) << "\n"; // __CandyPrint__(cxx-print-scalar,holdem_class_decl::get(std::get<0>(t)))
                        std::cout << "std::get<1>(candidates.back()) => " << std::get<1>(candidates.back()) << "\n"; // __CandyPrint__(cxx-print-scalar,std::get<1>(candidates.back()))
                }
                std::sort(candidates.begin(), candidates.end(), [](auto const& r, auto const& l){
                        return std::get<1>(r) < std::get<1>(l);
                });
                
                auto copy = state;
                if( candidates.size()){
                        copy[0] = std::get<0>(candidates.back());
                }
                copy[1] = bb_counter;
                #endif

                auto copy = state;
                ops.resize(ops.size()/2);
                copy[1] = bb_counter;
                for(auto const& t : ops){
                        auto cand_strat = state[0];
                        copy[0][std::get<0>(t)] = std::get<2>(t);
                }
                return copy;

        }
        #endif // NOT_DEFINED


        

        struct solver{
                virtual std::vector<Eigen::VectorXd> step(gt_context const& ctx, class_cache const& cache,
                                                          std::vector<Eigen::VectorXd> const& state)=0;
        };
        struct maximal_exploitable_solver : solver{
                virtual std::vector<Eigen::VectorXd> step(gt_context const& ctx, class_cache const& cache,
                                                          std::vector<Eigen::VectorXd> const& state)override{
                        double factor = 0.05;
                        auto bb_counter = unilateral_bb_maximal_exploitable(ctx,
                                                                            cache,
                                                                            state);
                        auto tmp = state;
                        tmp[1] = bb_counter;
                        auto sb_counter = unilateral_sb_maximal_exploitable(ctx,
                                                                            cache,
                                                                            tmp);
                        auto copy = state;
                        copy[0] *= ( 1 - factor );
                        copy[0] +=  factor * sb_counter;
                        copy[1]  = bb_counter;
                        return copy;
                }
        };

        std::vector<Eigen::VectorXd> solve(gt_context const& ctx, class_cache const& cache)
        {

                std::vector<Eigen::VectorXd> state;
                state.push_back(Eigen::VectorXd::Zero(169));
                state.push_back(Eigen::VectorXd::Zero(169));


                std::shared_ptr<solver> solver_impl( new maximal_exploitable_solver);

                std::set<std::vector<Eigen::VectorXd> > memory; 

                double epsilon = 0.2;

                enum{ MaxIter = 400 };
                for(size_t idx=0;idx<MaxIter;++idx){

                        auto next = solver_impl->step(ctx, cache, state);
                        auto d = next[0] - state[0];
                        auto norm = d.lpNorm<1>();

                        #if 0
                        std::cout << "norm => " << norm << "\n"; // __CandyPrint__(cxx-print-scalar,norm)
                        #endif

                        if( norm < epsilon ){
                                state[0] = clamp(state[0]);
                                state[1] = clamp(state[1]);
                                return state;
                        }

                        #if 0
                        if( memory.count( next ) != 0 ){
                                BOOST_LOG_TRIVIAL(warning) << "loop";
                        }
                        memory.insert(next);
                        #endif

                        state = next;

                        #if 0
                        std::cout << "S_0\n";
                        pretty_print_strat(state[0], 1);
                        std::cout << "S_1\n";
                        pretty_print_strat(state[1], 1);
                        #endif
                }

                std::vector<Eigen::VectorXd> result;
                result.push_back(Eigen::VectorXd::Zero(169));
                result.push_back(Eigen::VectorXd::Zero(169));

                BOOST_LOG_TRIVIAL(warning) << "Failed to converge solve ctx = " << ctx;
                return result;
        }




} // end namespace gt

struct HeadUpSolverCmd : Command{
        explicit
        HeadUpSolverCmd(std::vector<std::string> const& args):args_{args}{}
        virtual int Execute()override{
                class_cache cc;
	
                std::string cache_name{".cc.bin"};
                cc.load(cache_name);


                #if 0
                holdem_class_vector AA_KK{0,1};
                holdem_class_vector KK_AA{1,0};
                std::cout << "AA_KK => " << AA_KK << "\n"; // __CandyPrint__(cxx-print-scalar,AA_KK)
                std::cout << "KK_AA => " << KK_AA << "\n"; // __CandyPrint__(cxx-print-scalar,KK_AA)
                std::cout << "cc.LookupVector(AA_KK) => " << cc.LookupVector(AA_KK) << "\n"; // __CandyPrint__(cxx-print-scalar,cc.LookupVector(AA_KK))
                std::cout << "cc.LookupVector(KK_AA) => " << cc.LookupVector(KK_AA) << "\n"; // __CandyPrint__(cxx-print-scalar,cc.LookupVector(KK_AA))
                #endif

                using namespace gt;


                using result_t = std::future<std::tuple<double, std::vector<Eigen::VectorXd> > >;
                std::vector<result_t> tmp;
                auto enque = [&](double eff){
                        tmp.push_back(std::async([eff,&cc](){
                                gt_context gtctx(eff, .5, 1.);
                                return std::make_tuple(eff, solve(gtctx, cc));
                        }));
                };
                #if 1
                for(double eff = 1.0;eff <= 30.0;eff+=.1){
                        enque(eff);
                }
                #else
                enque(20);
                #endif
                Eigen::VectorXd s0(169);
                s0.fill(.0);
                Eigen::VectorXd s1(169);
                s1.fill(.0);
                for(auto& _ : tmp){
                        auto aux = _.get();
                        auto eff = std::get<0>(aux);
                        auto const& vec = std::get<1>(aux);
                        for(size_t idx=0;idx!=169;++idx){
                                s0(idx) = std::max(s0(idx), eff*vec[0](idx));
                                s1(idx) = std::max(s1(idx), eff*vec[1](idx));
                        }
                }
                
                pretty_print_strat(s0, 1);
                pretty_print_strat(s1, 1);




                return EXIT_SUCCESS;
        }
private:
        std::vector<std::string> const& args_;
};
static TrivialCommandDecl<HeadUpSolverCmd> HeadsUpSolverCmdDecl{"heads-up-solver"};



struct MaskEval : Command{
        explicit
        MaskEval(std::vector<std::string> const& args):args_{args}{}
        virtual int Execute()override{

                std::vector<frontend::range> players;
                for(auto const& s : args_ ){
                        players.push_back( frontend::parse(s) );
                }
                bool debug = false;
                

                auto comp = std::make_shared<mask_computer>();

                computation_context comp_ctx{players.size()};

                computation_pass_manager mgr;
                if( debug )
                        mgr.add_pass<pass_print>();
                mgr.add_pass<pass_permutate>();
                mgr.add_pass<pass_sort_type>();
                mgr.add_pass<pass_collect>();
                if( debug )
                        mgr.add_pass<pass_print>();
                mgr.add_pass<pass_eval_hand_instr>();
                if( debug )
                        mgr.add_pass<pass_print>();

                boost::timer::auto_cpu_timer at;

                instruction_list instr_list = frontend_to_instruction_list(players);
                //auto result = comp->compute(comp_ctx, instr_list);
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













int main(int argc, char** argv){
        try{
                CommandDecl::Driver(argc, argv);
        } catch(std::exception const& e){
                std::cerr << "Caught exception: " << e.what() << "\n";
        }
        return EXIT_SUCCESS;
}

