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
#include <boost/archive/tmpdir.hpp>

#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/text_oarchive.hpp>

#include <boost/serialization/base_object.hpp>
#include <boost/serialization/utility.hpp>
#include <boost/serialization/list.hpp>
#include <boost/serialization/map.hpp>
#include <boost/serialization/vector.hpp>
#include <boost/serialization/assume_abstract.hpp>

#include <boost/timer/timer.hpp>
#include <boost/asio.hpp>

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



struct equity_view : std::vector<double>{
        equity_view(matrix_t const& breakdown){
                sigma_ = 0;
                size_t n = breakdown.rows();
                std::map<long, unsigned long long> sigma_device;
                for( size_t i=0;i!=n;++i){
                        for(size_t j=0; j != n; ++j ){
                                sigma_device[j] += breakdown(j,i);
                        }
                }
                for( size_t i=0;i!=n;++i){
                        sigma_ += sigma_device[i] / ( i +1 );
                }


                for( size_t i=0;i!=n;++i){

                        double equity = 0.0;
                        for(size_t j=0; j != n; ++j ){
                                equity += breakdown(j,i) / ( j +1 );
                        }
                        equity /= sigma_;
                        push_back(equity);
                }
        }
        unsigned long long sigma()const{ return sigma_; }
private:
        unsigned long long sigma_;
};

struct class_cache{
        void add(std::vector<holdem_class_id> vec, std::vector<double> equity){
                cache_.emplace(std::move(vec), std::move(equity));
        }
	std::vector<double> const* Lookup(std::vector<holdem_class_id> const& vec)const{
                auto iter = cache_.find(vec);
                if( iter == cache_.end())
                        return nullptr;
                return &iter->second;
	}
        Eigen::VectorXd LookupVector(holdem_class_vector const& vec)const{
                #if 0
                if( std::is_sorted(vec.begin(), vec.end()) ){
                        Eigen::VectorXd tmp(vec.size());
                        auto ptr = Lookup(vec);
                        BOOST_ASSERT(pyt);
                        for(size_t idx=0;idx!=vec.size();++idx){
                                tmp(idx) = vec[idx];
                        }
                        return tmp;
                }
                #endif
                // find the permuation index
                std::array<
                        std::tuple<size_t, holdem_class_id>
                        , 9
                > aux;
                for(size_t idx=0;idx!=vec.size();++idx){
                        aux[idx] = std::make_tuple(idx, vec[idx]);
                }
                std::sort(aux.begin(), aux.begin() + vec.size(), [](auto const& l, auto const& r){
                          return std::get<1>(l) < std::get<1>(r);
                });

                // I think this is quicker than copying from aux
                auto copy = vec;
                std::sort(copy.begin(), copy.end());
                        
                // find the underlying
                auto ptr = Lookup(copy);
                BOOST_ASSERT(ptr);

                // copy to a vector
                Eigen::VectorXd tmp(vec.size());
                for(size_t idx=0;idx!=vec.size();++idx){
                        auto mapped_idx = std::get<0>(aux[idx]);
                        tmp(mapped_idx) = (*ptr)[idx];
                }
                return tmp;
        }
	void save(std::string const& filename){
		// make an archive
		std::ofstream ofs(filename);
		boost::archive::text_oarchive oa(ofs);
		oa << *this;
	}

	void load(std::string const& filename)
	{
		// open the archive
		std::ifstream ifs(filename);
		boost::archive::text_iarchive ia(ifs);

		// restore the schedule from the archive
		cache_.clear();
		ia >> *this;
        }
	
private:
        friend class boost::serialization::access;
        template<class Archive>
        void serialize(Archive & ar, const unsigned int version){
                ar & cache_;
        }
private:
        std::map<std::vector<holdem_class_id>, std::vector<double> > cache_;
};

struct class_cache_maker{
        void create(size_t n, class_cache* cache, std::string const& file_name){
                computation_pass_manager mgr;
                mgr.add_pass<pass_class2cards>();
                mgr.add_pass<pass_permutate>();
                mgr.add_pass<pass_sort_type>();
                mgr.add_pass<pass_collect>();
                mgr.add_pass<pass_eval_hand_instr>();

                size_t count = 0;
                enum{ MaxCount = 50 };
                auto save = [&](){
                        std::cout << "Saving...\n";
                        cache->save(file_name);
                        std::cout << "Done\n";
                };
                for(holdem_class_iterator iter(n),end;iter!=end;++iter){
                        auto vec = *iter;
                        BOOST_ASSERT( vec.is_standard_form() );
                        if( cache->Lookup(vec) )
                                continue;
                        instruction_list instr_list;
                        instr_list.push_back(std::make_shared<class_vec_instruction>(vec));
                        computation_context comp_ctx{n};
                        auto result = mgr.execute(&comp_ctx, &instr_list);
                        BOOST_ASSERT( result );
                        equity_view view( *result );
                        enum{ Debug = true };
                        if( Debug ){
                                #if 0
                                std::vector<std::string> s;
                                for(auto _ : vec){
                                        s.push_back(holdem_class_decl::get(_).to_string());
                                }
                                pretty_print_equity_breakdown_mat(std::cout, *result, s);
                                #endif
                                std::cout << vec << " -> " << detail::to_string(view) << "\n";
                        }


			cache->add(vec, view);
                        if( ++count == MaxCount ){
                                count = 0;
                                save();
                        }
                }
                save();
        }
};

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
        private:
                double eff_;
                double sb_;
                double bb_;
        };

        Eigen::VectorXd combination_value(gt_context const& ctx,
                                          class_cache const& cache,
                                          holdem_class_vector const& vec,
                                          double s0,
                                          double s1){
                Eigen::VectorXd result{2};
                result.fill(.0);
                Eigen::VectorXd One{2};
                One.fill(1.);

                double f_ = ( 1.0 - s0 );
                double pf = s0 * (1. - s1);
                double pp = s0 *       s1;

                result(0) -= ctx.sb() * f_;
                result(1) += ctx.sb() * f_;
                
                result(0) += ctx.bb() * pf;
                result(1) -= ctx.bb() * pf;

                auto ev = cache.LookupVector(vec);

                result += ( 2 * ev - One ) * ctx.eff() * pp;

                return result;
        }

        Eigen::VectorXd unilateral_sb_maximal_exploitable(gt_context const& ctx,
                                                          class_cache const& cache,
                                                          Eigen::VectorXd const& bb_strat)
        {
                Eigen::VectorXd push(169);
                push.fill(0.);
                Eigen::VectorXd fold(169);
                fold.fill(0.);


                for(holdem_class_perm_iterator iter(2),end;iter!=end;++iter){
                        double p = (*iter).prob();

                        auto const& cv = *iter;

                        fold(cv[0]) += p * combination_value(ctx, cache, *iter, 0.0, bb_strat[cv[1]])[0];
                        push(cv[0]) += p * combination_value(ctx, cache, *iter, 1.0, bb_strat[cv[1]])[0];
                }

                Eigen::VectorXd result(169);
                result.fill(.0);
                for(holdem_class_id id=0;id!=169;++id){
                        if( push(id) >= fold(id) ){
                                result(id) = 1.0;
                        }
                }

                return result;
        }
        Eigen::VectorXd unilateral_bb_maximal_exploitable(gt_context const& ctx,
                                                          class_cache const& cache,
                                                          Eigen::VectorXd const& sb_strat)
        {
                Eigen::VectorXd push(169);
                push.fill(0.);
                Eigen::VectorXd fold(169);
                fold.fill(0.);


                for(holdem_class_perm_iterator iter(2),end;iter!=end;++iter){
                        double p = (*iter).prob();

                        auto const& cv = *iter;

                        fold(cv[1]) += p * combination_value(ctx, cache, *iter, sb_strat[cv[0]], 0.0)[1];
                        push(cv[1]) += p * combination_value(ctx, cache, *iter, sb_strat[cv[0]], 1.0)[1];
                }

                Eigen::VectorXd result(169);
                result.fill(.0);
                for(holdem_class_id id=0;id!=169;++id){
                        if( push(id) >= fold(id) ){
                                result(id) = 1.0;
                        }
                }

                return result;
        }
        
        // print pretty table
        //
        //      AA  AKs ... A2s
        //      AKo KK
        //      ...     ...
        //      A2o         22
        //
        //
        void display(Eigen::VectorXd const& vec){
                /*
                        token_buffer[0][0] token_buffer[1][0]
                        token_buffer[0][1]

                        token_buffer[y][x]


                 */
                std::array<
                        std::array<std::string, 13>, // x
                        13                           // y
                > token_buffer;
                std::array<size_t, 13> widths;

                for(size_t i{0};i!=169;++i){
                        auto const& decl =  holdem_class_decl::get(i) ;
                        size_t x{decl.first().id()};
                        size_t y{decl.second().id()};
                        // inverse
                        x = 12 - x;
                        y = 12 - y;
                        if( decl.category() == holdem_class_type::offsuit ){
                                std::swap(x,y);
                        }

                        #if 1
                        //token_buffer[y][x] = boost::lexical_cast<std::string>(vec_[i]);
                        if( vec(i) == 1.0 ){
                                token_buffer[y][x] = "1";
                        } else if( vec(i) == 0.0 ){
                                token_buffer[y][x] = "0";
                        } else {
                                token_buffer[y][x] = str(boost::format("%.4f") % vec(i));
                        }

                        #else
                        token_buffer[y][x] = boost::lexical_cast<std::string>(decl.to_string());
                        #endif
                }
                for(size_t i{0};i!=13;++i){
                        widths[i] = std::max_element( token_buffer[i].begin(),
                                                      token_buffer[i].end(),
                                                      [](auto const& l, auto const& r){
                                                              return l.size() < r.size(); 
                                                      })->size();
                }

                auto pad= [](auto const& s, size_t w){
                        size_t padding{ w - s.size()};
                        size_t left_pad{padding/2};
                        size_t right_pad{padding - left_pad};
                        std::string ret;
                        if(left_pad)
                               ret += std::string(left_pad,' ');
                        ret += s;
                        if(right_pad)
                               ret += std::string(right_pad,' ');
                        return std::move(ret);
                };
                
                std::cout << "   ";
                for(size_t i{0};i!=13;++i){
                        std::cout << pad( rank_decl::get(12-i).to_string(), widths[i] ) << " ";
                }
                std::cout << "\n";
                std::cout << "  +" << std::string( std::accumulate(widths.begin(), widths.end(), 0) + 13, '-') << "\n";

                for(size_t i{0};i!=13;++i){
                        std::cout << rank_decl::get(12-i).to_string() << " |";
                        for(size_t j{0};j!=13;++j){
                                if( j != 0){
                                        std::cout << " ";
                                }
                                std::cout << pad(token_buffer[j][i], widths[j]);
                        }
                        std::cout << "\n";
                }
        }

        std::vector<Eigen::VectorXd> solve(gt_context const& ctx, class_cache const& cache)
        {
                Eigen::VectorXd s0(169);
                s0.fill(1.0);

                double factor = 0.05;

                enum{ MaxIter = 100 };
                for(size_t idx=0;idx<MaxIter;++idx){

                        auto bb_counter = unilateral_bb_maximal_exploitable(ctx,
                                                                            cache,
                                                                            s0);
                        auto sb_counter = unilateral_sb_maximal_exploitable(ctx,
                                                                            cache,
                                                                            bb_counter);

                        auto d = ( s0 - sb_counter );
                        auto norm = d.lpNorm<1>();

                        #if 0
                        display(sb_counter);
                        display(bb_counter);
                        #endif
                        std::cout << "norm => " << norm << "\n"; // __CandyPrint__(cxx-print-scalar,norm)

                        if( norm < 1. ){
                                std::vector<Eigen::VectorXd> result;
                                result.push_back(sb_counter);
                                result.push_back(bb_counter);
                                return result;
                        }

                        s0 *= ( 1- factor );
                        s0 += factor * sb_counter;
                }

                Eigen::VectorXd proto(169);
                proto.fill(.0);
                std::vector<Eigen::VectorXd> result;
                result.push_back(proto);
                result.push_back(proto);
                return result;
        }


        using any_value_t = boost::variant<
                double
        >;

        struct Context{
        };

        struct Node{
        };

        struct Placeholder : Node{
        };

        struct NonTerminal : Node, std::vector<std::shared_ptr<Node> >{
        };

        struct Function : NonTerminal{
        };

        struct BinaryOp : NonTerminal{
        };


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
                for(double eff = 10.0;eff <= 20.0;eff+=5.0){
                        tmp.push_back(std::async([eff,&cc](){
                                gt_context gtctx(eff, .5, 1.);
                                return std::make_tuple(eff, solve(gtctx, cc));
                        }));
                }
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
                
                display(s0);
                display(s1);




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
