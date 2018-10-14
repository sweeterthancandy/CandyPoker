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

namespace ps{
        
using matrix_t = Eigen::Matrix< unsigned long long , Eigen::Dynamic , Eigen::Dynamic >;

/*
 * we want to print a 2x2 matrix as
 *    [[m(0,0), m(1,0)],[m(0,1),m(1,1)]]
 */
template<class MatrixType>
inline std::string matrix_to_string(MatrixType const& mat){
        std::stringstream sstr;
        std::string sep;
        sstr << "[";
        for(size_t j=0;j!=mat.rows();++j){
                sstr << sep << "[";
                sep = ",";
                for(size_t i=0;i!=mat.cols();++i){
                        sstr << (i!=0?",":"") << mat(i,j);
                }
                sstr << "]";
        }
        sstr << "]";
        return sstr.str();
}
template<class VectorType>
inline std::string vector_to_string(VectorType const& vec){
        std::stringstream sstr;
        std::string sep;
        sstr << "[";
        for(size_t j=0;j!=vec.size();++j){
                sstr << sep << vec(j);
                sep = ",";
        }
        sstr << "]";
        return sstr.str();
}

struct equity_view : std::vector<double>{
        equity_view(matrix_t const& breakdown){
                sigma_ = 0;
                size_t n = breakdown.rows();
                std::map<long, unsigned long long> sigma_device;
                for( size_t i=0;i!=n;++i){
                        for(size_t j=0; j != n; ++j ){
                                sigma_device[j] += breakdown(j,i);
                        }
                }
                for( size_t i=0;i!=n;++i){
                        sigma_ += sigma_device[i] / ( i +1 );
                }


                for( size_t i=0;i!=n;++i){

                        double equity = 0.0;
                        for(size_t j=0; j != n; ++j ){
                                equity += breakdown(j,i) / ( j +1 );
                        }
                        equity /= sigma_;
                        push_back(equity);
                }
        }
        unsigned long long sigma()const{ return sigma_; }
        bool valid()const{
		for(auto _ : *this){
                        switch(std::fpclassify(_)) {
                        case FP_INFINITE:
                        case FP_NAN:
                        case FP_SUBNORMAL:
                        default:
                                return false;
                        case FP_ZERO:
                        case FP_NORMAL:
                                break;	
                        }
                }
                return true;
        }
private:
        unsigned long long sigma_;
};

struct instruction{
        enum type{
                T_CardEval,
                T_ClassEval,
                T_Matrix,
                T_ClassVec,
        };
        explicit instruction(type t):type_{t}{}
        type get_type()const{ return type_; }

        virtual std::string to_string()const=0;
        virtual std::shared_ptr<instruction> clone()const=0;
private:
        type type_;
};




struct class_vec_instruction : instruction{
        explicit class_vec_instruction(holdem_class_vector vec)
                :instruction{T_ClassVec}
                ,vec_{std::move(vec)}
        {}
        virtual std::string to_string()const override{
                std::stringstream sstr;
                sstr << "ClassVec{" << vec_.to_string() << "}";
                return sstr.str();
        }
        virtual std::shared_ptr<instruction> clone()const override{
                return std::make_shared<class_vec_instruction>(vec_);
        }
        holdem_class_vector const& get_vector()const{ return vec_; }
private:
        holdem_class_vector vec_;
};

struct matrix_instruction : instruction{
        explicit matrix_instruction(matrix_t mat)
                :instruction{T_Matrix}
                ,mat_{std::move(mat)}
        {}
        virtual std::string to_string()const override{
                std::stringstream sstr;
                sstr << "Matrix{" << matrix_to_string(mat_) << "}";
                return sstr.str();
        }
        virtual std::shared_ptr<instruction> clone()const override{
                return std::make_shared<matrix_instruction>(mat_);
        }
        matrix_t const& get_matrix()const{ return mat_; }
private:
        matrix_t mat_;
};

template<class VectorType, instruction::type Type>
struct basic_eval_instruction : instruction{

        using vector_type = VectorType;
        using self_type = basic_eval_instruction;

        basic_eval_instruction(vector_type const& vec)
                : instruction{Type}
                , vec_{vec}
                , matrix_{matrix_t::Identity(vec.size(), vec.size())}
        {
        }
        basic_eval_instruction(vector_type const& vec, matrix_t const& matrix)
                : instruction{Type}
                , vec_{vec}
                , matrix_{matrix}
        {
        }
        holdem_hand_vector get_vector()const{
                return vec_;
        }
        void set_vector(holdem_hand_vector const& vec){
                vec_ = vec;
        }
        matrix_t const& get_matrix()const{
                return matrix_;
        }
        void set_matrix(matrix_t const& matrix){
                matrix_ = matrix;
        }
        virtual std::string to_string()const override{
                std::stringstream sstr;
                sstr << (Type == T_CardEval ? "CardEval" : "ClassEval" ) << "{" << vec_ << ", " << matrix_to_string(matrix_) << "}";
                return sstr.str();
        }
        
        virtual std::shared_ptr<instruction> clone()const override{
                return std::make_shared<self_type>(vec_, matrix_);
        }
        
        friend std::ostream& operator<<(std::ostream& ostr, basic_eval_instruction const& self){
                return ostr << self.to_string();
        }


private:
        vector_type vec_;
        matrix_t matrix_;
};

using card_eval_instruction  = basic_eval_instruction<holdem_hand_vector, instruction::T_CardEval>;
using class_eval_instruction = basic_eval_instruction<holdem_class_vector, instruction::T_ClassEval>;


using instruction_list = std::list<std::shared_ptr<instruction> >;

inline
void transform_print(instruction_list& instr_list){
        for(auto instr : instr_list ){
                std::cout << instr->to_string() << "\n";
        }
}
inline
void transform_permutate(instruction_list& instr_list){
        for(auto instr : instr_list){
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


inline
void transform_sort_type(instruction_list& instr_list){
        instr_list.sort( [](auto l, auto r){
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
inline
void transform_collect(instruction_list& instr_list){
        using iter_type = decltype(instr_list.begin());

        std::vector<iter_type> subset;

        for(iter_type iter(instr_list.begin()),end(instr_list.end());iter!=end;++iter){
                if( (*iter)->get_type() == instruction::T_CardEval )
                        subset.push_back(iter);
        }


        for(; subset.size() >= 2 ;){
                auto a = reinterpret_cast<card_eval_instruction*>(&**subset[subset.size()-1]);
                auto b = reinterpret_cast<card_eval_instruction*>(&**subset[subset.size()-2]);

                if( a->get_vector() == b->get_vector() ){
                        b->set_matrix( a->get_matrix() + b->get_matrix() );
                        instr_list.erase(subset.back());
                        subset.pop_back();
                }  else{
                        subset.pop_back();
                }
        }
}

inline
std::vector<card_eval_instruction> transform_cast_to_card_eval(instruction_list& instr_list){
        transform_permutate(instr_list);
        transform_sort_type(instr_list);
        transform_collect(instr_list);

        std::vector<card_eval_instruction> result;
        for(auto instr : instr_list){
                BOOST_ASSERT(  instr->get_type() == instruction::T_CardEval );
                auto ptr = reinterpret_cast<card_eval_instruction*>(instr.get());
                result.emplace_back(ptr->get_vector(), ptr->get_matrix());
        }
        return result;
}

inline
instruction_list frontend_to_instruction_list(std::vector<frontend::range> const& players){
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
                        for( auto const& d : c.children ){
                                holdem_hand_vector aux{d.players};
                                //agg.append(*eval.evaluate(aux));

                                instr_list.push_back(std::make_shared<card_eval_instruction>(aux));
                        }
                }
        }
        return instr_list;
}

inline
std::vector<card_eval_instruction> frontend_to_card_instr(std::vector<frontend::range> const& players){
        auto instr_list = frontend_to_instruction_list(players);
        return transform_cast_to_card_eval(instr_list);
}
inline 
instruction_list instruction_list_deep_copy(instruction_list const& instr_list){
        instruction_list copy_list;
        for(auto instr : instr_list){
                copy_list.push_back(instr->clone());
        }
        return copy_list;
}


} // end namespace ps

#endif // PS_BASE_INSTRUCTION_H
