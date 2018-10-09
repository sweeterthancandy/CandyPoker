#ifndef PS_BASE_COMPUTER_H
#define PS_BASE_COMPUTER_H

#include "ps/base/instruction.h"
#include "ps/eval/equity_breakdown_matrix.h"


namespace ps{

struct computation_context{
        explicit
        computation_context(size_t num_players)
                :num_players_(num_players)
        {}
        size_t NumPlayers()const{
                return num_players_;
        }
        friend std::ostream& operator<<(std::ostream& ostr, computation_context const& self){
                ostr << "num_players_ = " << self.num_players_;
                return ostr;
        }
private:
        size_t num_players_;
};

struct computer{
        virtual ~computer()=default;
        virtual matrix_t compute(computation_context const& ctx, instruction_list const& instr_list)=0;
};

// most common case is to compute each standard card computation
struct card_eval_computer : computer{

        virtual matrix_t compute_single(computation_context const& ctx, card_eval_instruction const& instr)const noexcept=0;
        virtual matrix_t compute(computation_context const& ctx, instruction_list const& instr_list)override{
                instruction_list my_instr_list = instruction_list_deep_copy(instr_list);
                auto card_instr_list = transform_cast_to_card_eval(my_instr_list);
                

                #if 0
                std::vector<std::future<compute_single_result_t> > work;

                for(auto const& instr : card_instr_list ){
                        auto fut = std::async(std::launch::async, [this, &ctx, &instr](){ return compute_single(ctx, instr); });
                        work.emplace_back(std::move(fut));
                        #if 0
                        work.emplace_back(std
                        //agg->append_matrix(*sub, instr.get_matrix() );
                        agg->append_matrix(*std::get<0>(ret), std::get<1>(ret));
                        #endif
                }
                for(auto& _ : work){
                        auto ret = _.get();
                        agg->append_matrix(*std::get<0>(ret), std::get<1>(ret));
                }
                #endif
                matrix_t mat(ctx.NumPlayers(), ctx.NumPlayers());
                mat.fill(0ull);
                for(auto const& instr : card_instr_list ){
                        mat += compute_single(ctx, instr);
                }
                return mat;
        }
};

} // end namespace ps

#endif // PS_BASE_COMPUTER_H
