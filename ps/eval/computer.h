#ifndef PS_BASE_COMPUTER_H
#define PS_BASE_COMPUTER_H

#include "ps/eval/instruction.h"

namespace ps{

/*
 * Just want a simple linear extensible pass manager, with the ultimate
 * idea of being able to handle transparently class vs class cache but 
 * with loose coupling
 */

struct computation_context;

struct computation_pass{
        virtual ~computation_pass()=default;
        virtual void transform(computation_context* ctx, instruction_list* instr_list)=0;
};

struct instruction_map_pass : computation_pass{
        virtual boost::optional<instruction_list> try_map_instruction(computation_context* ctx, instruction* instr)=0;
        virtual void transform(computation_context* ctx, instruction_list* instr_list)override{
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
};

        
struct computation_context{
        explicit
        computation_context(size_t num_players)
                :num_players_(num_players)
        {}
        size_t NumPlayers()const{
                return num_players_;
        }
        friend std::ostream& operator<<(std::ostream& ostr, computation_context const& self){
                ostr << "num_players_ = " << self.num_players_;
                return ostr;
        }
private:
        size_t num_players_;
};

struct computation_pass_manager : std::vector<std::shared_ptr<computation_pass> >{
        template<class PassType>
        void add_pass(){
                this->push_back(std::make_shared<PassType>());
        }

        boost::optional<matrix_t> execute(computation_context* ctx, instruction_list* instr_list){
                for(size_t idx=0;idx!=size();++idx){
                        at(idx)->transform(ctx, instr_list);
                }
                matrix_t mat(ctx->NumPlayers(), ctx->NumPlayers());
                mat.fill(0);
                for(; instr_list->size() && instr_list->back()->get_type() ==  instruction::T_Matrix;){
                        auto mi = reinterpret_cast<matrix_instruction*>(instr_list->back().get());
                        mat += mi->get_matrix();
                        instr_list->pop_back();
                }
                if( ! instr_list->empty()){
                        return boost::optional<matrix_t>{};
                }
                return mat;
        }
};


struct pass_permutate : computation_pass{
        virtual void transform(computation_context* ctx, instruction_list* instr_list)override{
                for(auto instr : *instr_list){
                        if( instr->get_type() != instruction::T_CardEval )
                                continue;
                        auto ptr = reinterpret_cast<card_eval_instruction*>(instr.get());
                        auto const& vec = ptr->get_vector();
                        auto const& matrix = ptr->get_matrix();
                        auto result = permutate_for_the_better(vec);

                        if( std::get<1>(result) == vec )
                                continue;

                        matrix_t perm_matrix(vec.size(), vec.size());
                        perm_matrix.fill(0);
                        auto const& perm = std::get<0>(result);
                        for(size_t idx=0;idx!=perm.size();++idx){
                                perm_matrix(idx, perm[idx]) = 1.0;
                        }

                        ptr->set_vector(std::get<1>(result));
                        ptr->set_matrix( matrix * perm_matrix );
                }
        }
};
struct pass_sort_type : computation_pass{
        virtual void transform(computation_context* ctx, instruction_list* instr_list)override{
                transform_sort_type(*instr_list);
        }
};
struct pass_collect : computation_pass{
        virtual void transform(computation_context* ctx, instruction_list* instr_list)override{
                transform_collect(*instr_list);
        }
};
struct pass_print : computation_pass{
        virtual void transform(computation_context* ctx, instruction_list* instr_list)override{
                std::cout << "--------BEGIN----------\n";
                transform_print(*instr_list);
                std::cout << "---------END-----------\n";
        }
};

struct pass_class2cards : instruction_map_pass{
        virtual boost::optional<instruction_list> try_map_instruction(computation_context* ctx, instruction* instrr)override{
                if( instrr->get_type() != instruction::T_ClassVec )
                        return boost::none;
                auto instr = reinterpret_cast<class_vec_instruction*>(instrr);
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
                        result.push_back(std::make_shared<card_eval_instruction>(_.first, _.second));
                }
                return result;
        }
};

} // end namespace ps

#endif // PS_BASE_COMPUTER_H
