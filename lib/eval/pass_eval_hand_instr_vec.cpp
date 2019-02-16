#include "ps/eval/pass_eval_hand_instr_vec.h"
#include "lib/eval/rank_opt_device.h"
#include "lib/eval/dispatch_table.h"
#include "lib/eval/generic_shed.h"
#include "lib/eval/generic_sub_eval.h"
#include "lib/eval/optimized_transform.h"

namespace ps{

















































        
        struct sub_eval_two{
                using iter_t = instruction_list::iterator;
                sub_eval_two(iter_t iter, card_eval_instruction* instr)
                        :iter_{iter}, instr_{instr}
                {
                        hv   = instr->get_vector();
                        hv_mask = hv.mask();
                        mat.resize(n, n);
                        mat.fill(0);

                        wins_.fill(0);
                        draw2_.fill(0);
                }
                void accept(size_t mask, size_t weight, std::vector<ranking_t> const& R)noexcept
                {
                        bool cond = (mask & hv_mask ) == 0;
                        if(!cond){
                                return;
                        }
                        auto r0 = R[allocation_[0]];
                        auto r1 = R[allocation_[1]];

                        if( r0 < r1 ){
                                wins_[0] += weight;
                        } else if( r1 < r0 ){
                                wins_[1] += weight;
                        } else{
                                draw2_[0] += weight;
                                draw2_[1] += weight;
                        }
                }
                void finish(){
                        for(size_t idx=0;idx!=n;++idx){
                                mat(0, idx) += wins_[idx];
                                mat(1, idx) += draw2_[idx];
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
                matrix_t mat;
                size_t n{2};

                std::array<size_t, 9> allocation_;
                std::array<size_t, 9> wins_;
                std::array<size_t, 9> draw2_;
        };
        
        struct sub_eval_three{
                using iter_t = instruction_list::iterator;
                sub_eval_three(iter_t iter, card_eval_instruction* instr)
                        :iter_{iter}, instr_{instr}
                {
                        hv   = instr->get_vector();
                        hv_mask = hv.mask();
                        mat.resize(n, n);
                        mat.fill(0);

                        wins_.fill(0);
                        draw2_.fill(0);
                        draw3_.fill(0);
                }
                void accept(mask_set const& ms, std::vector<ranking_t> const& R)noexcept
                {
                        size_t weight = ms.count_disjoint(hv_mask);
                        if( weight == 0 )
                                return;
                        #if 0
                        Eigen::Vector3i V{
                                R[allocation_[0]],
                                R[allocation_[1]],
                                R[allocation_[2]]};
                        auto min = V.minCoeff();
                        if( V(0) == min ){
                                if( V(1) == min ){
                                        if( V(2) == min ){
                                                draw3_[0] += weight;
                                                draw3_[1] += weight;
                                                draw3_[2] += weight;
                                        } else {
                                                draw2_[0] += weight;
                                                draw2_[1] += weight;
                                        }
                                } else if( V(2) == min ){
                                        draw2_[0] += weight;
                                        draw2_[2] += weight;
                                } else {
                                        wins_[0] += weight;
                                }
                        } else if( V(1) == min ){
                                if( V(2) == min ){
                                        draw2_[1] += weight;
                                        draw2_[2] += weight;
                                } else {
                                        wins_[1] += weight;
                                }
                        } else {
                                        wins_[2] += weight;
                        }
                        return;
                        #endif

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
                void finish(){
                        for(size_t idx=0;idx!=n;++idx){
                                mat(0, idx) += wins_[idx];
                                mat(1, idx) += draw2_[idx];
                                mat(2, idx) += draw3_[idx];
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
                matrix_t mat;
                size_t n{3};

                std::array<size_t, 9> allocation_;
                std::array<size_t, 9> wins_;
                std::array<size_t, 9> draw2_;
                std::array<size_t, 9> draw3_;
        };
        
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

        struct sub_eval_four{
                using iter_t = instruction_list::iterator;
                sub_eval_four(iter_t iter, card_eval_instruction* instr)
                        :iter_{iter}, instr_{instr}
                {
                        hv   = instr->get_vector();
                        hv_mask = hv.mask();
                        mat.resize(n, n);
                        mat.fill(0);

                        wins_.fill(0);
                        draw2_.fill(0);
                        draw3_.fill(0);
                        draw4_.fill(0);
                }
                void accept(mask_set const& ms, std::vector<ranking_t> const& R)noexcept
                {
                        size_t weight = ms.count_disjoint(hv_mask);
                        if( weight == 0 )
                                return;

                        auto rank_0 = R[allocation_[0]];
                        auto rank_1 = R[allocation_[1]];
                        auto rank_2 = R[allocation_[2]];
                        auto rank_3 = R[allocation_[3]];

                        // pick best out of 0,1
                        auto l = rank_0;
                        auto li = 0;
                        if( rank_1 < rank_0 ){
                                l = rank_1;
                                li = 1;
                        }

                        // pick best out of 2,3
                        auto r = rank_2;
                        auto ri = 2;
                        if( rank_3 < rank_2 ){
                                r = rank_3;
                                ri = 3;
                        }

                        if( l < r ){
                                if( rank_0 == rank_1 ){
                                        draw2_[0] += weight;
                                        draw2_[1] += weight;
                                } else{
                                        wins_[li] += weight;
                                }
                        } else if( r < l ){
                                if( rank_2 == rank_3 ){
                                        draw2_[2] += weight;
                                        draw2_[3] += weight;
                                } else{
                                        wins_[ri] += weight;
                                }
                        } else {
                                // maybe {0,1,2,3} draw
                                if( rank_0 < rank_1 ){
                                        // {0}, maybe {2,3}
                                        if( rank_2 < rank_3 ){
                                                draw2_[0] += weight;
                                                draw2_[2] += weight;
                                        } else if ( rank_3 < rank_2 ){
                                                draw2_[0] += weight;
                                                draw2_[3] += weight;
                                        } else {
                                                draw3_[0] += weight;
                                                draw3_[2] += weight;
                                                draw3_[3] += weight;
                                        }
                                } else if (rank_1 < rank_0 ){
                                        // {1}, maybe {2,3}
                                        if( rank_2 < rank_3 ){
                                                draw2_[1] += weight;
                                                draw2_[2] += weight;
                                        } else if ( rank_3 < rank_2 ){
                                                draw2_[1] += weight;
                                                draw2_[3] += weight;
                                        } else {
                                                draw3_[1] += weight;
                                                draw3_[2] += weight;
                                                draw3_[3] += weight;
                                        }
                                }
                                else {
                                        // {0,1}, maybe {2,3}
                                        if( rank_2 < rank_3 ){
                                                draw3_[0] += weight;
                                                draw3_[1] += weight;
                                                draw3_[2] += weight;
                                        } else if ( rank_3 < rank_2 ){
                                                draw3_[0] += weight;
                                                draw3_[1] += weight;
                                                draw3_[3] += weight;
                                        } else {
                                                draw4_[0] += weight;
                                                draw4_[1] += weight;
                                                draw4_[2] += weight;
                                                draw4_[3] += weight;
                                        }
                                }
                        }

                }
                void finish(){
                        for(size_t idx=0;idx!=n;++idx){
                                mat(0, idx) += wins_[idx];
                                mat(1, idx) += draw2_[idx];
                                mat(2, idx) += draw3_[idx];
                                mat(3, idx) += draw4_[idx];
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
                matrix_t mat;
                size_t n{4};

                std::array<size_t, 9> allocation_;
                std::array<size_t, 9> wins_;
                std::array<size_t, 9> draw2_;
                std::array<size_t, 9> draw3_;
                std::array<size_t, 9> draw4_;
        };

        






























        struct sub_eval_three_intrinsic{
                enum{ UpperMask = 0b111 + 1 };
                using iter_t = instruction_list::iterator;
                sub_eval_three_intrinsic(iter_t iter, card_eval_instruction* instr)
                        :iter_{iter}, instr_{instr}
                {
                        hv   = instr->get_vector();
                        hv_mask = hv.mask();

                        eval_.fill(0);
                }
                std::uint16_t make_mask(std::vector<ranking_t> const& R)const noexcept{
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

                        return mask;
                }
                void accept(mask_set const& ms, std::vector<ranking_t> const& R)noexcept
                {
                        size_t weight = ms.count_disjoint(hv_mask);
                        if( weight == 0 )
                                return;

                        auto mask = make_mask(R);

                        eval_[mask] += weight;

                }

                template<size_t Idx>
                __attribute__((__always_inline__))
                void prepare_intrinsic_3( std::vector<ranking_t> const& R,
                                         __m256i& v0,
                                         __m256i& v1,
                                         __m256i& v2)noexcept
                {
                        auto r0 = R[allocation_[0]];
                        auto r1 = R[allocation_[1]];
                        auto r2 = R[allocation_[2]];
                                
                        v0 = _mm256_insert_epi16(v0, r0, Idx);
                        v1 = _mm256_insert_epi16(v1, r1, Idx);
                        v2 = _mm256_insert_epi16(v2, r2, Idx);
                }
                template<size_t Idx>
                __attribute__((__always_inline__))
                void accept_intrinsic_3(std::vector<ranking_t> const& R,
                                        mask_set const& ms, 
                                        __m256i& masks)noexcept
                {
                        int mask = _mm256_extract_epi16(masks, Idx);
                        size_t weight = ms.count_disjoint(hv_mask);
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
        template<class SubPtrType>
        struct eval_scheduler_intrinsic_3{
                explicit eval_scheduler_intrinsic_3(size_t batch_size,
                                               std::vector<SubPtrType>& subs)
                        :batch_size_{batch_size}
                        ,subs_{subs}
                {
                        evals_.resize(batch_size_);

                }
                void begin_eval(mask_set const& ms)noexcept{
                        ms_   = &ms;
                        out_  = 0;
                }
                void put(size_t index, ranking_t rank)noexcept{
                        evals_[index] = rank;
                }
                void end_eval()noexcept{
                        BOOST_ASSERT( out_ == batch_size_ );

                        //#define DBG(REG, X) do{ std::cout << #REG "[" << (X) << "]=" << std::bitset<16>(_mm256_extract_epi16(REG,X)).to_string() << "\n"; }while(0)
                        #define DBG(REG, X) do{}while(0)

                        __m256i m0 = _mm256_set1_epi16(0b001);
                        __m256i m1 = _mm256_set1_epi16(0b010);
                        __m256i m2 = _mm256_set1_epi16(0b100);

                        size_t idx=0;
                        for(;idx + 16 < subs_.size();idx+=16){
                                __m256i v0 = _mm256_setzero_si256();
                                __m256i v1 = _mm256_setzero_si256();
                                __m256i v2 = _mm256_setzero_si256();

                                #if 0
                                for(size_t j=0;j!=8;++j){
                                        subs_[idx+j]->prepare_intrinsic_3(evals_, j, &v0, &v1, &v2);
                                }
                                #endif
                                subs_[idx+0]->template prepare_intrinsic_3<0>(evals_,  v0, v1, v2);
                                subs_[idx+1]->template prepare_intrinsic_3<1>(evals_,  v0, v1, v2);
                                subs_[idx+2]->template prepare_intrinsic_3<2>(evals_,  v0, v1, v2);
                                subs_[idx+3]->template prepare_intrinsic_3<3>(evals_,  v0, v1, v2);
                                subs_[idx+4]->template prepare_intrinsic_3<4>(evals_,  v0, v1, v2);
                                subs_[idx+5]->template prepare_intrinsic_3<5>(evals_,  v0, v1, v2);
                                subs_[idx+6]->template prepare_intrinsic_3<6>(evals_,  v0, v1, v2);
                                subs_[idx+7]->template prepare_intrinsic_3<7>(evals_,  v0, v1, v2);
                                subs_[idx+8 ]->template prepare_intrinsic_3<8>(evals_,  v0, v1, v2);
                                subs_[idx+9 ]->template prepare_intrinsic_3<9>(evals_,  v0, v1, v2);
                                subs_[idx+10]->template prepare_intrinsic_3<10>(evals_,  v0, v1, v2);
                                subs_[idx+11]->template prepare_intrinsic_3<11>(evals_,  v0, v1, v2);
                                subs_[idx+12]->template prepare_intrinsic_3<12>(evals_,  v0, v1, v2);
                                subs_[idx+13]->template prepare_intrinsic_3<13>(evals_,  v0, v1, v2);
                                subs_[idx+14]->template prepare_intrinsic_3<14>(evals_,  v0, v1, v2);
                                subs_[idx+15]->template prepare_intrinsic_3<15>(evals_,  v0, v1, v2);


                                __m256i r_min = _mm256_min_epi16(v0, _mm256_min_epi16(v1, v2));
                                __m256i eq0 =_mm256_cmpeq_epi16(r_min, v0);
                                __m256i eq1 =_mm256_cmpeq_epi16(r_min, v1);
                                __m256i eq2 =_mm256_cmpeq_epi16(r_min, v2);
                                __m256i a0 = _mm256_and_si256(eq0, m0);
                                __m256i a1 = _mm256_and_si256(eq1, m1);
                                __m256i a2 = _mm256_and_si256(eq2, m2);
                                __m256i mask = _mm256_or_si256(a0, _mm256_or_si256(a1, a2));
                                
                                #if 0
                                for(size_t j=0;j!=8;++j){
                                        subs_[idx+j]->accept_intrinsic_3(evals_, *ms_, j, &mask);
                                }
                                #endif
                                subs_[idx+ 0]->template accept_intrinsic_3< 0>(evals_, *ms_, mask);
                                subs_[idx+ 1]->template accept_intrinsic_3< 1>(evals_, *ms_, mask);
                                subs_[idx+ 2]->template accept_intrinsic_3< 2>(evals_, *ms_, mask);
                                subs_[idx+ 3]->template accept_intrinsic_3< 3>(evals_, *ms_, mask);
                                subs_[idx+ 4]->template accept_intrinsic_3< 4>(evals_, *ms_, mask);
                                subs_[idx+ 5]->template accept_intrinsic_3< 5>(evals_, *ms_, mask);
                                subs_[idx+ 6]->template accept_intrinsic_3< 6>(evals_, *ms_, mask);
                                subs_[idx+ 7]->template accept_intrinsic_3< 7>(evals_, *ms_, mask);
                                subs_[idx+ 8]->template accept_intrinsic_3< 8>(evals_, *ms_, mask);
                                subs_[idx+ 9]->template accept_intrinsic_3< 9>(evals_, *ms_, mask);
                                subs_[idx+10]->template accept_intrinsic_3<10>(evals_, *ms_, mask);
                                subs_[idx+11]->template accept_intrinsic_3<11>(evals_, *ms_, mask);
                                subs_[idx+12]->template accept_intrinsic_3<12>(evals_, *ms_, mask);
                                subs_[idx+13]->template accept_intrinsic_3<13>(evals_, *ms_, mask);
                                subs_[idx+14]->template accept_intrinsic_3<14>(evals_, *ms_, mask);
                                subs_[idx+15]->template accept_intrinsic_3<15>(evals_, *ms_, mask);

                                //std::exit(0);

                        }
                        for(;idx!=subs_.size();++idx){
                                subs_[idx]->accept(*ms_, evals_);
                        }
                }
                void regroup()noexcept{
                        // nop
                }
        private:
                size_t batch_size_;
                std::vector<ranking_t> evals_;
                std::vector<SubPtrType>& subs_;
                mask_set const* ms_;
                size_t out_{0};
        };

        template<class SubPtrType>
        struct eval_scheduler_batch_sort{
                // allocation -> group
                struct eval_group{
                        size_t offset;
                        std::vector<size_t> subs;
                };
                struct eval{
                        size_t group;
                        ranking_t rank;
                };
                explicit eval_scheduler_batch_sort(size_t batch_size,
                                               std::vector<SubPtrType>& subs)
                        :batch_size_{batch_size}
                        ,subs_{subs}
                {
                        groups_.resize(batch_size_);

                        size_t count = 0;
                        for(size_t idx=0;idx!=subs_.size();++idx){
                                std::unordered_set<size_t> S;
                                subs_[idx]->declare_allocation(S);
                                for(auto _ : S){
                                        groups_.at(_).subs.push_back(idx);
                                }
                                ++count;
                        }
                        PS_LOG(trace) << "made eval_scheduler_batch_sort";

                        for(size_t idx=0;idx!=groups_.size();++idx){
                                groups_[idx].offset = evals_proto_.size();
                                for(auto _ : groups_[idx].subs ){
                                        evals_proto_.emplace_back(eval{_, 0});
                                }
                        }
                        PS_LOG(trace) << "made eval_scheduler_batch_sort and evals_proto_";
                }

                void begin_eval(mask_set const& ms)noexcept{
                        ms_   = &ms;
                        out_  = 0;
                        evals_ = evals_proto_;
                }
                void put(size_t index, ranking_t rank)noexcept{
                        auto const& G = groups_[index];
                        for(size_t idx=0;idx!=G.subs.size();++idx){
                                evals_.at(G.offset + idx).rank  = rank;
                        }
                }
                void end_eval()noexcept{
                        BOOST_ASSERT( out_ == batch_size_ );

                        /*
                                What I want to do here is optmizize the evaluation.
                                For each subgroup ie,
                                        AsKs TsTc 6h8s,
                                we have to figure out the evaluation, which is one of
                                        {(0), (1), (2), (0,1), (0,2), (1,2), (1,2,3)},
                                which correspond to draws etc.
                                        I'm speculating that this can be optimized,
                                but trying to figure out every batch at once

                         */

                        boost::sort( evals_, [](auto const& l, auto const& r){
                                if( l.rank != r.rank )
                                        return l.rank < r.rank;
                                return l.group < r.group;
                        });

                        #if 0
                        return;
                        std::vector<unsigned> group_A(subs_.size(),0u);
                        std::vector<unsigned> group_B(subs_.size(),0u);
                        std::vector<std::array<unsigned, 3> > group_R(subs_.size());
                        #endif

                        auto emit_level_set = [&](unsigned first, unsigned last){
                                #if 0
                                for(auto iter=first;iter!=evals_.size();++iter){
                                        for(auto group : evals_[iter].groups ){
                                                // set already done
                                                if( group_A[group] != 0 )
                                                        continue;
                                                group_R[group][group_B[group]] = evals_[iter].index;
                                                ++group_B[group];
                                        }
                                }
                                for(auto iter=0;iter!=group_B.size();++iter){
                                        if( group_B[iter] == 0 )
                                                continue;

                                        subs_[iter]->accept_(*ms_, group_B[iter], group_R[iter] );

                                        group_A[iter] += group_B[iter];
                                }
                                #endif
                        };

                        size_t first=0;
                        for(size_t idx=1;idx!=evals_.size();++idx){
                                if( evals_[idx].rank != evals_[first].rank ){
                                        emit_level_set(first, idx);
                                        first = idx;
                                }
                        }
                        emit_level_set(first, evals_.size());
                }
                void regroup()noexcept{
                        // nop
                }
        private:
                size_t batch_size_;
                std::vector<SubPtrType>& subs_;
                mask_set const* ms_;
                size_t out_{0};
                std::vector<eval_group> groups_;
                std::vector<eval> evals_proto_;
                std::vector<eval> evals_;
        };
        















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


struct pass_eval_hand_instr_vec_impl{



        pass_eval_hand_instr_vec_impl(){
        }
        virtual void transform_dispatch(computation_context* ctx, instruction_list* instr_list, computation_result* result){
                std::vector<instruction_list::iterator> to_map;

                for(auto iter(instr_list->begin()),end(instr_list->end());iter!=end;++iter){
                        if( (*iter)->get_type() == instruction::T_CardEval ){
                                to_map.push_back(iter);
                        }
                }
                // short circuit
                if( to_map.empty())
                        return;
                // as an optimization we want to figure out if we have one size
                std::unordered_set<size_t> n_dist;

                for(auto iter : to_map){
                        auto instr = reinterpret_cast<card_eval_instruction*>((*iter).get());
                        n_dist.insert(instr->get_vector().size());
                }

                dispatch_context dctx;
                if( n_dist.size() == 1 )
                        dctx.homo_num_players = *n_dist.begin();

                std::shared_ptr<optimized_transform_base> ot;
                for(auto const& item : dispatch_table::world() ){
                        if( item->match(dctx) ){
                                boost::timer::cpu_timer tmr;
                                ot = item->make();
                                PS_LOG(trace) << "Took " << tmr.format(2, "%w seconds") << " to do make transform";
                                PS_LOG(trace) << "Using transform " << item->name();
                                break;
                        }
                }
                if( ! ot ){
                        PS_LOG(trace) << "no dispatch";
                } else {
                        boost::timer::cpu_timer tmr;
                        ot->apply(oct_, ctx, instr_list, result, to_map);
                        PS_LOG(trace) << "Took " << tmr.format(2, "%w seconds") << " to do main loop";
                }
        }
private:
        optimized_transform_context oct_;
};


pass_eval_hand_instr_vec::pass_eval_hand_instr_vec()
        : impl_{std::make_shared<pass_eval_hand_instr_vec_impl>()}
{}
void pass_eval_hand_instr_vec::transform(computation_context* ctx, instruction_list* instr_list, computation_result* result){
        impl_->transform_dispatch(ctx, instr_list, result);
}


} // end namespace ps
