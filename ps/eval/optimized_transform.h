#ifndef LIB_EVAL_OPTIMIZED_TRANSFORM_H
#define LIB_EVAL_OPTIMIZED_TRANSFORM_H

#include "ps/eval/rank_opt_device.h"

// #define DO_VAlGRIND


#ifdef DO_VAlGRIND
#include <valgrind/callgrind.h>
#endif // DO_VAlGRIND

namespace ps{

    template<class T>
    struct supports_single_mask : std::false_type {};


template<class Sub,
         class Schedular,
         class Factory,
         class Eval>
struct optimized_transform : optimized_transform_base
{
        using factory_type   = typename Factory::template bind<Sub>;
        using sub_ptr_type   = typename factory_type::sub_ptr_type;
        using schedular_type = typename Schedular::template bind<sub_ptr_type>;
        using eval_type      = Eval;

        virtual void apply(optimized_transform_context& otc, computation_context* ctx, instruction_list* instr_list, computation_result* result,
                   std::vector<typename instruction_list::iterator> const& target_list)noexcept override
        {
                boost::timer::cpu_timer tmr;

                constexpr bool WithLogging = true;

#ifdef DO_VAlGRIND
                CALLGRIND_START_INSTRUMENTATION;
#endif // DO_VAlGRIND

                // this needs to outlive the lifetime of every object
                factory_type factory;

                std::vector<sub_ptr_type> subs;

                for(auto& target : target_list){
                        auto instr = reinterpret_cast<card_eval_instruction*>((*target).get());
                        subs.push_back( factory(target, instr) );
                }

                // Here we create a list of all the evaluations that need to be done.
                // Because each evalution is done card wise, doing multiple evaluations
                // are the same time, some of the cards are shared
                // 
                std::unordered_set<holdem_id> S;
                for(auto& _ : subs){
                        _->declare(S);
                }

                if(WithLogging) PS_LOG(trace) << "Have " << S.size() << " unique holdem hands";

                // this is the maximually speed up the compution, by preocompyting some stuff
                rank_opt_device rod = rank_opt_device::create(S);
                std::unordered_map<holdem_id, size_t> allocation_table;
                for(size_t idx=0;idx!=rod.size();++idx){
                        allocation_table[rod[idx].hid] = idx;
                }
                for(auto& _ : subs){
                        _->allocate( [&](auto hid){ return allocation_table.find(hid)->second; });
                }

                if (WithLogging) PS_LOG(trace) << "Have " << subs.size() << " subs";

                std::vector<ranking_t> R;
                R.resize(rod.size());

                

                using weights_ty = std::vector< eval_counter_type>;
                weights_ty weights;
                weights.resize(subs.size());
                
                
                schedular_type shed{ rod.size(), subs};


                if (WithLogging) PS_LOG(trace) << tmr.format(4, "init took %w seconds");
                tmr.start();
                size_t count = 0;
                //std::unordered_map<int, int> non_zero_m;

                constexpr bool enable_weight_branch{ false };




                std::vector<ranking_t> ranking_proto;
                ranking_proto.resize(rod.size());

                std::array< std::vector<ranking_t>, 4> suit_batch;
                suit_batch[0].resize(rod.size());
                suit_batch[1].resize(rod.size());
                suit_batch[2].resize(rod.size());
                suit_batch[3].resize(rod.size());


                for (auto const& g : otc.w.grouping)
                {
                    for (size_t idx = 0; idx != rod.size(); ++idx) {
                        auto const& hand_decl = rod[idx];
                        ranking_proto[idx] = g.no_flush_rank(hand_decl.r0, hand_decl.r1);
                    }

                    for (size_t idx = 0; idx != rod.size(); ++idx) {
                        shed.put(idx, ranking_proto[idx]);
                    }
                    shed.end_eval(&g.get_no_flush_masks(), 0ull);





                    for (auto const& f : g.suit_symmetry_vec())
                    {

                        for (suit_id sid = 0; sid != 4; ++sid)
                        {

                            for (size_t idx = 0; idx != rod.size(); ++idx) {
                                auto const& hand_decl = rod[idx];
                                ranking_t rr = g.no_flush_rank(hand_decl.r0, hand_decl.r1);
                                bool s0m = (hand_decl.s0 == sid);
                                bool s1m = (hand_decl.s1 == sid);

                                auto fm = f.flush_mask();

                                if (s0m) {
                                    fm |= 1ull << hand_decl.r0;
                                }
                                if (s1m) {
                                    fm |= 1ull << hand_decl.r1;
                                }

                                ranking_t sr = otc.fme(fm);
                                ranking_t tr = std::min(sr, rr);

                                shed.put(idx, tr);
                            }
                            shed.end_eval(&f.board_card_masks()[sid], 0ull);

                        }   
                    }
            






                    continue;
                    for (auto const& f : g.suit_symmetry_vec())
                    {
                        const size_t fm_proto = f.flush_mask();

                        // we now do 4 boards at once
                        suit_batch[0] = ranking_proto;
                        suit_batch[1] = ranking_proto;
                        suit_batch[2] = ranking_proto;
                        suit_batch[3] = ranking_proto;

                        for (size_t idx = 0; idx != rod.size(); ++idx) {
                            auto const& hand_decl = rod[idx];

                            auto commit_flush = [&](suit_id sid, size_t flush_mask)
                            {
                                ranking_t sr = otc.fme(flush_mask);
                                ranking_t tr = std::min(sr, suit_batch[sid][idx]);
                                suit_batch[sid][idx] = tr;
                            };

                            if (hand_decl.s0 == hand_decl.s1)
                            {
                                auto fm = fm_proto | (1ull << hand_decl.r0) | (1ull << hand_decl.r1);
                                commit_flush(hand_decl.s0, fm);
                            }
                            else
                            {
                                auto first_fm = fm_proto | (1ull << hand_decl.r0);
                                auto second_fm = fm_proto | (1ull << hand_decl.r1);
                                commit_flush(hand_decl.s0, first_fm);
                                commit_flush(hand_decl.s1, second_fm);
                            }
                        }

                        for (suit_id sid = 0; sid != 4; ++sid)
                        {
                            mask_set const& suit_mask_set = f.board_card_masks()[sid];
                            for (size_t idx = 0; idx != rod.size(); ++idx) {
                                shed.put(idx, suit_batch[sid][idx]);
                            }
                            shed.end_eval(&suit_mask_set, 0ull);
                        }
                    }

                    
                    ++count;
                }








                if (WithLogging) PS_LOG(trace) << tmr.format(4, "no    flush boards took %w seconds") << " to do " << count << " boards";
                tmr.start();

                if (WithLogging) PS_LOG(trace) << tmr.format(4, "maybe flush boards took %w seconds") << " to do " << count << " boards";


                shed.regroup();

                
                for(auto& _ : subs){
                        _->finish();
                }

#ifdef DO_VAlGRIND
                CALLGRIND_STOP_INSTRUMENTATION;
                CALLGRIND_DUMP_STATS;
#endif
        }
};
} // end namespace ps

#endif // LIB_EVAL_OPTIMIZED_TRANSFORM_H
