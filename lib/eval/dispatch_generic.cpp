#include "ps/eval/dispatch_table.h"
#include "ps/eval/generic_shed.h"
#include "ps/eval/generic_sub_eval.h"
#include "ps/eval/optimized_transform.h"

namespace ps{


struct dispatch_generic : dispatch_table{
        using transform_type =
                optimized_transform<
                        generic_sub_eval,
                        generic_shed,
                        basic_sub_eval_value_factory>;


        virtual bool match(dispatch_context const& dispatch_ctx)const override{
                return true;
        }
        virtual std::shared_ptr<optimized_transform_base> make()const override{
                return std::make_shared<transform_type>();
        }
        virtual std::string name()const override{
                return "generic";
        }
};
static register_disptach_table<dispatch_generic> reg_dispatch_generic;

















#if 0

template<class MapType>
inline std::string std_map_to_string(MapType const& m) {
        std::stringstream sstr;
        std::string sep;
        sstr << "{";
        for(auto const& p : m)
        {
                sstr << sep << p.first << "=>" << p.second;

        sep = ",";
        }
        sstr << "}";
        return sstr.str();
}

template<class MapType, class ValueFormat>
inline std::string std_map_to_string(MapType const& m, ValueFormat const& vfmt) {
        std::stringstream sstr;
        std::string sep;
        sstr << "{";
        for(auto const& p : m)
        {
                sstr << sep << p.first << "=>" << vfmt(p.second);

        sep = ",";
        }
        sstr << "}";
        return sstr.str();
}




struct fast_sub_eval{
        using iter_t = instruction_list::iterator;
        fast_sub_eval(iter_t iter, card_eval_instruction* instr)
                :iter_{iter}, instr_{instr}
        {
                hv   = instr->get_vector();
                hv_mask = hv.mask();
                n = hv.size();
                mat.resize(n, n);
                mat.fill(0);
        }
        size_t hand_mask()const noexcept{ return hv_mask; }

        holdem_hand_vector get_hand_vec()const{ return hv; }

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
        template<class Alloc>
        void allocate(Alloc const& alloc){
                for(size_t idx=0;idx!=n;++idx){
                        allocation_[idx] = alloc(hv[idx]);
                        inv_map_[allocation_[idx]] = idx;
                }
        }

        void commit(size_t weight, std::vector<size_t> const& level_set)
        {
                //PS_LOG(trace) << std_vector_to_string(level_set) << "  " << std_map_to_string(inv_map_,[](auto id){ return holdem_class_decl::get(id).to_string();});
                const auto col = level_set.size() -1;
                for(size_t j=0;j!=level_set.size();++j){
                        mat(col, inv_map_[level_set[j]]) += weight;
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
        std::unordered_map<size_t,size_t> inv_map_;
};

 struct fast_shed{

        using weights_ty = std::vector<eval_counter_type>;
        template<class SubPtrType>
        struct bind{

                struct dispatch_buffer
                {
                        dispatch_buffer(SubPtrType const& ptr):ptr_{ptr}{}
                        void clear(){
                                buf_.clear();
                                already_committed_ = false;
                        }
                        void push(size_t idx){
                                buf_.push_back(idx);
                        }
                        void commit(size_t weight)
                        {
                                if (already_committed_)
                                {
                                        return;
                                }
                                ptr_->commit(weight, buf_);
                                already_committed_ = true;
                        }
                        auto ptr(){ return ptr_; }
                        bool already_committed()const{ return already_committed_; }
                private:
                        SubPtrType ptr_;
                        bool already_committed_{false};
                        std::vector<size_t> buf_;
                };

                // this is 5% faster (in my limited testing)
                enum{ CheckZeroWeight = true };
                explicit bind(holdem_hand_vector const& allocation, std::vector<SubPtrType>& subs)
                        :subs_{subs}
                {

                        for (size_t idx = 0; idx != allocation.size(); ++idx)
                        {
                                evals_proto_.emplace_back(static_cast<ranking_t>(-1),idx);
                        }

                        evals_ = evals_proto_;


                        
                        std::unordered_map<size_t,size_t> hid_to_index;
                        for(size_t idx=0;idx!=allocation.size();++idx)
                        {
                                hid_to_index[allocation[idx]] = idx;
                        }


                        dispatch_table_.resize(allocation.size());
                        
                        for (auto const& ptr : subs_)
                        {
                                auto dbobj = std::make_shared<dispatch_buffer>(ptr);
                                memory_.push_back(dbobj);
                                for (auto const& hid : ptr->get_hand_vec())
                                {
                                        auto mapped_hid = hid_to_index[hid];
                                        dispatch_table_[mapped_hid].push_back(dbobj.get());
                                }
                        }

                }
                void put(size_t index, ranking_t rank)noexcept{
                        std::get<0>(evals_[index]) = rank;
                }

                void end_eval(mask_set const* ms, size_t single_mask)noexcept{
                        (void)single_mask;

                        std::sort(
                                std::begin(evals_),
                                std::end(evals_),
                                [](auto const& l, auto const& r) {
                                        return std::get<0>(l) < std::get<0>(r);
                                });

                        size_t iter = 0;
                        size_t end = evals_.size();

                        size_t commit_count = 0;

                        for (;iter!=end;)
                        {
                                const auto current_level = std::get<0>(evals_[iter]);
                                auto const mid = iter;

                                step_seen_.clear();
                                for (;iter!=end;++iter)
                                {
                                        if (std::get<0>(evals_[iter]) != current_level)
                                        {
                                                break;
                                        }
                                        const auto index = std::get<1>(evals_[iter]);
                                        for (auto ptr : dispatch_table_[index])
                                        {
                                                if (ptr->already_committed())
                                                {
                                                        continue;
                                                }
                                                ptr->push(std::get<1>(evals_[iter]));
                                                step_seen_.push_back(ptr);
                                        }
                                }
                                

                                for (auto ptr : step_seen_)
                                {
                                        if (ptr->already_committed())
                                        {
                                                continue;
                                        }
                                        ++commit_count;
                                        auto weight = generic_weight_policy{}
                                                .calculate(ptr->ptr()->hand_mask(), *ms);
                                        ptr->commit(weight);
                                }

                                if (commit_count == memory_.size())
                                {
                                        break;

                                }

                        }

                        for (auto& ptr : memory_)
                        {
                                ptr->clear();
                        }

                        evals_ = evals_proto_;

                        static bool first = true;
                        if (first)
                        {
                                first = false;
                                std::cout << "Aaaaa\n";
                        }
                }

              

                void regroup()noexcept{
                        // nop
                }
        private:
                std::vector<std::tuple<ranking_t,size_t> > evals_proto_;
                std::vector<std::tuple<ranking_t,size_t> > evals_;
                std::vector<SubPtrType>& subs_;
                std::vector<std::shared_ptr<dispatch_buffer> > memory_;
                std::vector<std::vector<dispatch_buffer*> > dispatch_table_;
                std::vector<dispatch_buffer*> step_seen_;
        };
};



struct dispatch_fast : dispatch_table{
        using transform_type =
                optimized_transform<
                        fast_sub_eval,
                        fast_shed,
                        basic_sub_eval_factory,
                        rank_hash_eval>;

        virtual size_t precedence()const{ return 100; }

        virtual bool match(dispatch_context const& dispatch_ctx)const override{
                return true;
        }
        virtual std::shared_ptr<optimized_transform_base> make()const override{
                return std::make_shared<transform_type>();
        }
        virtual std::string name()const override{
                return "fast";
        }
};
static register_disptach_table<dispatch_fast> reg_dispatch_fast;

#endif


} // end namespace ps
