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
#ifndef PS_BASE_INSTRUCTION_H
#define PS_BASE_INSTRUCTION_H


#include <Eigen/Dense>
#include <map>
#include <cfloat>
#include <memory>
#include <string>
#include <list>
#include <tuple>

#include "ps/base/cards.h"
#include "ps/base/algorithm.h"
#include "ps/base/frontend.h"
#include "ps/base/tree.h"

#include <boost/optional.hpp>

namespace ps{
        

struct instruction{
        enum type{
                T_CardEval,
                T_ClassEval,
                T_Matrix,
                T_ClassVec,
                T_NOP,
        };
        explicit instruction(type t)
                : type_{t}
        {}
        type get_type()const{ return type_; }

        virtual std::string to_string()const=0;
        virtual std::shared_ptr<instruction> clone()const=0;
private:
        type type_;
};

struct nop_instruction : instruction
{
    nop_instruction() :instruction{ T_NOP } {}
    virtual std::string to_string()const {
        return "NOP";
    }
    virtual std::shared_ptr<instruction> clone()const
    {
        return std::make_shared<nop_instruction>();
    }
};

struct result_description
{
    result_description(
        std::string const& group,
        matrix_t const& transform)
        : group_{ group }, transform_{ transform }
    {}
    std::string const& group()const {
        return group_;
    }
    matrix_t const& transform()const
    {
        return transform_;
    }
    friend std::ostream& operator<<(std::ostream& ostr, result_description const& self)
    {
        return ostr << "ResultDesc{" << matrix_to_string(self.transform()) << ", group=" << self.group() << "}";
    }
    static std::vector<result_description> apply_perm(std::vector<result_description> const& desc_list, std::vector<int> const& perm)
    {

        matrix_t perm_matrix(perm.size(), perm.size());
        perm_matrix.fill(0);

        for (size_t idx = 0; idx != perm.size(); ++idx) {
            perm_matrix(idx, perm[idx]) = 1;
        }

        std::vector<result_description> result;
        for (auto const& x : desc_list)
        {
            result.emplace_back(x.group(), x.transform() * perm_matrix);
        }
        return result;
    }
    static std::vector<result_description> aggregate(std::vector<result_description> const& desc_list)
    {
        std::unordered_map<
            std::string,
            boost::optional<matrix_t>
        > sum_device;
        for (auto const& x : desc_list)
        {
            auto& head = sum_device[x.group()];
            if (head.has_value())
            {
                head.get() += x.transform();
            }
            else
            {
                head.emplace(x.transform());
            }
        }

        std::vector<result_description> result;
        for (auto const& p : sum_device)
        {
            result.emplace_back(p.first, std::move(p.second.get()));
        }

        return result;
    }
private:
    std::string group_;
    matrix_t transform_;
};


#if 0

struct class_vec_instruction : instruction{
        explicit class_vec_instruction(std::vector<result_description> const& result_desc, holdem_class_vector vec)
                :instruction{T_ClassVec}
            , result_desc_{ result_desc }
                ,vec_{std::move(vec)}
        {}
        virtual std::string to_string()const override{
                std::stringstream sstr;
                sstr << "ClassVec{" << vec_.to_string() << "}";
                return sstr.str();
        }
        virtual std::shared_ptr<instruction> clone()const override{
                return std::make_shared<class_vec_instruction>(result_desc_, vec_);
        }
        holdem_class_vector const& get_vector()const{ return vec_; }
        auto const& result_desc()const { return result_desc_; }
private:
        std::vector<result_description> result_desc_;
        holdem_class_vector vec_;
};
#endif

struct matrix_instruction : instruction{
        explicit matrix_instruction(std::vector< result_description> result_desc, matrix_t mat,
                                    std::string const& dbg_msg = std::string{})
                :instruction{ T_Matrix}
                , result_desc_{std::move(result_desc)}
                , mat_{ std::move(mat) }
                ,dbg_msg_{dbg_msg}
        {}
        virtual std::string to_string()const override{
                std::stringstream sstr;
                sstr << "Matrix{" << matrix_to_string(mat_) << ", " << std_vector_to_string(result_desc_);
                if( dbg_msg_.size() ){
                        sstr << ", " << dbg_msg_;
                }
                sstr << "}";
                return sstr.str();
        }
        virtual std::shared_ptr<instruction> clone()const override{
                return std::make_shared<matrix_instruction>(result_desc_, mat_);
        }
        matrix_t const& result_matrix()const { return mat_; }
        std::vector< result_description> const& result_desc()const { return result_desc_; }
        std::string const& debug_message()const{ return dbg_msg_; }
private:
        std::vector< result_description> result_desc_;
        matrix_t mat_;
        std::string dbg_msg_;
};

template<class VectorType, instruction::type Type>
struct basic_eval_instruction : instruction{

        using vector_type = VectorType;
        using self_type = basic_eval_instruction;

        basic_eval_instruction(std::string const& group, vector_type const& vec)
                : instruction{Type}
                , vec_{vec}
            , result_desc_{ {result_description(group, matrix_t::Identity(vec.size(), vec.size()))} }
        {
        }
        basic_eval_instruction(std::vector<result_description> result_desc, vector_type const& vec)
                : instruction{Type}
                , vec_{vec}
                , result_desc_{std::move(result_desc)}
        {
        }
        vector_type get_vector()const{
                return vec_;
        }
        virtual std::string to_string()const override{
                std::stringstream sstr;
                sstr << (Type == T_CardEval ? "CardEval" : "ClassEval" ) << "{" << vec_ << ", group=" << std_vector_to_string(result_desc_) << "}";
                return sstr.str();
        }
        
        virtual std::shared_ptr<instruction> clone()const override{
                return std::make_shared<self_type>(result_desc_, vec_);
        }
        
        friend std::ostream& operator<<(std::ostream& ostr, basic_eval_instruction const& self){
                return ostr << self.to_string();
        }
        auto const& result_desc()const { return result_desc_; }

private:
        vector_type vec_;
        std::vector<result_description> result_desc_;
};

using card_eval_instruction  = basic_eval_instruction<holdem_hand_vector, instruction::T_CardEval>;
using class_eval_instruction = basic_eval_instruction<holdem_class_vector, instruction::T_ClassEval>;


using instruction_list = std::list<std::shared_ptr<instruction> >;



inline
instruction_list frontend_to_instruction_list(std::string const& group, std::vector<frontend::range> const& players){
        instruction_list instr_list;
        tree_range root( players );

        for( auto const& c : root.children ){
#if 0
                // this means it's a class vs class evaulation
                if( c.opt_cplayers.size() != 0 ){
                        holdem_class_vector aux{c.opt_cplayers};
                        //agg.append(*class_eval.evaluate(aux));
                        instr_list.push_back(std::make_shared<class_eval_instruction>(aux));
                } else
#endif
                {
                        for( auto const& d : c.get_children() ){
                                holdem_hand_vector aux{d.players};
                                //agg.append(*eval.evaluate(aux));

                                instr_list.push_back(std::make_shared<card_eval_instruction>(group, aux));
                        }
                }
        }
        return instr_list;
}



} // end namespace ps

#endif // PS_BASE_INSTRUCTION_H
