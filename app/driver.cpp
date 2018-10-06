#include <thread>
#include <atomic>
#include <boost/format.hpp>
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




struct instruction{
        enum type{
                T_CardEval,
                T_ClassEval,
        };
        explicit instruction(type t):type_{t}{}
        type get_type()const{ return type_; }

        virtual std::string to_string()const=0;
private:
        type type_;
};


/*
 * we want to print a 2x2 matrix as
 *    [[m(0,0), m(1,0)],[m(0,1),m(1,1)]]
 */
inline std::string matrix_to_string(Eigen::MatrixXd const& mat){
        std::stringstream sstr;
        std::string sep;
        sstr << "[";
        for(size_t j=0;j!=mat.rows();++j){
                sstr << sep << "[";
                sep = ",";
                for(size_t i=0;i!=mat.cols();++i){
                        sstr << (i!=0?",":"") << mat(i,j);
                }
                sstr << "]";
        }
        sstr << "]";
        return sstr.str();
}

template<class VectorType, instruction::type Type>
struct basic_eval_instruction : instruction{
        using vector_type = VectorType;
        basic_eval_instruction(vector_type const& vec)
                : instruction{Type}
                , vec_{vec}
                , matrix_{Eigen::MatrixXd::Identity(vec.size(), vec.size())}
        {
        }
        holdem_hand_vector get_vector()const{
                return vec_;
        }
        void set_vector(holdem_hand_vector const& vec){
                vec_ = vec;
        }
        Eigen::MatrixXd const& get_matrix()const{
                return matrix_;
        }
        void set_matrix(Eigen::MatrixXd const& matrix){
                matrix_ = matrix;
        }
        virtual std::string to_string()const override{
                std::stringstream sstr;
                sstr << (Type == T_CardEval ? "CardEval" : "ClassEval" ) << "{" << vec_ << ", " << matrix_to_string(matrix_) << "}";
                return sstr.str();
        }
        
        friend std::ostream& operator<<(std::ostream& ostr, basic_eval_instruction const& self){
                return ostr << self.to_string();
        }
private:
        vector_type vec_;
        Eigen::MatrixXd matrix_;
};

using card_eval_instruction  = basic_eval_instruction<holdem_hand_vector, instruction::T_CardEval>;
using class_eval_instruction = basic_eval_instruction<holdem_class_vector, instruction::T_ClassEval>;


using instruction_list = std::list<std::shared_ptr<instruction> >;

void transform_print(instruction_list& instr_list){
        for(auto instr : instr_list ){
                std::cout << instr->to_string() << "\n";
        }
}
void transform_permutate(instruction_list& instr_list){
        for(auto instr : instr_list){
                if( instr->get_type() != instruction::T_CardEval )
                        continue;
                auto ptr = reinterpret_cast<card_eval_instruction*>(instr.get());
                auto const& vec = ptr->get_vector();
                auto const& matrix = ptr->get_matrix();
                auto result = permutate_for_the_better(vec);

                if( std::get<1>(result) == vec )
                        continue;

                Eigen::MatrixXd perm_matrix(vec.size(), vec.size());
                perm_matrix.fill(.0);
                auto const& perm = std::get<0>(result);
                for(size_t idx=0;idx!=perm.size();++idx){
                        perm_matrix(idx, perm[idx]) = 1.0;
                }

                ptr->set_vector(std::get<1>(result));
                ptr->set_matrix( matrix * perm_matrix );
        }
}


void transform_sort_type(instruction_list& instr_list){
        instr_list.sort( [](auto l, auto r){
                if( l->get_type() != r->get_type())
                        return l->get_type() != r->get_type();
                switch(l->get_type()){
                case instruction::T_CardEval:
                        {
                                auto lt = reinterpret_cast<card_eval_instruction*>(l.get());
                                auto rt = reinterpret_cast<card_eval_instruction*>(r.get());
                                return lt->get_vector() < rt->get_vector();
                        }
                        break;
                }
                return false;
        });
}
void transform_collect(instruction_list& instr_list){
        using iter_type = decltype(instr_list.begin());

        std::vector<iter_type> subset;

        for(iter_type iter(instr_list.begin()),end(instr_list.end());iter!=end;++iter){
                if( (*iter)->get_type() == instruction::T_CardEval )
                        subset.push_back(iter);
        }


        for(; subset.size() >= 2 ;){
                auto a = reinterpret_cast<card_eval_instruction*>(&**subset[subset.size()-1]);
                auto b = reinterpret_cast<card_eval_instruction*>(&**subset[subset.size()-2]);

                if( a->get_vector() == b->get_vector() ){
                        b->set_matrix( a->get_matrix() + b->get_matrix() );
                        instr_list.erase(subset.back());
                        subset.pop_back();
                }  else{
                        subset.pop_back();
                }
        }
}


        






struct SimpleCardEval : Command{
        explicit
        SimpleCardEval(std::vector<std::string> const& args):args_{args}{}
        virtual int Execute()override{
                equity_evaulator_principal eval;
                class_equity_evaluator_principal class_eval;

                std::vector<frontend::range> players;
                for(auto const& s : args_ ){
                        players.push_back( frontend::parse(s) );
                }
                tree_range root( players );
        

                std::list<std::shared_ptr<instruction> > instr_list;

                for( auto const& c : root.children ){

                        #if 0
                        // this means it's a class vs class evaulation
                        if( c.opt_cplayers.size() != 0 ){
                                holdem_class_vector aux{c.opt_cplayers};
                                agg.append(*class_eval.evaluate(aux));
                        } else
                        #endif
                        {
                                for( auto const& d : c.children ){
                                        holdem_hand_vector aux{d.players};
                                        //agg.append(*eval.evaluate(aux));

                                        instr_list.push_back(std::make_shared<card_eval_instruction>(aux));
                                }
                        }
                }
                #endif
                transform_permutate(instr_list);
                transform_sort_type(instr_list);
                transform_collect(instr_list);
                transform_print(instr_list);


                equity_breakdown_matrix_aggregator agg(players.size());
                for(auto instr : instr_list ){
                        auto ptr = reinterpret_cast<card_eval_instruction*>(instr.get());

                        auto result = eval.evaluate(ptr->get_vector());
                        agg.append_matrix(*result, ptr->get_matrix());
                }

                std::cout << agg << "\n";

                pretty_print_equity_breakdown(std::cout, agg, args_);



                return EXIT_SUCCESS;
        }
private:
        std::vector<std::string> const& args_;
};
static TrivialCommandDecl<SimpleCardEval> SimpleCardEvalDecl{"simple-card-eval"};



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


#if 0
struct hand_tree{
        enum type{
                T_Class,
                T_Hand,
        };
        virtual type get_type()const=0;
        virtual ~hand_tree()=default;
        inline
        static std::shared_ptr<hand_tree> create(std::string const& str);

        virtual holdem_class_vector get_class_vector()const{ 
                BOOST_THROW_EXCEPTION(std::domain_error("not implemented"));
        }
        virtual holdem_hand_vector get_hand_vector()const{ 
                BOOST_THROW_EXCEPTION(std::domain_error("not implemented"));
        }

};
struct hand_tree_class : hand_tree{
        explicit hand_tree_class(holdem_class_vector const& vec):vec_{vec}{}
        virtual type get_type()const override{ return T_Class; }
        
        virtual holdem_class_vector get_class_vector()const{ 
                return vec_;
        }
        virtual holdem_hand_vector get_hand_vector()const{ 
        }
private:
        holdem_class_vector vec_;
};

std::shared_ptr<hand_tree> hand_tree::create(std::string const& str){

        auto rng = frontend::parse(s);
        auto expanded = expand(rng);


        try{
                auto cv = expanded.to_class_vector();
                return std::make_shared<hand_tree_class>(cv);
        }catch(...){
                auto hv = expanded.to_holdem_vector();
                return std::make_shared<hand_tree_class>(cv);
        }
}

struct hand_tree_range 
#endif


struct Scratch : Command{
        explicit
        Scratch(std::vector<std::string> const& args):players_s_{args}{}
        virtual int Execute()override{

                holdem_class_vector cv;
                cv.push_back("AA");
                cv.push_back("KK");
                cv.push_back("TT");
                auto sf = cv.to_standard_form_hands();
        

                std::cout << "to_standard_form_hands\n";
                for( auto hvt : cv.to_standard_form_hands()){
                        std::cout << "  " << hvt << "\n";
                }
                std::cout << "get_hand_vectors\n";
                for( auto hv : cv.get_hand_vectors()){
                        std::cout << "  " << hv << "\n";
                }

                return EXIT_SUCCESS;
        }
private:
        std::vector<std::string> const& players_s_;
};
static TrivialCommandDecl<Scratch> ScratchDecl{"scratch"};












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
