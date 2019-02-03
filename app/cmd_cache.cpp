
#include "ps/eval/class_cache.h"


#include <fstream>
#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/text_oarchive.hpp>
#include <boost/archive/binary_iarchive.hpp>
#include <boost/archive/binary_oarchive.hpp>
#include <boost/serialization/base_object.hpp>
#include <boost/serialization/utility.hpp>
#include <boost/serialization/map.hpp>
#include <boost/serialization/vector.hpp>
#include <boost/serialization/assume_abstract.hpp>
#include <boost/archive/tmpdir.hpp>
#include <boost/timer/timer.hpp>

#include "ps/support/command.h"

#include "ps/eval/instruction.h"
#include "ps/eval/pass.h"
#include "ps/eval/pass_mask_eval.h"
	
#include <boost/program_options.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>

namespace bpo = boost::program_options;
namespace bpt = boost::property_tree;


namespace ps{
struct CreateCacheCmd : Command{
        explicit
        CreateCacheCmd(std::vector<std::string> const& args):args_{args}{}
        virtual int Execute()override{

                std::string cache_name = ".cc.bin";
                size_t n = 2;
                size_t num_threads = std::thread::hardware_concurrency();
                bool help = false;
                        
                bpo::options_description desc("Scratch command");
                desc.add_options()
                        ("n"          , bpo::value(&n), "number of players")
                        ("help"       , bpo::value(&help)->implicit_value(true), "this message")
                        ("num-threads", bpo::value(&num_threads), "number of threads")
                        ("cache"      , bpo::value(&cache_name), "name of cache to use")
                ;
                        
                std::vector<const char*> aux;
                aux.push_back("dummy");
                for(auto const& _ : args_){
                        aux.push_back(_.c_str());
                }
                aux.push_back(nullptr);

                bpo::variables_map vm;
                bpo::store(parse_command_line(aux.size()-1, &aux[0], desc), vm);
                bpo::notify(vm);    


                if( help ){
                        std::cout << desc << "\n";
                        return EXIT_SUCCESS;
                }

                class_cache cc;

                try{
                        cc.load(cache_name);
                }catch(...){}
                std::cout << "cc.size() => " << cc.size() << "\n"; // __CandyPrint__(cxx-print-scalar,cc.size())
                boost::timer::auto_cpu_timer at;
                class_cache::create(n, &cc, cache_name, num_threads);

                return EXIT_SUCCESS;
        }
private:
        std::vector<std::string> const& args_;
};
static TrivialCommandDecl<CreateCacheCmd> CreateCacheCmdDecl{"create-cache"};

struct PrintCacheCmd : Command{
        explicit
        PrintCacheCmd(std::vector<std::string> const& args):args_{args}{}
        virtual int Execute()override{
                class_cache cc;
        
                std::string cache_name = ".cc.bin";
                bool help = false;
                bool print_json = false;
                size_t max_players = 3;
                        
                bpo::options_description desc("Scratch command");
                desc.add_options()
                        ("help"       , bpo::value(&help)->implicit_value(true), "this message")
                        ("json" , bpo::value(&print_json)->implicit_value(true), "print json file")
                ;
                        
                std::vector<const char*> aux;
                aux.push_back("dummy");
                for(auto const& _ : args_){
                        aux.push_back(_.c_str());
                }
                aux.push_back(nullptr);

                bpo::variables_map vm;
                bpo::store(parse_command_line(aux.size()-1, &aux[0], desc), vm);
                bpo::notify(vm);    



                try{
                        cc.load(cache_name);

                        if( print_json ){
                                std::string indent{"    "};
                                std::cout << "{\n";
                                bool sep = false;
                                for(auto iter(cc.begin()),end(cc.end());iter!=end;++iter){
                                        holdem_class_vector aux(iter->first.begin(), iter->first.end());
                                        if( aux.size() > max_players )
                                                continue;
                                        std::string vs = detail::to_string(iter->second,
                                                                           detail::lexical_caster{},
                                                                           detail::pretty_array_traits{});
                                        std::cout << indent
                                                  << ( sep ? ", " : "" )
                                                  << "\"" << aux << "\" : " << vs << "\n";
                                        sep = true;
                                }
                                std::cout << "}\n";
                        } else {
                                for(auto iter(cc.begin()),end(cc.end());iter!=end;++iter){
                                        holdem_class_vector aux(iter->first.begin(), iter->first.end());
                                        std::cout << aux << " -> " << detail::to_string(iter->second) << "\n";
                                }
                        }

                }catch(std::exception const& e){
                        std::cerr << "Failed to read: " << e.what() << "\n";
                }

                return EXIT_SUCCESS;
        }
private:
        std::vector<std::string> const& args_;
};
static TrivialCommandDecl<PrintCacheCmd> PrintCacheCmdDecl{"print-cache"};

struct ReadCacheCmd : Command{
        explicit
        ReadCacheCmd(std::vector<std::string> const& args):args_{args}{}
        virtual int Execute()override{
                class_cache cc;
        
                std::string cache_name = ".cc.bin.stage";
                bool help = false;
                std::string file_name = "Assets/PushFoldEquity.json";
                        
                bpo::options_description desc("Scratch command");
                desc.add_options()
                        ("help"       , bpo::value(&help)->implicit_value(true), "this message")
                        ("file-name" , bpo::value(&file_name), "input file name")
                ;
                        
                std::vector<const char*> aux;
                aux.push_back("dummy");
                for(auto const& _ : args_){
                        aux.push_back(_.c_str());
                }
                aux.push_back(nullptr);

                bpo::variables_map vm;
                bpo::store(parse_command_line(aux.size()-1, &aux[0], desc), vm);
                bpo::notify(vm);    


                try{
                        cc.load(cache_name);
                }catch(...){}

                try{

                        bpt::ptree root;
                        bpt::read_json(file_name, root);
                        for(auto const& p : root ){
                                std::string cv_s = p.first.data();
                                
                                assert( cv_s.size() > 3 );

                                std::string tmp = cv_s.substr(1,cv_s.size()-3);

                                holdem_class_vector cv;

                                static std::regex words_regex("[^ ,{}]+");
                                auto words_begin = std::sregex_iterator(cv_s.begin(), cv_s.end(), words_regex);
                                for(std::sregex_iterator iter=words_begin, end;iter!=end;++iter){
                                        cv.push_back(iter->str());
                                }


                                std::vector<double> equity_vec;
                                for(auto const& e : p.second ){
                                        equity_vec.push_back(e.second.get_value<double>());
                                }

                                cc.add(cv, equity_vec);
                        }
                        cc.save(cache_name);

                }catch(std::exception const& e){
                        std::cerr << "Failed to read: " << e.what() << "\n";
                }

                return EXIT_SUCCESS;
        }
private:
        std::vector<std::string> const& args_;
};
static TrivialCommandDecl<ReadCacheCmd> ReadCacheCmdDecl{"read-cache"};


} // end namespace ps
