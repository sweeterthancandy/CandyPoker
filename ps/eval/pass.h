/*

CandyPoker
https://github.com/sweeterthancandy/CandyPoker

MIT License

Copyright (c) 2019 Gerry Candy

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.

*/
#ifndef PS_BASE_COMPUTER_H
#define PS_BASE_COMPUTER_H

#include <unordered_map>
#include "ps/eval/instruction.h"

namespace ps{

/*
 * Just want a simple linear extensible pass manager, with the ultimate
 * idea of being able to handle transparently class vs class cache but 
 * with loose coupling
 */

struct computation_context;
struct computation_result;

struct computation_pass{
        virtual ~computation_pass()=default;
        virtual void transform(computation_context* ctx, instruction_list* instr_list, computation_result* result = nullptr)=0;
};

struct instruction_map_pass : computation_pass{
        virtual boost::optional<instruction_list> try_map_instruction(computation_context* ctx, instruction* instr)=0;
        virtual void transform(computation_context* ctx, instruction_list* instr_list, computation_result* result)override{
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

/*
        The idea here, is that we have
 */
struct computation_result
        : std::unordered_map<std::string, matrix_t>
{
        explicit computation_result(computation_context const& ctx)
        {
                proto_.resize(ctx.NumPlayers(), ctx.NumPlayers());
                proto_.fill(0.0);
        }
        matrix_t& allocate_tag(std::string const& key){
                auto iter = find(key);
                if( iter == end()){
                        emplace(key, proto_);
                        return allocate_tag(key);
                }
                return iter->second;
        }

        // error handling
        operator bool()const{ return error_.empty(); }
        void set_error(std::string const& msg){
                // take first
                if( error_.empty()){
                        error_ = msg;
                }
        }
private:
        matrix_t proto_;
        std::string error_;
};


struct computation_pass_manager : std::vector<std::shared_ptr<computation_pass> >{
        template<class PassType>
        void add_pass(){
                this->push_back(std::make_shared<PassType>());
        }

        void execute_(computation_context* ctx, instruction_list* instr_list, computation_result* result){
                for(size_t idx=0;idx!=size();++idx){
                        at(idx)->transform(ctx, instr_list, result);
                }
        }
};


struct pass_permutate : computation_pass{
        virtual void transform(computation_context* ctx, instruction_list* instr_list, computation_result* result)override{
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
        virtual void transform(computation_context* ctx, instruction_list* instr_list, computation_result* result)override{
                instr_list->sort( [](auto l, auto r){
                        if( l->get_type() != r->get_type())
                                return l->get_type() != r->get_type();
                        switch(l->get_type()){
                        case instruction::T_CardEval:
                                {
                                        auto lt = reinterpret_cast<card_eval_instruction*>(l.get());
                                        auto rt = reinterpret_cast<card_eval_instruction*>(r.get());
                                        return lt->get_vector() < rt->get_vector();
                                }
                                break;
                        }
                        return false;
                });
        }
};
struct pass_collect : computation_pass{
        virtual void transform(computation_context* ctx, instruction_list* instr_list, computation_result* result)override{
                using iter_type = decltype(instr_list->begin());

                std::vector<iter_type> subset;

                for(iter_type iter(instr_list->begin()),end(instr_list->end());iter!=end;++iter){
                        if( (*iter)->get_type() == instruction::T_CardEval )
                                subset.push_back(iter);
                }


                for(; subset.size() >= 2 ;){
                        auto a = reinterpret_cast<card_eval_instruction*>(&**subset[subset.size()-1]);
                        auto b = reinterpret_cast<card_eval_instruction*>(&**subset[subset.size()-2]);

                        if( a->get_vector() == b->get_vector() ){
                                b->set_matrix( a->get_matrix() + b->get_matrix() );
                                instr_list->erase(subset.back());
                                subset.pop_back();
                        }  else{
                                subset.pop_back();
                        }
                }

        }
};
struct pass_print : computation_pass{
        virtual void transform(computation_context* ctx, instruction_list* instr_list, computation_result* result)override{
                std::cout << "--------BEGIN----------\n";
                for(auto instr : *instr_list ){
                        std::cout << instr->to_string() << "\n";
                }
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
                        result.push_back(std::make_shared<card_eval_instruction>(instrr->group(), _.first, _.second));
                }
                return result;
        }
};

struct pass_write : computation_pass{
        virtual void transform(computation_context* ctx, instruction_list* instr_list, computation_result* result)override{
                for(auto instr : *instr_list ){
                        if( instr->get_type() != instruction::T_Matrix ){
                                result->set_error("not matrix");
                                return;
                        }
                }
                for(auto instr_ : *instr_list ){
                        auto instr = reinterpret_cast<matrix_instruction*>(instr_.get());
                        result->allocate_tag(instr->group()) += instr->get_matrix();
                }
        }
};

} // end namespace ps

#endif // PS_BASE_COMPUTER_H
