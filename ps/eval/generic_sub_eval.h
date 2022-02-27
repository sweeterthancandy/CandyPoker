#ifndef LIB_EVAL_GENERIC_SUB_EVAL_H
#define LIB_EVAL_GENERIC_SUB_EVAL_H


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
                generic_sub_eval(iter_t iter, card_eval_instruction* instr)
                        :iter_{iter}, instr_{instr}
                {
                        hv   = instr->get_vector();
                        hv_mask = hv.mask();
                        n = hv.size();
                        mat.resize(n, n);
                        mat.fill(0);
                }
                size_t hand_mask()const noexcept{ return hv_mask; }
                template<class ArrayType>
                void accept_(mask_set const& ms, size_t n, ArrayType const& v){
                        size_t weight = ms.count_disjoint(hv_mask);
                        if( weight == 0 )
                                return;
                        for(size_t i=0;i!=n;++i){
                                mat(n,i) += weight;
                        }
                }
                void accept_weight(size_t weight, std::vector<ranking_t> const& R)noexcept
                {
                        for(size_t i=0;i!=n;++i){
                                ranked[i] = R[allocation_[i]];
                        }
                        detail::dispatch_ranked_vector_mat(mat, ranked, n, weight);
                }
                void finish(){
                        *iter_ = std::make_shared<matrix_instruction>(instr_->result_desc(), mat);
                }
                void declare(std::unordered_set<holdem_id>& S){
                        for(auto _ : hv){
                                S.insert(_);
                        }
                }
                void declare_allocation(std::unordered_set<size_t>& S){
                        for(size_t idx=0;idx!=n;++idx){
                                S.insert(allocation_[idx]);
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
                std::array<ranking_t, 9> ranked;
                holdem_hand_vector hv;
                size_t hv_mask;
                matrix_t mat;
                size_t n;
                std::array<size_t, 9> allocation_;
        };

} // end namespace ps

#endif // LIB_EVAL_GENERIC_SUB_EVAL_H
