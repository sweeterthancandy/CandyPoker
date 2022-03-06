#ifndef LIB_EVAL_GENERIC_SUB_EVAL_H
#define LIB_EVAL_GENERIC_SUB_EVAL_H

#include <boost/container/small_vector.hpp>

namespace ps{
namespace detail{
        template<class MatrixType, class ArrayType>
        void dispatch_ranked_vector_mat_func(MatrixType& result, ArrayType const& ranked, size_t n, size_t weight = 1)noexcept{
                auto lowest = ranked(0) ;
                size_t count{1};
                auto ptr = &result(0, 0);
                for(size_t i=1;i<n;++i){
                        if( ranked(i) == lowest ){
                                ++count;
                                ptr = nullptr;
                        } else if( ranked(i) < lowest ){
                                lowest = ranked(i); 
                                count = 1;
                                ptr = &result(0, i);
                        }
                }
                if (ptr)
                {
                        *ptr += weight;
                }
                else
                {
                        for(size_t i=0;i!=n;++i){
                                if( ranked(i) == lowest ){
                                        result(count-1, i) += weight;
                                }
                        }
                }
               
        }
} // detail
} // ps


namespace ps{

        /*
                generic_sub_eval represents a single hand vs hand evaluations.
                The idea is that we have a set of evaluations to perform similtanously,
                ie rather than do 
                        for every eval
                                for eval possible board,
                we do
                        for eval possible board
                                for eval eval

                This allows us to do optimizations where cards are shared, for 
                example
                                KcKs sv AhQh
                                KcKs sv AhQd,
                we implement this by three evalutions for each board
                                {board \union KcKs,
                                 board \union AhQh,
                                 board \union AhQd},
                and we save evalutions, we map map
                        board \union KcKs -> integer.


         */
        struct generic_sub_eval{
                using iter_t = instruction_list::iterator;
                generic_sub_eval()=default;
                generic_sub_eval(iter_t iter, card_eval_instruction* instr)
                {
                        for(auto hid : instr->get_vector())
                        {
                                hv.push_back(hid);
                        }
                        hv_mask = instr->get_vector().mask();
                        mat.resize(hv.size(), hv.size());
                        mat.fill(0);
                }
                size_t hand_mask()const noexcept{ return hv_mask; }
                
                void accept_weight(size_t weight, std::vector<ranking_t> const& R)noexcept
                {
#if 0
                        for(size_t i=0;i!=n;++i){
                                ranked[i] = R[allocation_[i]];
                        }
                        detail::dispatch_ranked_vector_mat(mat, ranked, n, weight);
#else

                        const auto access = [&R,this](size_t idx){ return R[hv[idx]];};
                        detail::dispatch_ranked_vector_mat_func(mat, access, hv.size(), weight);
#endif
                }
                void finish(iter_t iter, card_eval_instruction* instr){
                        *iter = std::make_shared<matrix_instruction>(instr->result_desc(), mat);
                }
                void declare(std::unordered_set<holdem_id>& S){
                        for(auto _ : hv){
                                S.insert(_);
                        }
                }
        private:
                //iter_t iter_;
                //card_eval_instruction* instr_;
                //holdem_hand_vector hv;
                boost::container::small_vector<holdem_id, 5> hv;
                size_t hv_mask;
                matrix_t mat;
        };

} // end namespace ps

#endif // LIB_EVAL_GENERIC_SUB_EVAL_H
