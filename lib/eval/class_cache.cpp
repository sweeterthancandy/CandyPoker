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
	
namespace ps{



void class_cache::save(std::string const& filename){
        using archive_type = boost::archive::text_oarchive;
        std::ofstream ofs(filename);
        archive_type oa(ofs);
        oa << *this;
}

void class_cache::load(std::string const& filename)
{
        using archive_type = boost::archive::text_iarchive;
        std::ifstream ifs(filename);
        archive_type ia(ifs);
        cache_.clear();
        ia >> *this;
}
void class_cache::create(size_t n, class_cache* cache, std::string const& file_name){
        computation_pass_manager mgr;
        mgr.add_pass<pass_class2cards>();
        mgr.add_pass<pass_permutate>();
        mgr.add_pass<pass_sort_type>();
        mgr.add_pass<pass_collect>();
        //mgr.add_pass<pass_eval_hand_instr>();
        mgr.add_pass<pass_eval_hand_instr_vec>();

        size_t count = 0;
        enum{ MaxCount = 50 };
        enum{ BatchSize = 50 };

        auto save_impl = [&](){
                std::cout << "Saving...\n";
                cache->save(file_name);
                cache->save(file_name + ".other");
                std::cout << "Done\n";
        };

        holdem_class_iterator iter(n),end;
        std::mutex mtx;
        auto pull = [&]()->std::vector<holdem_class_vector>{
                std::lock_guard<std::mutex> lock(mtx);
                std::vector<holdem_class_vector> batch;
                for(;batch.size()!=BatchSize && iter != end;++iter){
                        holdem_class_vector vec = *iter;
                        BOOST_ASSERT( iter->is_standard_form() );
                        if( cache->Lookup(vec) )
                                continue;
                        batch.push_back(vec);
                }
                return batch;
        };

        auto push = [&](std::vector<holdem_class_vector> const& cvv,
                        std::vector<boost::optional<matrix_t> > const& matv)
        {
                BOOST_ASSERT(cvv.size() == matv.size());
                std::lock_guard<std::mutex> lock(mtx);
                size_t count = 0;

                for(size_t idx=0;idx!=cvv.size();++idx){
                        equity_view view( matv[idx].get() );
                        if( ! view.valid() )
                                continue;
                        cache->add(cvv[idx], view);
                        ++count;
                        enum{ Debug = true };
                        if( Debug ){
                                #if 0
                                std::vector<std::string> s;
                                for(auto _ : vec){
                                        s.push_back(holdem_class_decl::get(_).to_string());
                                }
                                pretty_print_equity_breakdown_mat(std::cout, *result, s);
                                #endif
                                std::cout << cvv[idx] << " -> " << detail::to_string(view) << "\n";
                        }
                }
                if( count ){
                        save_impl();
                }
        };
                
        computation_context comp_ctx{n};

        auto driver = [&](){
                for(;;){
                        auto batch = pull();
                        std::vector<boost::optional<matrix_t> > batch_ret;
                        if( batch.empty() )
                                return;
                        for(auto const& cv : batch ){
                                instruction_list instr_list;
                                instr_list.push_back(std::make_shared<class_vec_instruction>("A", cv));
                                auto result = mgr.execute_old(&comp_ctx, &instr_list);
                                BOOST_ASSERT( result );
                                batch_ret.push_back(result);
                        }


                        push(batch, batch_ret);
                }
        };
        size_t num_threads = 8;
        std::vector<std::thread> tg;
        for(size_t idx=0;idx!=num_threads;++idx){
                tg.emplace_back(driver);
        }
        for(auto & _ : tg){
                _.join();
        }

        #if 0
        for(,end;iter!=end;++iter){
                holdem_class_vector vec = *iter;
                BOOST_ASSERT( iter->is_standard_form() );
                if( cache->Lookup(vec) )
                        continue;
                instruction_list instr_list;
                instr_list.push_back(std::make_shared<class_vec_instruction>(vec));
                computation_context comp_ctx{n};
                boost::timer::cpu_timer timer;
                auto result = mgr.execute(&comp_ctx, &instr_list);
                BOOST_ASSERT( result );
                equity_view view( *result );
                if( ! view.valid() )
                        continue;
                std::cout << timer.format();
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
                        save_impl();
                }
        }
        save_impl();
        #endif
}


struct CreateCacheCmd : Command{
        explicit
        CreateCacheCmd(std::vector<std::string> const& args):args_{args}{}
        virtual int Execute()override{
                class_cache cc;

                std::string cache_name = ".cc.bin";
                size_t n = 2;

                int arg_ptr = 0;
                for(;arg_ptr < args_.size();){
                        switch(args_.size()-arg_ptr){
                        default:
                        case 2:
                                if( args_[arg_ptr] == "--file" ){
                                        cache_name = args_[arg_ptr+1];
                                        arg_ptr += 2;
                                        continue;
                                }
                                if( args_[arg_ptr] == "--n" ){
                                        n = boost::lexical_cast<size_t>(args_[arg_ptr+1]);
                                        if( ! ( 2 <= n && n <= 9 ) ){
                                                std::cerr << "bad number of players\n";
                                                return EXIT_FAILURE;
                                        }
                                        arg_ptr += 2;
                                        continue;
                                }
                        case 1:
                                std::cerr << "unknown option " << args_[arg_ptr] << "\n";
                                return EXIT_FAILURE;
                        }
                }

                try{
                        cc.load(cache_name);
                }catch(...){}
                std::cout << "cc.size() => " << cc.size() << "\n"; // __CandyPrint__(cxx-print-scalar,cc.size())
                boost::timer::auto_cpu_timer at;
                class_cache::create(n, &cc, cache_name);

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
        
                std::string cache_name{".cc.bin"};
                if( args_.size() >= 1 ){
                        cache_name = args_.at(0);
                }

                try{
                        cc.load(cache_name);
                        for(auto iter(cc.begin()),end(cc.end());iter!=end;++iter){
                                holdem_class_vector aux(iter->first.begin(), iter->first.end());
                                std::cout << aux << " -> " << detail::to_string(iter->second) << "\n";
                        }

                }catch(...){}

                return EXIT_SUCCESS;
        }
private:
        std::vector<std::string> const& args_;
};
static TrivialCommandDecl<PrintCacheCmd> PrintCacheCmdDecl{"print-cache"};
} // end namespace ps


#include "ps/support/persistent_impl.h"

namespace{
        using namespace ps;
        struct class_cache_impl : support::persistent_memory_impl_serializer<class_cache>{
                virtual std::string name()const override{
                        return "class_cache";
                }
                virtual std::shared_ptr<class_cache> make()const override{
                        // boot strap
                        std::string cache_name{".cc.bin"};
                        auto ptr = std::make_shared<class_cache>();
                        ptr->load(cache_name);
                        return ptr;
                }
                virtual void display(std::ostream& ostr)const override{
                        auto const& obj = *reinterpret_cast<class_cache const*>(ptr());
                        for(auto const& _ : obj){
                                holdem_class_vector cv(_.first.begin(), _.first.end());
                                std::cout << cv << "=>" << detail::to_string(_.second) << "\n";
                        }
                }
        private:
                size_t N_;
        };
} // end namespace anon

namespace ps{

support::persistent_memory_decl<class_cache> Memory_ClassCache{std::make_unique<class_cache_impl>()};

} // end namespace ps
