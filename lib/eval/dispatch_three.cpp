#include "lib/eval/dispatch_table.h"
#include "lib/eval/generic_shed.h"
#include "lib/eval/generic_sub_eval.h"
#include "lib/eval/optimized_transform.h"

namespace ps{

        struct sub_eval_three{
                using iter_t = instruction_list::iterator;
                sub_eval_three(iter_t iter, card_eval_instruction* instr)
                        :iter_{iter}, instr_{instr}
                {
                        hv   = instr->get_vector();
                        hv_mask = hv.mask();

                        wins_.fill(0);
                        draw2_.fill(0);
                        draw3_.fill(0);
                }
                void accept(mask_set const* ms, size_t single_mask, std::vector<ranking_t> const& R)noexcept
                {

                       size_t weight = [&]()noexcept->size_t{
                                if( !! ms ){
                                        if( (ms->get_union() & hv_mask) == 0 ){
                                                return ms->size();
                                        }
                                        return ms->count_disjoint(hv_mask);
                                } else{
                                        return ( ( hv_mask & single_mask) == 0 ? 1 : 0 );
                                }
                        }();

                        if( weight == 0 )
                                return;

                        auto r0 = R[allocation_[0]];
                        auto r1 = R[allocation_[1]];
                        auto r2 = R[allocation_[2]];
                                        

                        if( r0 < r1 ){
                                if( r0 < r2 ){
                                        wins_[0] += weight;
                                } else if( r2 < r0 ){
                                        wins_[2] += weight;
                                } else {
                                        draw2_[0] += weight;
                                        draw2_[2] += weight;
                                }
                        } else if( r1 < r0 ){
                                if( r1 < r2 ){
                                        wins_[1] += weight;
                                } else if( r2 < r1 ){
                                        wins_[2] += weight;
                                } else {
                                        draw2_[1] += weight;
                                        draw2_[2] += weight;
                                }
                        } else {
                                // ok one of three casese
                                //    1) r0,r1 draw with each other
                                //    2) r2 wins
                                //    3) r0,r1,r2 draw with each other
                                if( r0 < r2 ){
                                        draw2_[0] += weight;
                                        draw2_[1] += weight;
                                } else if( r2 < r0 ){
                                        wins_[2] += weight;
                                } else {
                                        draw3_[0] += weight;
                                        draw3_[1] += weight;
                                        draw3_[2] += weight;
                                }
                        }
                }
                void finish()noexcept{
                        matrix_t mat;
                        mat.resize(n, n);
                        mat.fill(0);
                        for(size_t idx=0;idx!=n;++idx){
                                mat(0, idx) += wins_[idx];
                                mat(1, idx) += draw2_[idx];
                                mat(2, idx) += draw3_[idx];
                        }
                        *iter_ = std::make_shared<matrix_instruction>(instr_->group(), mat * instr_->get_matrix());
                }
                void declare(std::unordered_set<holdem_id>& S)noexcept{
                        for(auto _ : hv){
                                S.insert(_);
                        }
                }
                template<class Alloc>
                void allocate(Alloc const& alloc)noexcept{
                        for(size_t idx=0;idx!=n;++idx){
                                allocation_[idx] = alloc(hv[idx]);
                        }
                }
        private:
                iter_t iter_;
                card_eval_instruction* instr_;

                holdem_hand_vector hv;
                size_t hv_mask;
                size_t n{3};

                std::array<size_t, 9> allocation_;
                std::array<size_t, 9> wins_;
                std::array<size_t, 9> draw2_;
                std::array<size_t, 9> draw3_;
        };



        

        
        #if 0
        struct sub_eval_three_perm{
                enum{ UpperMask = 0b111 + 1 };
                using iter_t = instruction_list::iterator;
                sub_eval_three_perm(iter_t iter, card_eval_instruction* instr)
                        :iter_{iter}, instr_{instr}
                {
                        hv   = instr->get_vector();
                        hv_mask = hv.mask();
                        mat.resize(n, n);
                        mat.fill(0);
                        eval_.fill(0);
                }
                void accept(mask_set const& ms, std::vector<ranking_t> const& R)noexcept
                {
                        size_t weight = ms.count_disjoint(hv_mask);
                        if( weight == 0 )
                                return;

                        auto r0 = R[allocation_[0]];
                        auto r1 = R[allocation_[1]];
                        auto r2 = R[allocation_[2]];

                                        
                        //auto r_min = std::min({r0,r1,r2});
                        auto r_min = std::min(r0,std::min(r1,r2));


                        std::uint16_t mask = 0;
                        if( r_min == r0 )
                                mask |= 0b001;
                        if( r_min == r1 )
                                mask |= 0b010;
                        if( r_min == r2 )
                                mask |= 0b100;

                        //std::cout << "std::bitset<16>(mask).to_string() => " << std::bitset<16>(mask).to_string() << "\n"; // __CandyPrint__(cxx-print-scalar,std::bitset<16>(mask).to_string())

                        eval_.at(mask) += weight;
                }
                void finish(){
                        mat(0,0) += eval_[0b001];
                        mat(0,1) += eval_[0b010];
                        mat(0,2) += eval_[0b100];

                        #if 0
                        for(int mask = 1; mask != UpperMask; ++mask){
                                auto pcnt = __builtin_popcount(mask);
                                if( mask & 0b001 )
                                        mat(pcnt-1, 0) += eval_[mask];
                                if( mask & 0b010 )
                                        mat(pcnt-1, 1) += eval_[mask];
                                if( mask & 0b100 )
                                        mat(pcnt-1, 2) += eval_[mask];
                        }
                        #endif
                        *iter_ = std::make_shared<matrix_instruction>(instr_->group(), mat * instr_->get_matrix());
                }
                void declare(std::unordered_set<holdem_id>& S){
                        for(auto _ : hv){
                                S.insert(_);
                        }
                }
                template<class Alloc>
                void allocate(Alloc const& alloc){
                        for(size_t idx=0;idx!=n;++idx){
                                allocation_[idx] = alloc(hv[idx]);
                        }
                }
        private:
                iter_t iter_;
                card_eval_instruction* instr_;

                holdem_hand_vector hv;
                size_t hv_mask;
                matrix_t mat;
                size_t n{3};

                std::array<size_t, 9> allocation_;
                std::array<std::uint16_t, UpperMask > eval_;
        };
        #endif
        
        
        
        


        struct dispatch_three_player : dispatch_table{
                using transform_type =
                        optimized_transform<
                        sub_eval_three,
                        generic_shed,
                        basic_sub_eval_factory,
                        rank_hash_eval>;


                virtual bool match(dispatch_context const& dispatch_ctx)const override{
                        return dispatch_ctx.homo_num_players &&  dispatch_ctx.homo_num_players.get() == 3;
                }
                virtual std::shared_ptr<optimized_transform_base> make()const override{
                        return std::make_shared<transform_type>();
                }
                virtual std::string name()const override{
                        return "three-player-generic";
                }
                virtual size_t precedence()const override{ return 100; }
        };
        static register_disptach_table<dispatch_three_player> reg_reg_dispatch_generic;
        
        
        
        #if 0
        
        
        
        struct sub_eval_three_perm{
                enum{ UpperMask = 0b111 + 1 };
                using iter_t = instruction_list::iterator;
                sub_eval_three_perm(iter_t iter, card_eval_instruction* instr)
                        :iter_{iter}, instr_{instr}
                {
                        hv   = instr->get_vector();
                        hv_mask = hv.mask();

                        eval_.fill(0);
                }
                void accept(mask_set const& ms, std::vector<ranking_t> const& R)noexcept
                {
                        size_t weight = ms.count_disjoint(hv_mask);
                        if( weight == 0 )
                                return;

                        auto r0 = R[allocation_[0]];
                        auto r1 = R[allocation_[1]];
                        auto r2 = R[allocation_[2]];
                                        
                        auto r_min = std::min({r0,r1,r2});

                        std::uint16_t mask = 0;
                        if( r_min == r0 )
                                mask |= 0b001;
                        if( r_min == r1 )
                                mask |= 0b010;
                        if( r_min == r2 )
                                mask |= 0b100;

                        eval_[mask] += weight;

                }
                void finish(){
                        matrix_t mat;
                        mat.resize(n, n);
                        mat.fill(0);
                        for(int mask = 1; mask != UpperMask; ++mask){
                                auto pcnt = __builtin_popcount(mask);
                                if( mask & 0b001 )
                                        mat(pcnt-1, 0) += eval_[mask];
                                if( mask & 0b010 )
                                        mat(pcnt-1, 1) += eval_[mask];
                                if( mask & 0b100 )
                                        mat(pcnt-1, 2) += eval_[mask];
                        }
                        *iter_ = std::make_shared<matrix_instruction>(instr_->group(), mat * instr_->get_matrix());
                }
                void declare(std::unordered_set<holdem_id>& S){
                        for(auto _ : hv){
                                S.insert(_);
                        }
                }
                template<class Alloc>
                void allocate(Alloc const& alloc){
                        for(size_t idx=0;idx!=n;++idx){
                                allocation_[idx] = alloc(hv[idx]);
                        }
                }
        private:
                iter_t iter_;
                card_eval_instruction* instr_;

                holdem_hand_vector hv;
                size_t hv_mask;
                size_t n{3};

                std::array<size_t, 9> allocation_;
                std::array<size_t, UpperMask> eval_;
        };

        // this seems slower
        struct dispatch_three_player_perm : dispatch_table{
                using transform_type =
                        optimized_transform<
                        sub_eval_three_perm,
                        generic_shed,
                        basic_sub_eval_factory,
                        rank_hash_eval>;


                virtual bool match(dispatch_context const& dispatch_ctx)const override{
                        return dispatch_ctx.homo_num_players &&  dispatch_ctx.homo_num_players.get() == 3;
                }
                virtual std::shared_ptr<optimized_transform_base> make()const override{
                        return std::make_shared<transform_type>();
                }
                virtual std::string name()const override{
                        return "three-player-perm";
                }
                virtual size_t precedence()const override{ return 101; }
        };
        static register_disptach_table<dispatch_three_player_perm> reg_dispatch_three_player_perm;
        #endif


} // end namespace ps
