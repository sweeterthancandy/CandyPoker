#include "ps/eval/pass.h"

namespace ps{

void instruction_map_pass::transform(computation_context* ctx, instruction_list* instr_list, computation_result* result){
        for(auto iter(instr_list->begin()),end(instr_list->end());iter!=end;){
                auto ret = try_map_instruction(ctx, &**iter);

                if( ! ret ){
                        ++iter;
                } else {
                        for(auto& _ : *ret){
                                instr_list->insert(iter, _);
                        }
                        auto tmp = iter;
                        ++iter;
                        instr_list->erase(tmp);
                }
                #if 0
                static int counter = 0;
                if( counter == 1 )
                        return;
                ++counter;
                #endif
        }
}

void pass_permutate::transform(computation_context* ctx, instruction_list* instr_list, computation_result* result){
        for(auto& instr : *instr_list){
                if( instr->get_type() != instruction::T_CardEval )
                        continue;
                auto ptr = reinterpret_cast<card_eval_instruction*>(instr.get());
                auto const& vec = ptr->get_vector();
                auto result = permutate_for_the_better(vec);

                if( std::get<1>(result) == vec )
                        continue;

                auto const& perm = std::get<0>(result);

                std::vector<result_description> permed_result_desc = result_description::apply_perm(ptr->result_desc(), perm);

                instr = std::make_shared< card_eval_instruction>(permed_result_desc, std::get<1>(result));

        }
}


 void pass_sort_type::transform(computation_context* ctx, instruction_list* instr_list, computation_result* result){
        instr_list->sort( [](auto l, auto r){
                if( l->get_type() != r->get_type())
                        return l->get_type() != r->get_type();
                switch(l->get_type()){
                case instruction::T_CardEval:
                        {
                                auto lt = reinterpret_cast<card_eval_instruction*>(l.get());
                                auto rt = reinterpret_cast<card_eval_instruction*>(r.get());
                                return std::tie(lt->get_vector()) < std::tie(rt->get_vector());
                        }
                        break;
                }
                return false;
        });
}

 void pass_print::transform(computation_context* ctx, instruction_list* instr_list, computation_result* result){
        std::cout << "--------BEGIN----------\n";
        for(auto instr : *instr_list ){
                std::cout << instr->to_string() << "\n";
        }
        std::cout << "---------END-----------\n";
}

 void pass_permutate_class::transform(computation_context* ctx, instruction_list* instr_list, computation_result* result){
        for(auto& instrr : *instr_list){
                if( instrr->get_type() != instruction::T_ClassEval )
                        continue;
                auto mem = instrr;
                auto instr = reinterpret_cast<class_eval_instruction*>(instrr.get());
                auto const& vec = instr->get_vector();
                auto result = vec.to_standard_form();
                        
                if( std::get<1>(result) == vec )
                        continue;

                auto const& perm = std::get<0>(result);

                std::vector<result_description> permed_result_desc = result_description::apply_perm(instr->result_desc(), perm);

                instrr = std::make_shared< class_eval_instruction>(permed_result_desc, std::get<1>(result));
        }
 }

 boost::optional<instruction_list> pass_class2cards::try_map_instruction(computation_context* ctx, instruction* instrr){
        if( instrr->get_type() != instruction::T_ClassEval)
                return boost::none;
        auto instr = reinterpret_cast<class_eval_instruction*>(instrr);
        auto vec = instr->get_vector();

        
        instruction_list result;
        //PS_LOG(trace) << "vec => " << vec;
        for( auto hv : vec.get_hand_vectors()){
                //PS_LOG(trace) << "  hv => " << hv;
                result.push_back(std::make_shared<card_eval_instruction>(instr->result_desc(), hv));
        }
        return result;
#if 0
        auto const n = vec.size();
        std::map<holdem_hand_vector, matrix_t  > meta;

        for( auto hv : vec.get_hand_vectors()){

                auto p =  permutate_for_the_better(hv) ;
                auto& perm = std::get<0>(p);
                auto const& perm_players = std::get<1>(p);

                if( meta.count(perm_players) == 0 ){
                        meta[perm_players] = matrix_t{n,n};
                        meta[perm_players].fill(0);
                }
                auto& item = meta.find(perm_players)->second;
                for(int i=0;i!=n;++i){
                        ++item(i,perm[i]);
                }
        }
                
                
        instruction_list result;
        for(auto const& _ : meta){
                std::vector<result_description> step_result_desc = result_description::apply_matrix(
                        instr->result_desc(), 
                        _.second);
                result.push_back(std::make_shared<card_eval_instruction>(step_result_desc, _.first));
        }
        return result;
#endif
}




 


 void pass_write::transform(computation_context* ctx, instruction_list* instr_list, computation_result* result){
                for(auto instr : *instr_list ){
                        if( instr->get_type() != instruction::T_Matrix ){
                                result->set_error("not matrix");
                                return;
                        }
                }
                for(auto instr_ : *instr_list ){
                        auto instr = reinterpret_cast<matrix_instruction*>(instr_.get());
                        auto const& result_desc = instr->result_desc();
                        for (auto const& output_target : result_desc)
                        {
                            result->allocate_tag(output_target.group()) += output_target.transform() * instr->result_matrix();
                        }      
                }
        }


 struct holdem_class_hard_from_proto
{
        using match_ty = std::tuple<holdem_class_type, rank_id, rank_id>;
        using rank_mapping_ty = std::array<int, rank_decl::max_id>;

        static std::string pattern_to_string(std::vector<match_ty> const& pattern)
        {
                std::stringstream ss;
                ss << "<";
                std::string sep = "";
                for(auto const& m : pattern)
                {
                        switch (std::get<0>(m))
                        {
                        case holdem_class_type::pocket_pair:
                                ss << "(pp," << rank_decl::get(std::get<1>(m)).to_string() << ")" << sep;
                                break;
                        case holdem_class_type::suited:
                                ss << "(suited," << rank_decl::get(std::get<1>(m)).to_string() 
                                        << "," << rank_decl::get(std::get<2>(m)).to_string() << ")" << sep;
                                break;
                        case holdem_class_type::offsuit:
                                ss << "(offsuit," << rank_decl::get(std::get<1>(m)).to_string() 
                                        << "," << rank_decl::get(std::get<2>(m)).to_string() << ")" << sep;
                        }
                        sep = ",";
                }
                ss << ">";
                return ss.str();
        }

        struct prototype
        {
                prototype(
                        std::vector< holdem_hand_vector > const& hv,
                        std::vector< matrix_t > const& transform)
                        : hv_(hv), transform_(transform)
                {}
                void emit(
                        holdem_class_vector const& cv,
                        rank_mapping_ty const& mapping,
                        std::vector< holdem_hand_vector >& out,
                        std::vector<matrix_t>& transform)const
                {
                        holdem_hand_vector working_hv;
                        for (size_t idx = 0; idx != hv_.size(); ++idx)
                        {
                                working_hv.clear();
                                for (holdem_id hid : hv_[idx])
                                {
                                        auto const& hand = holdem_hand_decl::get(hid);

                                        auto mapped_rank_first = mapping[hand.first().rank()];
                                        auto mapped_rank_second = mapping[hand.second().rank()];

                                        auto mapped_first = card_decl::make_id(
                                                hand.first().suit(),
                                                mapped_rank_first);

                                        auto mapped_second = card_decl::make_id(
                                                hand.second().suit(),
                                                mapped_rank_second);

                                        auto mapped_hid = holdem_hand_decl::make_id(mapped_first, mapped_second);

                                        working_hv.push_back(mapped_hid);

                                }

                                out.push_back(working_hv);
                                transform.push_back(transform_[idx]);
                        }
                }
                std::vector< holdem_hand_vector > hv_;
                std::vector< matrix_t > transform_;
        };
        void populate(holdem_class_vector const& cv, std::vector< holdem_hand_vector >& out, std::vector<matrix_t>& transform)
        {
                auto p = make_pattern(cv);


                auto const& pattern = std::get<0>(p);

                //std::cout << cv << " => " << pattern_to_string(pattern) << "\n";

                // mapping is cv rank to standard rank
                auto const& mapping = std::get<1>(p);
                auto const& inv_mapping = std::get<2>(p);

                auto iter = prototype_map_.find(pattern);

                if (iter == prototype_map_.end())
                {
                        //std::cout << "MAKING\n";


                        holdem_class_vector mapped_cv;
                        for (auto const& cid : cv)
                        {
                                auto const& decl = holdem_class_decl::get(cid);

                                auto mapped_cid = holdem_class_decl::make_id(
                                        decl.category(),
                                        mapping[decl.first()],
                                        mapping[decl.second()]);
  
                                mapped_cv.push_back(mapped_cid);
                        }

                        instruction_list instr_list;
                        instr_list.push_back(std::make_shared<class_eval_instruction>("DummyGroup", mapped_cv));


                        computation_pass_manager mgr;
                        mgr.add_pass<pass_permutate_class>();
                        mgr.add_pass<pass_collect_class>();
                        mgr.add_pass<pass_class2cards>();
                        mgr.add_pass<pass_permutate>();
                        mgr.add_pass<pass_sort_type>();
                        mgr.add_pass<pass_collect>();

                        const size_t common_size = cv.size();
                        const int verboseicity = 0;
                        auto comp_ctx = std::make_shared< computation_context>(common_size, verboseicity);
                        mgr.execute_(comp_ctx.get(), &instr_list, nullptr);

                        std::vector<holdem_hand_vector> agg_hv;
                        std::vector<matrix_t> agg_trans;

                        for (auto const& untyped_instr : instr_list)
                        {
                                auto instr = reinterpret_cast<const card_eval_instruction*>(untyped_instr.get());

                                agg_hv.push_back(instr->get_vector());
                                agg_trans.push_back(instr->result_desc().back().transform());

                        }

                        prototype_map_[pattern] = std::make_shared<prototype>(agg_hv, agg_trans);
                }
                return prototype_map_.find(pattern)->second->emit(cv, inv_mapping, out, transform);
        }

        std::tuple<std::vector<match_ty>,rank_mapping_ty,rank_mapping_ty > make_pattern(holdem_class_vector const& cv)const
        {
                rank_mapping_ty rank_mapping;
                rank_mapping.fill(-1);
                rank_id rank_alloc(rank_decl::max_id);
                auto map_rank = [&rank_mapping,&rank_alloc](rank_id rid)mutable->rank_id
                {
                        if(rank_mapping[rid] == -1)
                        {
                                --rank_alloc;
                                const rank_id mapped_rank = rank_alloc;
                                rank_mapping[rid] = mapped_rank;
                                return mapped_rank;
                        }
                        else
                        {
                                return rank_mapping[rid];
                        }
                };

                std::vector<match_ty> pattern;
                for(holdem_class_id cid : cv)
                {
                        holdem_class_decl const& decl = holdem_class_decl::get(cid);

                        auto mapped_first_rank = map_rank(decl.first());
                        auto mapped_second_rank = map_rank(decl.second());
                        pattern.emplace_back(
                                decl.category(),
                                mapped_first_rank,
                                mapped_second_rank);
                }

                rank_mapping_ty inv_rank_mapping;
                inv_rank_mapping.fill(-1);
                for (rank_id iter = 0; iter != rank_decl::max_id; ++iter)
                {
                        if( rank_mapping[iter] != -1 )
                                inv_rank_mapping[rank_mapping[iter]] = iter;
                }

                //std::cout << "rank_mapping = " << std_vector_to_string(rank_mapping) << "\n";
                //std::cout << "inv_rank_mapping = " << std_vector_to_string(inv_rank_mapping) << "\n";
                return std::make_tuple(pattern,rank_mapping,inv_rank_mapping);

        }
private:
        std::map<
                std::vector<match_ty>,
                std::shared_ptr<prototype>
        > prototype_map_;
};


void pass_class2normalisedcards::transform(computation_context* ctx, instruction_list* instr_list, computation_result* result){

        static holdem_class_hard_from_proto cache;

        std::vector<holdem_hand_vector> out;
        std::vector<matrix_t> trans;

        instruction_list mapped_instructions;
        std::vector<instruction_list::iterator> to_delete;

        for(auto iter = instr_list->begin(),end = instr_list->end();iter!=end;++iter)
        {
                auto const& instrr = *iter;
                if( instrr->get_type() != instruction::T_ClassEval )
                        continue;
                auto mem = instrr;
                auto instr = reinterpret_cast<class_eval_instruction*>(instrr.get());
               
                out.clear();
                trans.clear();

                cache.populate(instr->get_vector(), out, trans);

                for(size_t idx=0;idx!=out.size();++idx)
                {
                        auto step_rd = result_description::apply_matrix(
                                instr->result_desc(),
                                trans[idx]);
                        mapped_instructions.push_back(std::make_shared<card_eval_instruction>(step_rd, out[idx]));
                }
                to_delete.push_back(iter);

        }

        for (auto const& x : to_delete)
        {
                instr_list->erase(x);
        }
        std::copy(
                std::cbegin(mapped_instructions),
                std::cend(mapped_instructions),
                std::back_inserter(*instr_list)
        );

        
 }


 instruction_list frontend_to_instruction_list(std::string const& group, std::vector<frontend::range> const& players, const bool use_perm_cache){
        instruction_list instr_list;
        tree_range root( players );

        

        std::vector<holdem_hand_vector> out;
        std::vector<matrix_t> trans;

        std::vector<holdem_class_vector> all_class_vs_class_vecs;
        for( auto const& c : root.children ){

                // this means it's a class vs class evaulation
                if( c.is_class_vs_class()  ){

                        holdem_class_vector aux;
                        for (auto const& p : c.players)
                        {
                                aux.push_back(p.get_class_id());
                        }

                        if (use_perm_cache)
                        {
                                all_class_vs_class_vecs.push_back(std::move(aux));
                        }
                        else
                        {
                                instr_list.push_back(std::make_shared<class_eval_instruction>(group, aux));
                        }
                } else
                {
                        for( auto const& d : c.get_children() ){
                                holdem_hand_vector aux{d.players};
                                //agg.append(*eval.evaluate(aux));

                                instr_list.push_back(std::make_shared<card_eval_instruction>(group, aux));
                        }
                }
        }

        if (all_class_vs_class_vecs.size() > 0)
        {
                holdem_class_hard_from_proto cache;
                instruction_list class_vs_class_instr_list;
                for (auto const& cv : all_class_vs_class_vecs)
                {
                        class_vs_class_instr_list.push_back(std::make_shared<class_eval_instruction>(group, cv));
                }
                computation_pass_manager mgr;
                mgr.add_pass<pass_permutate_class>();
                mgr.add_pass<pass_collect_class>();
                //mgr.add_pass<pass_print>();

                const size_t common_size = all_class_vs_class_vecs[0].size();
                const int verboseicity = 0;
                auto comp_ctx = std::make_shared< computation_context>(common_size, verboseicity);
                mgr.execute_(comp_ctx.get(), &class_vs_class_instr_list, nullptr);
                
                for (auto const& untyped_instr : class_vs_class_instr_list)
                {
                        if (untyped_instr->get_type() != instruction::T_ClassEval)
                        {
                                BOOST_THROW_EXCEPTION(std::domain_error("unexpected"));
                        }
                        auto instr = reinterpret_cast<class_eval_instruction*>(untyped_instr.get());

                        out.clear();
                        trans.clear();
                        
                        cache.populate(instr->get_vector(), out, trans);

                        for(size_t idx=0;idx!=out.size();++idx)
                        {
                                auto step_rd = result_description::apply_matrix(
                                        instr->result_desc(),
                                        trans[idx]);
                                instr_list.push_back(std::make_shared<card_eval_instruction>(step_rd, out[idx]));
                        }
                }
        }
        


        return instr_list;
}


} // end namespace ps
