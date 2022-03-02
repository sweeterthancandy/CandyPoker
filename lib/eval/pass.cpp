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
                            result->allocate_tag(output_target.group()) += instr->result_matrix() * output_target.transform();
                        }      
                }
        }




} // end namespace ps
