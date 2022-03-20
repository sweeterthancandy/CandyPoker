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

#include <fstream>

namespace ps{
namespace interface_ {


        struct write_to_csv_context
        {
                std::ofstream stream;
                int output_count = 0;
        };
        struct write_to_csv: computation_pass{
                    write_to_csv(std::shared_ptr<write_to_csv_context> ctx, std::string const& str)
                            : ctx_{ctx}, str_{str}
                    {}
                    virtual void transform(computation_context* ctx, instruction_list* instr_list, computation_result* result = nullptr)
                    {
                            size_t instruction_idx = 0;
                            for (auto const& untyped_instr : *instr_list)
                            {
                                    const std::string header = std::to_string(ctx_->output_count) 
                                            + "," + str_ 
                                            + "," + std::to_string(instruction_idx)
                                            + "," + boost::lexical_cast<std::string>(untyped_instr->get_type());

                                    auto do_emit_any_eval = [&](auto const& vec, const std::vector<result_description>& result_desc)mutable
                                    {
                                            for(size_t result_idx=0;result_idx!=result_desc.size();++result_idx)
                                            {
                                                    auto step_desc = result_desc[result_idx];
                                                    auto m = step_desc.transform();
                                                    auto sz = m.size();
                                                    auto iter = m.data();
                                                    auto end = iter + sz;
                                                    
                                                    ctx_->stream << header << "," << vec << "," << result_idx << "," << step_desc.group();
                                                    for(size_t j=0;j!=m.cols();++j){
                                                        for(size_t i=0;i!=m.rows();++i){
                                                                ctx_->stream << "," << m(i,j);
                                                        }
                                                    }
                                                    ctx_->stream << "\n";                                                    
                                            }
                                    };

                                    if (untyped_instr->get_type() == instruction::T_ClassEval)
                                    {
                                            auto class_eval_instr = reinterpret_cast<class_eval_instruction const*>(untyped_instr.get());
                                            const auto& vec = class_eval_instr->get_vector();
                                            const auto& result_desc = class_eval_instr->result_desc();

                                            do_emit_any_eval(vec,result_desc);
                                    }
                                    else if (untyped_instr->get_type() == instruction::T_CardEval)
                                    {
                                            auto card_eval_instr = reinterpret_cast<card_eval_instruction const*>(untyped_instr.get());

                                            const auto& vec = card_eval_instr->get_vector();
                                            const auto& result_desc = card_eval_instr->result_desc();

                                            do_emit_any_eval(vec,result_desc);
                                    }
                                    else
                                    {
                                            ctx_->stream << header << "\n";
                                    }

                                    ++instruction_idx;
                            }

                            ++ctx_->output_count;
                    }
            private:
                    std::shared_ptr<write_to_csv_context> ctx_;
                    std::string str_;
            };


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

            
            
            
            boost::timer::cpu_timer shared_timer;

            auto csv_ctx = std::make_shared<write_to_csv_context>();

            const auto now = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
            std::string csv_output_name = std::string("output.") + std::to_string(now) + ".csv";
            PS_LOG(trace) << "write csv to " << csv_output_name;
            csv_ctx->stream.open(csv_output_name);
            

            computation_pass_manager mgr;
#if 0
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
#else
            mgr.add_pass<write_to_csv>(csv_ctx, "Start");
            //mgr.add_pass<pass_permutate_class>();
            //mgr.add_pass<write_to_csv>(csv_ctx, "pass_permutate_class");
            mgr.add_pass<pass_collect_class>();
            mgr.add_pass<write_to_csv>(csv_ctx, "pass_collect_class");
            mgr.add_pass<pass_class2cards>();
            mgr.add_pass<write_to_csv>(csv_ctx, "pass_class2cards");
            mgr.add_pass<pass_permutate>();
            mgr.add_pass<write_to_csv>(csv_ctx, "pass_permutate");
            mgr.add_pass<pass_sort_type>();
            mgr.add_pass<write_to_csv>(csv_ctx, "pass_sort_type");
            mgr.add_pass<pass_collect>();
            mgr.add_pass<write_to_csv>(csv_ctx, "pass_collect");
#endif

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



} // end namespace interface_
} // end namespace ps