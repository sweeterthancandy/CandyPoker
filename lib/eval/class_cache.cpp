/*

CandyPoker
https://github.com/sweeterthancandy/CandyPoker

MIT License

Copyright (c) 2019 Gerry Candy

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.

*/
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
#include "ps/eval/pass_eval_hand_instr_vec.h"
	
#include <boost/program_options.hpp>
namespace bpo = boost::program_options;

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
void class_cache::create(size_t n, class_cache* cache, std::string const& file_name, size_t num_threads){
        computation_pass_manager mgr;
        mgr.add_pass<pass_class2cards>();
        mgr.add_pass<pass_permutate>();
        mgr.add_pass<pass_sort_type>();
        mgr.add_pass<pass_collect>();
        mgr.add_pass<pass_eval_hand_instr_vec>();
        mgr.add_pass<pass_write>();

        size_t count = 0;
        enum{ MaxCount = 50 };
        enum{ BatchSize = 169  };

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
                                
        #if 0
        instruction_list instr_list;
        instr_list.push_back(std::make_shared<class_vec_instruction>("A", cv));
        auto result = mgr.execute_old(&comp_ctx, &instr_list);
        BOOST_ASSERT( result );
        batch_ret.push_back(result);
        #endif

        auto driver = [&](){
                for(;;){
                        auto batch = pull();
                        if( batch.empty() )
                                return;
                        std::vector<boost::optional<matrix_t> > batch_ret;
                        size_t token = 0;
                        std::vector<std::string> tags;
                        computation_result result{comp_ctx};
                        instruction_list instr_list;
                        for(auto const& cv : batch ){

                                auto tag = boost::lexical_cast<std::string>(token++);
                                tags.push_back(tag);
                                instr_list.push_back(std::make_shared<class_vec_instruction>(tag, cv));
                        }
                        mgr.execute_(&comp_ctx, &instr_list, &result);
                        BOOST_ASSERT( result );
                        for(auto const& _ : tags ){
                                batch_ret.push_back(result[_]);
                        }

                        push(batch, batch_ret);
                }
        };
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
