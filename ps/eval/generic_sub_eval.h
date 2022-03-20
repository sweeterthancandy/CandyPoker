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
                for(size_t player_index=1;player_index<n;++player_index){
                        if( ranked(player_index) == lowest ){
                                ++count;
                                ptr = nullptr;
                        } else if( ranked(player_index) < lowest ){
                                lowest = ranked(player_index); 
                                count = 1;
                                ptr = &result(player_index, 0);
                        }
                }
                if (ptr)
                {
                        *ptr += weight;
                }
                else
                {
                        const size_t draw_index = count-1;
                        for(size_t player_index=0;player_index!=n;++player_index){
                                if( ranked(player_index) == lowest ){
                                        
                                        result(player_index, draw_index) += weight;
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
                        auto const& typed_hv = instr->get_vector();
                        hv.resize(typed_hv.size());
                        for(size_t idx=0;idx!=typed_hv.size();++idx)
                        {
                                hv[idx] = typed_hv[idx];
                        }
                        hv_mask = typed_hv.mask();
                        mat.resize(typed_hv.size(), typed_hv.size());
                        mat.fill(0);
                }
                size_t hand_mask()const noexcept{ return hv_mask; }
                
                void accept_weight(size_t weight, std::vector<ranking_t> const& R)noexcept
                {

                        const auto access = [&R,this](size_t idx){ return R[hv[idx]];};
                        detail::dispatch_ranked_vector_mat_func(mat, access, hv.size(), weight);
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
                boost::container::small_vector<holdem_id, 5> hv;
                size_t hv_mask;
                matrix_t mat;
        };

        template<size_t Size>
        struct sized_sub_eval{
                using iter_t = instruction_list::iterator;
                sized_sub_eval()=default;
                sized_sub_eval(iter_t iter, card_eval_instruction* instr)
                {
                        auto const& typed_hv = instr->get_vector();
                        for(size_t idx=0;idx!=typed_hv.size();++idx)
                        {
                                hv[idx] = typed_hv[idx];
                        }
                        hv_mask = typed_hv.mask();
                        //mat.resize(hv.size(), hv.size());
                        mat.fill(0);
                }
                size_t hand_mask()const noexcept{ return hv_mask; }
                
                void accept_weight(size_t weight, std::vector<ranking_t> const& R)noexcept
                {

                        const auto access = [&R,this](size_t idx){ return R[hv[idx]];};
                        detail::dispatch_ranked_vector_mat_func(mat, access, hv.size(), weight);
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
                std::array<holdem_id, Size> hv;
                size_t hv_mask;
                Eigen::Matrix< unsigned long long , Size,Size> mat;
        };

} // end namespace ps

#endif // LIB_EVAL_GENERIC_SUB_EVAL_H
