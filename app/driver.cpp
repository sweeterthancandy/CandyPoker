#include <thread>
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
	std::vector<double>* Lookup(std::vector<holdem_class_id> const& vec){
                auto iter = cache_.find(vec);
                if( iter == cache_.end())
                        return nullptr;
                return &iter->second;
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
        struct node{
        };
        struct choice : node{
        };
} // end namespace gt









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
