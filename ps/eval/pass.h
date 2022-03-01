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
                proto_.fill(0);
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
        template<class PassType, class... Args>
        void add_pass(Args&&... args){
                this->push_back(std::make_shared<PassType>(std::forward<Args>(args)...));
        }

        void execute_(computation_context* ctx, instruction_list* instr_list, computation_result* result){
                for(size_t idx=0;idx!=size();++idx){
                        at(idx)->transform(ctx, instr_list, result);
                }
        }
};


struct pass_permutate : computation_pass{
        virtual void transform(computation_context* ctx, instruction_list* instr_list, computation_result* result)override{
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
                                        return std::tie(lt->get_vector()) < std::tie(rt->get_vector());
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

                std::map<
                    holdem_hand_vector,
                    std::vector<iter_type>
                > vec_device;

                
                for(iter_type iter(instr_list->begin()),end(instr_list->end());iter!=end;++iter){
                    if ((*iter)->get_type() == instruction::T_CardEval)
                    {
                        const auto ptr = reinterpret_cast<card_eval_instruction*>(&**iter);
                        vec_device[ptr->get_vector()].push_back(iter);
                    }
                }

                for (auto const& p : vec_device)
                {
                    if (p.second.size() < 2)
                    {
                        continue;
                    }
                    std::vector<result_description> agg_result_desc;
                    for (auto const& iter : p.second)
                    {
                        const auto ptr = reinterpret_cast<card_eval_instruction*>(&**iter);
                        auto const& step_result_desc = ptr->result_desc();
                        std::copy(
                            std::cbegin(step_result_desc),
                            std::cend(step_result_desc),
                            std::back_inserter(agg_result_desc));
                        instr_list->erase(iter);
                    }
                    agg_result_desc = result_description::aggregate(agg_result_desc);
                    auto agg_ptr = std::make_shared< card_eval_instruction>(agg_result_desc, p.first);
                    instr_list->push_back(agg_ptr);
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

#if 0
struct pass_class2cards : instruction_map_pass{
        virtual boost::optional<instruction_list> try_map_instruction(computation_context* ctx, instruction* instrr)override{
                if( instrr->get_type() != instruction::T_ClassVec )
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
                        result.push_back(std::make_shared<card_eval_instruction>(instr->result_desc(), _.first, _.second));
                }
                return result;
        }
};
#endif


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
                        auto const& result_desc = instr->result_desc();
                        for (auto const& output_target : result_desc)
                        {
                            result->allocate_tag(output_target.group()) += instr->result_matrix() * output_target.transform();
                        }      
                }
        }
};

#if 0
struct pass_check_matrix_duplicates : computation_pass{
        virtual void transform(computation_context* ctx, instruction_list* instr_list, computation_result* result)override{
                for(auto instr : *instr_list){
                        if( instr->get_type() != instruction::T_Matrix )
                                continue;
                        auto ptr = reinterpret_cast<matrix_instruction*>(instr.get());
                        auto const& matrix = ptr->get_matrix();

                }
        }
};
#endif

} // end namespace ps

#endif // PS_BASE_COMPUTER_H
