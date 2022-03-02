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
#include "ps/interface/interface.h"


#include <boost/timer/timer.hpp>

#include <boost/log/trivial.hpp>

#include <Eigen/Dense>
#include <fstream>

#include "ps/support/command.h"
#include "ps/support/persistent.h"
#include "ps/eval/holdem_class_vector_cache.h"



namespace ps{
namespace interface_ {

    class EvaluationObjectImpl
    {
    public:
        virtual ~EvaluationObjectImpl() = default;
        virtual std::vector<EvaulationResultView> ComputeListImpl()const = 0;
    };

    class EvaluationObjectImplPrepared : public EvaluationObjectImpl
    {
    public:
        EvaluationObjectImplPrepared(
            EvaluationObject::Flags flags,
            std::vector<std::vector<std::string> > const& player_ranges_list,
            std::string const& engine,
            std::vector<std::string> const& tag_list,
            std::shared_ptr< computation_context> const& comp_ctx,
            std::shared_ptr< computation_result> const& result,
            std::shared_ptr< instruction_list> const& agg_instr_list)
         
            : flags_{flags}
            , player_ranges_list_ {
                player_ranges_list
            }
            , engine_{ engine }
            , tag_list_{tag_list}
            , comp_ctx_{ comp_ctx }
            , result_{ result }
            , agg_instr_list_{agg_instr_list}
        {

            PS_ASSERT(player_ranges_list.size() > 0, "need at least one");
        }

        std::vector<EvaulationResultView> ComputeListImpl()const override
        {
            computation_pass_manager mgr;
           
            mgr.add_pass<pass_eval_hand_instr_vec>(engine_);
            if (flags_ & ( EvaluationObject::F_DebugInstructions |  EvaluationObject::F_ShowInstructions))
                    mgr.add_pass<pass_print>();
            mgr.add_pass<pass_write>();
            mgr.execute_(comp_ctx_.get(), agg_instr_list_.get(), result_.get());

            std::vector< EvaulationResultView> result_view;
            for (size_t idx = 0; idx != player_ranges_list_.size(); ++idx)
            {
                auto const& tag = tag_list_[idx];
                auto const& player_ranges = player_ranges_list_[idx];
                result_view.push_back(EvaulationResultView{ player_ranges, result_->allocate_tag(tag) });
            }
            return result_view;
        }
    private:
        EvaluationObject::Flags flags_;
        std::vector<std::vector<std::string> > player_ranges_list_;
        std::string engine_;

        std::vector<std::string> tag_list_;
        std::shared_ptr< computation_context> comp_ctx_;
        std::shared_ptr< computation_result> result_;
        std::shared_ptr< instruction_list> agg_instr_list_;
    };

    class EvaluationObjectImplUnprepared : public EvaluationObjectImpl
    {
    public:
        EvaluationObjectImplUnprepared(
            std::vector<std::vector<std::string> > const& player_ranges_list,
            boost::optional<std::string> const& engine,
            EvaluationObject::Flags flags )
            : player_ranges_list_{ player_ranges_list }
            , engine_{ engine.value_or("") }
            , flags_{flags}
        {
            PS_ASSERT(player_ranges_list.size() > 0, "need at least one");
        }

        std::vector<EvaulationResultView> ComputeListImpl()const override
        {
            auto prepared = MakePrepared();
            return prepared->ComputeListImpl();
        }
        std::shared_ptr<EvaluationObjectImplPrepared> MakePrepared()const
        {
            const size_t common_size = player_ranges_list_[0].size();
            const int verboseicity = (( flags_ & EvaluationObject::F_StepPercent) ? 2 : 0);
            auto comp_ctx = std::make_shared< computation_context>(common_size, verboseicity);
            auto result = std::make_shared< computation_result>(*comp_ctx);
            size_t index = 0;
            std::vector<std::string> tag_list;

            auto agg_instr_list = std::make_shared< instruction_list>();
            for (auto const& player_ranges : player_ranges_list_)
            {
                if (player_ranges.size() != common_size)
                {
                    BOOST_THROW_EXCEPTION(std::domain_error("all players must be same size"));
                }
                std::vector<frontend::range> players;
                for (auto const& s : player_ranges) {
                    players.push_back(frontend::range(s));
                }

                std::string tag = "Tag_" + std::to_string(index);
                ++index;

                result->allocate_tag(tag);
                tag_list.push_back(tag);

                instruction_list instr_list = frontend_to_instruction_list(tag, players);

                std::copy(
                    std::cbegin(instr_list),
                    std::cend(instr_list),
                    std::back_inserter(*agg_instr_list));

            }

            
            const bool should_emit_times = (flags_ & EvaluationObject::F_TimeInstructionManager);
            const bool should_debug_instrs = (flags_ & EvaluationObject::F_DebugInstructions);

            
            struct time_printer : computation_pass{
                    explicit time_printer(std::string const& str, boost::timer::cpu_timer* tmr):
                            str_{str}, tmr_{tmr}
                    {}
                    virtual void transform(computation_context* ctx, instruction_list* instr_list, computation_result* result = nullptr)
                    {
                            PS_LOG(trace) << tmr_->format(4, str_ + " took %w seconds");
                            tmr_->start();
                    }
            private:
                    std::string str_;
                    boost::timer::cpu_timer* tmr_;
            };
            
            boost::timer::cpu_timer shared_timer;
            

            computation_pass_manager mgr;
            mgr.add_pass<pass_permutate_class>();
            if (should_debug_instrs)
                mgr.add_pass<pass_print>();
            if( should_emit_times)
                mgr.add_pass<time_printer>("permutate_class", &shared_timer);


            mgr.add_pass<pass_collect_class>();
            if (should_debug_instrs)
                mgr.add_pass<pass_print>();
            if( should_emit_times)
                mgr.add_pass<time_printer>("pass_collect_class", &shared_timer);

            mgr.add_pass<pass_class2cards>();
            if (should_debug_instrs)
                mgr.add_pass<pass_print>();
            if( should_emit_times)
                mgr.add_pass<time_printer>("pass_class2cards", &shared_timer);


            mgr.add_pass<pass_permutate>();
            if (should_debug_instrs)
                mgr.add_pass<pass_print>();
            if( should_emit_times)
                mgr.add_pass<time_printer>("pass_permutate", &shared_timer);


            mgr.add_pass<pass_sort_type>();
            mgr.add_pass<pass_collect>();
            if (flags_ & ( EvaluationObject::F_DebugInstructions |  EvaluationObject::F_ShowInstructions))
                mgr.add_pass<pass_print>();
            if( should_emit_times)
                mgr.add_pass<time_printer>("pass_collect", &shared_timer);

            mgr.execute_(comp_ctx.get(), agg_instr_list.get(), result.get());

            return std::make_shared< EvaluationObjectImplPrepared>(
                flags_,
                player_ranges_list_,
                engine_,
                tag_list,
                comp_ctx,
                result,
                agg_instr_list);
        }
    private:
        std::vector<std::vector<std::string> > player_ranges_list_;
        std::string engine_;
        EvaluationObject::Flags flags_;
    };


    EvaluationObject::EvaluationObject(std::vector<std::vector<std::string> > const& player_ranges_list, boost::optional<std::string> const& engine, EvaluationObject::Flags flags)
    {
        impl_ = std::make_shared< EvaluationObjectImplUnprepared>(
            player_ranges_list,
            engine,
            flags);
    }

    std::vector<EvaulationResultView> EvaluationObject::ComputeList()const
    {
        return impl_->ComputeListImpl();
    }
    boost::optional<EvaluationObject> EvaluationObject::Prepare()const
    {
        if (auto ptr = std::dynamic_pointer_cast<EvaluationObjectImplUnprepared>(impl_))
        {
            return EvaluationObject{ ptr->MakePrepared() };
        }
        return {};
    }

    void EvaluationObject::BuildCache()
    {
        pass_eval_hand_instr_vec::ready_cache();
    }

#if 0

    void test_prepare(std::vector<std::string> const& player_ranges, std::string const& engine)
    {
        std::vector<std::vector<std::string> > player_ranges_list{ player_ranges };
        if (player_ranges_list.empty())
        {
            return;
        }
        const size_t common_size = player_ranges_list[0].size();
        computation_context comp_ctx{ common_size };
        computation_result result{ comp_ctx };
        size_t index = 0;
        std::vector<std::string> tag_list;

        instruction_list agg_instr_list;
        for (auto const& player_ranges : player_ranges_list)
        {
            if (player_ranges.size() != common_size)
            {
                BOOST_THROW_EXCEPTION(std::domain_error("all players must be same size"));
            }
            std::vector<frontend::range> players;
            for (auto const& s : player_ranges) {
                players.push_back(frontend::parse(s));
            }

            std::string tag = "Tag_" + std::to_string(index);
            ++index;

            result.allocate_tag(tag);
            tag_list.push_back(tag);

            instruction_list instr_list = frontend_to_instruction_list(tag, players);

            std::copy(
                std::cbegin(instr_list),
                std::cend(instr_list),
                std::back_inserter(agg_instr_list));

        }

        computation_pass_manager mgr;
        mgr.add_pass<pass_permutate>();
        mgr.add_pass<pass_sort_type>();
        mgr.add_pass<pass_collect>();
        mgr.execute_(&comp_ctx, &agg_instr_list, &result);
    }

    std::vector<EvaulationResultView> evaluate_list(std::vector<std::vector<std::string> > const& player_ranges_list, std::string const& engine)
    {
        if (player_ranges_list.empty())
        {
            return {};
        }
        const size_t common_size = player_ranges_list[0].size();
        computation_context comp_ctx{ common_size };
        computation_result result{ comp_ctx };
        size_t index = 0;
        std::vector<std::string> tag_list;

        instruction_list agg_instr_list;
        for (auto const& player_ranges : player_ranges_list)
        {
            if (player_ranges.size() != common_size)
            {
                BOOST_THROW_EXCEPTION(std::domain_error("all players must be same size"));
            }
            std::vector<frontend::range> players;
            for (auto const& s : player_ranges) {
                players.push_back(frontend::range(s));
            }

            std::string tag = "Tag_" + std::to_string(index);
            ++index;

            result.allocate_tag(tag);
            tag_list.push_back(tag);

            instruction_list instr_list = frontend_to_instruction_list(tag, players);

            std::copy(
                std::cbegin(instr_list),
                std::cend(instr_list),
                std::back_inserter(agg_instr_list));
            
        }

        const bool debug = false;

        computation_pass_manager mgr;
        mgr.add_pass<pass_permutate>();
        if (debug)
            mgr.add_pass<pass_print>();
        mgr.add_pass<pass_sort_type>();
        if (debug)
            mgr.add_pass<pass_print>();
        mgr.add_pass<pass_collect>();
        if (debug)
            mgr.add_pass<pass_print>();
        mgr.add_pass<pass_eval_hand_instr_vec>(engine);
        if (debug)
            mgr.add_pass<pass_print>();
        mgr.add_pass<pass_write>();
        if (debug)
            mgr.add_pass<pass_print>();
        mgr.execute_(&comp_ctx, &agg_instr_list, &result);

        std::vector< EvaulationResultView> result_view;
        for (size_t idx =0; idx!= player_ranges_list.size();++idx)
        {
            auto const& tag = tag_list[idx];
            auto const& player_ranges = player_ranges_list[idx];
            result_view.push_back(EvaulationResultView{ player_ranges, result.allocate_tag(tag) });
        }
        return result_view;
    }

    EvaulationResultView evaluate(std::vector<std::string> const& player_ranges, std::string const& engine)
    {
        auto packed_result = evaluate_list({ player_ranges }, engine);
        return packed_result[0];
    }
#endif

} // end namespace interface_
} // end namespace ps