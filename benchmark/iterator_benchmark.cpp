#include <benchmark/benchmark.h>

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
#include "ps/eval/class_cache.h"
#include "ps/eval/pass.h"
#include "ps/eval/pass_eval_hand_instr_vec.h"
#include "ps/base/rank_board_combination_iterator.h"

#include <boost/timer/timer.hpp>

#include <boost/log/trivial.hpp>

#include <Eigen/Dense>
#include <fstream>

#include "ps/support/command.h"
#include "ps/support/persistent.h"
#include "ps/eval/holdem_class_vector_cache.h"

using namespace ps;
using namespace ps::frontend;

/*
The point of this is to save the compution of mapping AA vs KK vs QQ to hard vectors,
then can re-apply the result to any matching pattern
*/
struct holdem_class_hard_from_proto
{
        using match_ty = std::tuple<holdem_class_type, rank_id, rank_id>;
        using rank_mapping_ty = std::array<int, rank_decl::max_id>;

        static std::string pattern_to_string(std::vector<match_ty> const& pattern)
        {
                std::stringstream ss;
                ss << "<";
                std::string sep = "";
                for(auto const& m : pattern)
                {
                        switch (std::get<0>(m))
                        {
                        case holdem_class_type::pocket_pair:
                                ss << "(pp," << rank_decl::get(std::get<1>(m)).to_string() << ")" << sep;
                                break;
                        case holdem_class_type::suited:
                                ss << "(suited," << rank_decl::get(std::get<1>(m)).to_string() 
                                        << "," << rank_decl::get(std::get<2>(m)).to_string() << ")" << sep;
                                break;
                        case holdem_class_type::offsuit:
                                ss << "(offsuit," << rank_decl::get(std::get<1>(m)).to_string() 
                                        << "," << rank_decl::get(std::get<2>(m)).to_string() << ")" << sep;
                        }
                        sep = ",";
                }
                ss << ">";
                return ss.str();
        }

        struct prototype
        {
                prototype(
                        std::vector< holdem_hand_vector > const& hv,
                        std::vector< matrix_t > const& transform)
                        : hv_(hv), transform_(transform)
                {}
                void emit(
                        holdem_class_vector const& cv,
                        rank_mapping_ty const& mapping,
                        std::vector< holdem_hand_vector >& out,
                        std::vector<matrix_t>& transform)const
                {
                        holdem_hand_vector working_hv;
                        for (size_t idx = 0; idx != hv_.size(); ++idx)
                        {
                                working_hv.clear();
                                for (holdem_id hid : hv_[idx])
                                {
                                        auto const& hand = holdem_hand_decl::get(hid);

                                        auto mapped_rank_first = mapping[hand.first().rank()];
                                        auto mapped_rank_second = mapping[hand.second().rank()];

                                        auto mapped_first = card_decl::make_id(
                                                hand.first().suit(),
                                                mapped_rank_first);

                                        auto mapped_second = card_decl::make_id(
                                                hand.second().suit(),
                                                mapped_rank_second);

                                        auto mapped_hid = holdem_hand_decl::make_id(mapped_first, mapped_second);

                                        working_hv.push_back(mapped_hid);

                                }

                                out.push_back(working_hv);
                                transform.push_back(transform_[idx]);
                        }
                }
                std::vector< holdem_hand_vector > hv_;
                std::vector< matrix_t > transform_;
        };
        void populate(holdem_class_vector const& cv, std::vector< holdem_hand_vector >& out, std::vector<matrix_t>& transform)
        {
                auto p = make_pattern(cv);


                auto const& pattern = std::get<0>(p);

                std::cout << cv << " => " << pattern_to_string(pattern) << "\n";
                auto const& mapping = std::get<1>(p);
                auto const& inv_mapping = std::get<2>(p);

                auto iter = prototype_map_.find(pattern);

                if (iter == prototype_map_.end())
                {


                        std::vector<frontend::range> players;
                        for (auto const& cid : cv)
                        {
                                auto const& decl = holdem_class_decl::get(cid);

                                auto mapped_cid = holdem_class_decl::make_id(
                                        decl.category(),
                                        mapping[decl.first()],
                                        mapping[decl.second()]);
  
                                players.push_back(frontend::range(holdem_class_decl::get(mapped_cid).to_string()));
                        }
                        auto instr_list = frontend_to_instruction_list("Group0", players);



                        computation_pass_manager mgr;
                        mgr.add_pass<pass_permutate_class>();
                        mgr.add_pass<pass_collect_class>();
                        mgr.add_pass<pass_class2cards>();
                        mgr.add_pass<pass_permutate>();
                        mgr.add_pass<pass_sort_type>();
                        mgr.add_pass<pass_collect>();

                        const size_t common_size = cv.size();
                        const int verboseicity = 0;
                        auto comp_ctx = std::make_shared< computation_context>(common_size, verboseicity);
                        mgr.execute_(comp_ctx.get(), &instr_list, nullptr);

                        std::vector<holdem_hand_vector> agg_hv;
                        std::vector<matrix_t> agg_trans;

                        for (auto const& untyped_instr : instr_list)
                        {
                                auto instr = reinterpret_cast<const card_eval_instruction*>(untyped_instr.get());

                                agg_hv.push_back(instr->get_vector());
                                agg_trans.push_back(instr->result_desc().back().transform());

                        }

                        prototype_map_[pattern] = std::make_shared<prototype>(agg_hv, agg_trans);
                }
                return prototype_map_.find(pattern)->second->emit(cv, inv_mapping, out, transform);
        }

        std::tuple<std::vector<match_ty>,rank_mapping_ty,rank_mapping_ty > make_pattern(holdem_class_vector const& cv)const
        {
                rank_mapping_ty rank_mapping;
                rank_mapping.fill(-1);
                rank_id rank_alloc(rank_decl::max_id);
                auto map_rank = [&rank_mapping,&rank_alloc](rank_id rid)mutable->rank_id
                {
                        if(rank_mapping[rid] == -1)
                        {
                                --rank_alloc;
                                const rank_id mapped_rank = rank_alloc;
                                rank_mapping[rid] = mapped_rank;
                                return mapped_rank;
                        }
                        else
                        {
                                return rank_mapping[rid];
                        }
                };

                std::vector<match_ty> pattern;
                for(holdem_class_id cid : cv)
                {
                        holdem_class_decl const& decl = holdem_class_decl::get(cid);

                        pattern.emplace_back(
                                decl.category(),
                                map_rank(decl.first()),
                                map_rank(decl.second()));
                }

                rank_mapping_ty inv_rank_mapping;
                inv_rank_mapping.fill(-1);
                for (rank_id iter = 0; iter != rank_decl::max_id; ++iter)
                {
                        if( rank_mapping[iter] != -1 )
                                inv_rank_mapping[rank_mapping[iter]] = iter;
                }

                std::cout << "rank_mapping = " << std_vector_to_string(rank_mapping) << "\n";
                std::cout << "inv_rank_mapping = " << std_vector_to_string(inv_rank_mapping) << "\n";
                return std::make_tuple(pattern,inv_rank_mapping,inv_rank_mapping);

        }
private:
        std::map<
                std::vector<match_ty>,
                std::shared_ptr<prototype>
        > prototype_map_;
};

static void BM_ClassHands(benchmark::State& state)
{
        const auto cv = holdem_class_vector::parse("AA,KK,QQ");
                
        for (auto _ : state)
        {  
                for( auto hv : cv.get_hand_vectors()){
                        benchmark::DoNotOptimize(hv);
                }
        }    
}
BENCHMARK(BM_ClassHands);

static void BM_ClassHands_Proto(benchmark::State& state)
{
        const auto cv = holdem_class_vector::parse("AA,KK");

        holdem_class_hard_from_proto proto;

        std::vector<holdem_hand_vector> out;
        std::vector<matrix_t> trans;

        out.clear();
        trans.clear();
        proto.populate(holdem_class_vector::parse("AA,KK"), out, trans);
        std::cout << "-------------- first --------\n";
        for (size_t idx=0;idx!=out.size();++idx)
        {
                std::cout << out[idx] << " - " << matrix_to_string(trans[idx]) << "\n";
        }

        out.clear();
        trans.clear();
        proto.populate(holdem_class_vector::parse("AA,KK"), out, trans);
        std::cout << "-------------- second --------\n";
         for (size_t idx=0;idx!=out.size();++idx)
        {
                std::cout << out[idx] << " - " << matrix_to_string(trans[idx]) << "\n";
        }


         out.clear();
        trans.clear();
        proto.populate(holdem_class_vector::parse("KK,AA"), out, trans);
        std::cout << "-------------- second --------\n";
         for (size_t idx=0;idx!=out.size();++idx)
        {
                std::cout << out[idx] << " - " << matrix_to_string(trans[idx]) << "\n";
        }

        std::exit(0);
        return;

        out.clear();
        trans.clear();
        proto.populate(holdem_class_vector::parse("AA,JJ"), out, trans);
        std::cout << "-------------- third --------\n";
        for (auto const& cv : out)
        {
                std::cout << cv << "\n";
        }

}
BENCHMARK(BM_ClassHands_Proto);


#if 0
template <class ...Args>
static void HoldemHandVector_Permutate(benchmark::State& state, Args&&... args) {

        const auto hv = holdem_hand_vector::parse(args...);
                
        for (auto _ : state)
        {  
                auto p =  permutate_for_the_better(hv) ;
                benchmark::DoNotOptimize(p);
        }    
}
BENCHMARK_CAPTURE(HoldemHandVector_Permutate, JcTc7s7dAsKd, std::string("JcTc7s7dAsKd"))->Unit(benchmark::kMillisecond);
BENCHMARK_CAPTURE(HoldemHandVector_Permutate, JcTc7s7dAdQsAsKd, std::string("JcTc7s7dAdQsAsKd"))->Unit(benchmark::kMillisecond);
BENCHMARK_CAPTURE(HoldemHandVector_Permutate, JcTc7s7dAdQsAsKd8d9d, std::string("JcTc7s7dAdQsAsKd8d9d"))->Unit(benchmark::kMillisecond);
BENCHMARK_CAPTURE(HoldemHandVector_Permutate, JcTc7s7dAdQsAsKd8d9d3c2s, std::string("JcTc7s7dAdQsAsKd8d9d3c2s"))->Unit(benchmark::kMillisecond);
#endif




#if 0

template <class ...Args>
void ClassVector_GetHandVectors(benchmark::State& state, Args&&... args) {
       
        const auto cv = holdem_class_vector::parse(args...);
                
        for (auto _ : state)
        {  
                for( auto hv : cv.get_hand_vectors()){
                        benchmark::DoNotOptimize(hv);
                }
        }    
}
BENCHMARK_CAPTURE(ClassVector_GetHandVectors, AA_KK, std::string("AA,KK"))->Unit(benchmark::kMillisecond);
BENCHMARK_CAPTURE(ClassVector_GetHandVectors, AA_KK_QQ, std::string("AA,KK,QQ"))->Unit(benchmark::kMillisecond);
BENCHMARK_CAPTURE(ClassVector_GetHandVectors, QQ_KK_AA, std::string("QQ,KK,AA"))->Unit(benchmark::kMillisecond);
BENCHMARK_CAPTURE(ClassVector_GetHandVectors, AA_KK_QQ_JJ, std::string("AA,KK,QQ,JJ"))->Unit(benchmark::kMillisecond);
BENCHMARK_CAPTURE(ClassVector_GetHandVectors, AA_KK_QQ_JJ_TT, std::string("AA,KK,QQ,JJ,TT"))->Unit(benchmark::kMillisecond);
BENCHMARK_CAPTURE(ClassVector_GetHandVectors, AKo_QJo, std::string("AKo,QJo"))->Unit(benchmark::kMillisecond);
BENCHMARK_CAPTURE(ClassVector_GetHandVectors, AKo_QJo_T9o, std::string("AKo,QJo,T9o"))->Unit(benchmark::kMillisecond);
BENCHMARK_CAPTURE(ClassVector_GetHandVectors, AKo_QJo_T9o_87o, std::string("AKo,QJo,T9o,87o"))->Unit(benchmark::kMillisecond);
BENCHMARK_CAPTURE(ClassVector_GetHandVectors, AKo_QJo_T9o_87o_65o, std::string("AKo,QJo,T9o,87o,65o"))->Unit(benchmark::kMillisecond);

static void Iterator_BitmaskFiveCardBoard(benchmark::State& state) {

        //constexpr size_t end_board = 0b1111100000000000000000000000000000000000000000000000ull;
        constexpr size_t end_board = 0b11111000000000000000000000000000ull;
        constexpr size_t end = end_board + 1;
        for (auto _ : state)
        { 
                size_t counter = 0;      
                for (size_t iter = 0; iter < end; ++iter)
                {
                        if (detail::popcount(iter) != 5)
                        {
                                continue;
                        }
                        ++counter;

                        benchmark::DoNotOptimize(iter);
                }
                std::cout << "counter = " << counter << "\n";

        }    
}
static void Iterator_FiveCardBoard(benchmark::State& state) {

        for (auto _ : state)
        {  
                for(board_combination_iterator iter(5),end;iter!=end ;++iter){
                        benchmark::DoNotOptimize(*iter);
                }
        }    
}
BENCHMARK(Iterator_FiveCardBoard)->Unit(benchmark::kMillisecond);


static void Iterator_ThreePlayerClass(benchmark::State& state) {

        for (auto _ : state)
        {  
                for(holdem_class_iterator iter(3),end;iter!=end;++iter){
                        benchmark::DoNotOptimize(*iter);
                }
        }    
}
BENCHMARK(Iterator_ThreePlayerClass)->Unit(benchmark::kMillisecond);



static void Vs_ClassRange_First1000(benchmark::State& state, size_t n) {

        for (auto _ : state)
        {  
                size_t counter = 0;
                for(holdem_class_iterator iter(n),end;iter!=end;++iter,++counter){
                        auto const& cv = *iter;
                        for( auto hv : cv.get_hand_vectors()){
                                benchmark::DoNotOptimize(hv);
                        }
                        if( counter == 1000 )
                                break;
                }
        }    
}
BENCHMARK_CAPTURE(Vs_ClassRange_First1000, Two, 2)->Unit(benchmark::kMillisecond);
BENCHMARK_CAPTURE(Vs_ClassRange_First1000, Three, 3)->Unit(benchmark::kMillisecond);
BENCHMARK_CAPTURE(Vs_ClassRange_First1000, Four, 4)->Unit(benchmark::kMillisecond);

#endif


BENCHMARK_MAIN();
