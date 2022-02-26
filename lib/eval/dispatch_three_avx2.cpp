#include "lib/eval/dispatch_table.h"
#include "lib/eval/generic_shed.h"
#include "lib/eval/generic_sub_eval.h"
#include "lib/eval/optimized_transform.h"


#include <boost/predef.h>

#if BOOST_OS_LINUX
    #define PS_ALWAYS_INLINE __attribute__((__always_inline__))
#else
    #define PS_ALWAYS_INLINE
#endif


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
                size_t hand_mask()const noexcept{ return hv_mask; }
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
                void accept_weight(size_t weight, std::vector<ranking_t> const& R)noexcept
                {

                        auto mask = make_mask(R);

                        eval_[mask] += weight;

                }

                template<size_t Idx>
                PS_ALWAYS_INLINE
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
                PS_ALWAYS_INLINE
                void accept_intrinsic_3(size_t weight,
                                        std::vector<ranking_t> const& R,
                                        __m256i& masks)noexcept
                {
                        int mask = _mm256_extract_epi16(masks, Idx);
                        eval_[mask] += weight;
                }

                void finish(){
                        matrix_t mat;
                        mat.resize(n, n);
                        mat.fill(0);
                        for(int mask = 1; mask != UpperMask; ++mask){
                                auto pcnt = detail::popcount(mask);
                                if( mask & 0b001 )
                                        mat(pcnt-1, 0) += eval_[mask];
                                if( mask & 0b010 )
                                        mat(pcnt-1, 1) += eval_[mask];
                                if( mask & 0b100 )
                                        mat(pcnt-1, 2) += eval_[mask];
                        }
                        *iter_ = std::make_shared<matrix_instruction>(instr_->result_desc(), mat);
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
                enum{ CheckUnion = true };
                enum{ CheckZeroWeight = false };
                template<class SubPtrType>
                struct bind{
                        explicit bind(size_t batch_size, std::vector<SubPtrType>& subs)
                                :subs_{subs}
                        {
                                evals_.resize(batch_size);

                        }
                        void put(size_t index, ranking_t rank)noexcept{
                                evals_[index] = rank;
                        }
                        void end_eval(mask_set const* ms, size_t single_mask)noexcept{

                                //#define DBG(REG, X) do{ std::cout << #REG "[" << (X) << "]=" << std::bitset<16>(_mm256_extract_epi16(REG,X)).to_string() << "\n"; }while(0)
                                #define DBG(REG, X) do{}while(0)

                                __m256i m0 = _mm256_set1_epi16(0b001);
                                __m256i m1 = _mm256_set1_epi16(0b010);
                                __m256i m2 = _mm256_set1_epi16(0b100);

                                #define FOR_EACH(X) \
                                do {\
                                        X(0);\
                                        X(1);\
                                        X(2);\
                                        X(3);\
                                        X(4);\
                                        X(5);\
                                        X(6);\
                                        X(7);\
                                        X(8);\
                                        X(9);\
                                        X(10);\
                                        X(11);\
                                        X(12);\
                                        X(13);\
                                        X(14);\
                                        X(15);\
                                }while(0)


                                size_t idx=0;
                                for(;idx + 16 < subs_.size();idx+=16){
                                        __m256i v0 = _mm256_setzero_si256();
                                        __m256i v1 = _mm256_setzero_si256();
                                        __m256i v2 = _mm256_setzero_si256();

                                        #define PREPARE(N)                                                                 \
                                                do{                                                                        \
                                                        subs_[idx+N]->template prepare_intrinsic_3<N>(evals_,  v0, v1, v2);\
                                                }while(0)

                                        FOR_EACH(PREPARE);


                                        __m256i r_min = _mm256_min_epi16(v0, _mm256_min_epi16(v1, v2));
                                        __m256i eq0 =_mm256_cmpeq_epi16(r_min, v0);
                                        __m256i eq1 =_mm256_cmpeq_epi16(r_min, v1);
                                        __m256i eq2 =_mm256_cmpeq_epi16(r_min, v2);
                                        __m256i a0 = _mm256_and_si256(eq0, m0);
                                        __m256i a1 = _mm256_and_si256(eq1, m1);
                                        __m256i a2 = _mm256_and_si256(eq2, m2);
                                        __m256i mask = _mm256_or_si256(a0, _mm256_or_si256(a1, a2));

                                        #define FINALIZE(N)                                                             \
                                                do{                                                                     \
                                                        size_t weight = generic_weight_policy{} \
                                                        .calculate( subs_[idx+ N]->hand_mask(), ms, single_mask);        \
                                                        if( CheckZeroWeight ){\
                                                                if( weight == 0 ) break ; \
                                                        }\
                                                        subs_[idx+ N]->template accept_intrinsic_3< N>(weight, evals_, mask);\
                                                }while(0)

                                        FOR_EACH(FINALIZE);

                                        
                                }
                                for(;idx!=subs_.size();++idx){
                                        size_t weight = generic_weight_policy{}
                                                .calculate(subs_[idx]->hand_mask(), ms, single_mask);
                                        subs_[idx]->accept_weight(weight, evals_);
                                }
                        }
                        void regroup()noexcept{
                                // nop
                        }
                private:
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
        
        
        
        
        
        
        struct scheduler_intrinsic_three_avx2_opt{
                enum{ CheckUnion = true };
                template<class SubPtrType>
                struct bind{
                        explicit bind(size_t batch_size, std::vector<SubPtrType>& subs)
                                :subs_{subs}
                        {
                                evals_.resize(batch_size);

                        }
                        void put(size_t index, ranking_t rank)noexcept{
                                evals_[index] = rank;
                        }
                        void end_eval(mask_set const* ms, size_t single_mask)noexcept{

                                //#define DBG(REG, X) do{ std::cout << #REG "[" << (X) << "]=" << std::bitset<16>(_mm256_extract_epi16(REG,X)).to_string() << "\n"; }while(0)
                                #define DBG(REG, X) do{}while(0)


                                #define FOR_EACH(X) \
                                do {\
                                        X(0);\
                                        X(1);\
                                        X(2);\
                                        X(3);\
                                        X(4);\
                                        X(5);\
                                        X(6);\
                                        X(7);\
                                        X(8);\
                                        X(9);\
                                        X(10);\
                                        X(11);\
                                        X(12);\
                                        X(13);\
                                        X(14);\
                                        X(15);\
                                }while(0)


                                size_t head=0; 
                                size_t tail=0;

                                std::array<decltype(std::declval<SubPtrType>().get()), 16> batch;
                                std::array<size_t, 16> batch_weight;
                                
                                __m256i m0 = _mm256_set1_epi16(0b001);
                                __m256i m1 = _mm256_set1_epi16(0b010);
                                __m256i m2 = _mm256_set1_epi16(0b100);

                                for(; head != subs_.size();tail=head){
                                        __m256i v0 = _mm256_setzero_si256();
                                        __m256i v1 = _mm256_setzero_si256();
                                        __m256i v2 = _mm256_setzero_si256();

                                        #define SCATTER(N)                                                                 \
                                                do{                                                                        \
                                                        for(;head!=subs_.size() && count != 16;++head){\
                                                                size_t weight = generic_weight_policy{} \
                                                                        .calculate( subs_[head]->hand_mask(), ms, single_mask);        \
                                                                if( weight == 0 ) continue; \
                                                                batch_weight[N] = weight;\
                                                                batch[N] = subs_[head].get();\
                                                                batch[N]->template prepare_intrinsic_3<N>(evals_,  v0, v1, v2);\
                                                                ++count;\
                                                                ++head;\
                                                                break;\
                                                        }\
                                                }while(0)

                                        size_t count = 0;
                                        FOR_EACH(SCATTER);
                                        // no edge cases
                                        if( count != 16 )
                                                break;


                                        __m256i r_min = _mm256_min_epi16(v0, _mm256_min_epi16(v1, v2));
                                        __m256i eq0 =_mm256_cmpeq_epi16(r_min, v0);
                                        __m256i eq1 =_mm256_cmpeq_epi16(r_min, v1);
                                        __m256i eq2 =_mm256_cmpeq_epi16(r_min, v2);
                                        __m256i a0 = _mm256_and_si256(eq0, m0);
                                        __m256i a1 = _mm256_and_si256(eq1, m1);
                                        __m256i a2 = _mm256_and_si256(eq2, m2);
                                        __m256i mask = _mm256_or_si256(a0, _mm256_or_si256(a1, a2));

                                        #define GATHER(N)                                                               \
                                                do{                                                                     \
                                                        batch[N]->template accept_intrinsic_3< N>(batch_weight[N], evals_, mask);\
                                                }while(0)

                                        FOR_EACH(GATHER);

                                        
                                }
                                for(;tail!=subs_.size();++tail){
                                        size_t weight = generic_weight_policy{}
                                                .calculate(subs_[tail]->hand_mask(), ms, single_mask);
                                        subs_[tail]->accept_weight(weight, evals_);
                                }
                        }
                        void regroup()noexcept{
                                // nop
                        }
                private:
                        std::vector<ranking_t> evals_;
                        std::vector<SubPtrType>& subs_;
                        mask_set const* ms_;
                        size_t out_{0};
                };
        };
        
        struct dispatch_three_player_avx2_opt : dispatch_table{
                using transform_type =
                        optimized_transform<
                        sub_eval_three_intrinsic_avx2,
                        scheduler_intrinsic_three_avx2_opt,
                        basic_sub_eval_factory,
                        rank_hash_eval>;


                virtual bool match(dispatch_context const& dispatch_ctx)const override{
                        return dispatch_ctx.homo_num_players &&  dispatch_ctx.homo_num_players.get() == 3;
                }
                virtual std::shared_ptr<optimized_transform_base> make()const override{
                        return std::make_shared<transform_type>();
                }
                virtual std::string name()const override{
                        return "three-player-avx2-opt";
                }
                virtual size_t precedence()const override{ return 102; }
        };
        static register_disptach_table<dispatch_three_player_avx2_opt> reg_dispatch_three_player_avx2_opt;
} // end namespace ps
