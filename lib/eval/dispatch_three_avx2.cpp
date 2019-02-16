#include "lib/eval/dispatch_table.h"
#include "lib/eval/generic_shed.h"
#include "lib/eval/generic_sub_eval.h"
#include "lib/eval/optimized_transform.h"

namespace ps{
        // These are actually slower, hard to beat a good compiler

        struct sub_eval_three_intrinsic_avx2{
                enum{ UpperMask = 0b111 + 1 };
                using iter_t = instruction_list::iterator;
                sub_eval_three_intrinsic_avx2(iter_t iter, card_eval_instruction* instr)
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
        struct scheduler_intrinsic_three_avx2{
                template<class SubPtrType>
                struct bind{
                        explicit bind(size_t batch_size, std::vector<SubPtrType>& subs)
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
        };
        
        struct dispatch_three_player_avx2 : dispatch_table{
                using transform_type =
                        optimized_transform<
                        sub_eval_three_intrinsic_avx2,
                        scheduler_intrinsic_three_avx2,
                        basic_sub_eval_factory,
                        rank_hash_eval>;


                virtual bool match(dispatch_context const& dispatch_ctx)const override{
                        return dispatch_ctx.homo_num_players &&  dispatch_ctx.homo_num_players.get() == 3;
                }
                virtual std::shared_ptr<optimized_transform_base> make()const override{
                        return std::make_shared<transform_type>();
                }
                virtual std::string name()const override{
                        return "three-player-avx2";
                }
                virtual size_t precedence()const override{ return 101; }
        };
        static register_disptach_table<dispatch_three_player_avx2> reg_dispatch_three_player_avx2;
} // end namespace ps
