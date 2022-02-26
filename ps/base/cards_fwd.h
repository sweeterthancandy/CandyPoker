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
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISEmatrix_t, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.

*/
#ifndef PS_CARDS_FWD_H
#define PS_CARDS_FWD_H

#include <string>
#include <iostream>
#include <sstream>
#include <cstdint>

#include <Eigen/Dense>

#include <boost/log/trivial.hpp>
#include <boost/assert.hpp>

//#define PS_LOG(level) BOOST_LOG_TRIVIAL(level) << "[" << BOOST_CURRENT_FUNCTION << "] "
#define PS_LOG(level) BOOST_LOG_TRIVIAL(level)

#define PS_ASSERT_POLICY_BOOST 0
#define PS_ASSERT_POLICY_LOG 1
#define PS_ASSERT_POLICY_NONE 2

#define PS_ASSERT_POLICY PS_ASSERT_POLICY_LOG


#if PS_ASSERT_POLICY == PS_ASSERT_POLICY_BOOST
        #define PS_ASSERT(expr, msg)                                       \
                do{                                                        \
                        if( ! ( expr ) ){                                  \
                                PS_LOG(error) << "{Assert Failed}" << msg; \
                        }                                                  \
                        BOOST_ASSERT( expr);                               \
                }while(0)
#elif PS_ASSERT_POLICY == PS_ASSERT_POLICY_LOG
        #define PS_ASSERT(expr, msg)                                       \
                do{                                                        \
                        if( ! ( expr ) ){                                  \
                                PS_LOG(error) << "{Assert Failed}" << msg; \
                        }                                                  \
                }while(0)
#elif PS_ASSERT_POLICY == PS_ASSERT_POLICY_NONE
#define PS_ASSERT(expr, msg)              
#else
#error "not a valid assert policy"
#endif


namespace ps{

        using id_type         = std::uint_fast16_t;
        using suit_id         = std::uint_fast8_t;
        using rank_id         = std::uint_fast8_t; 
        using card_id         = std::uint_fast8_t; 
        using holdem_id       = std::uint_fast16_t;
        using holdem_class_id = std::uint_fast8_t; 


        enum class suit_category{
                any_suit,
                suited,
                offsuit
        };
        
        enum class holdem_class_type{
                pocket_pair,
                suited,
                offsuit
        };


        struct suit_decl;
        struct rank_decl;
        struct holdem_hand_decl;
        struct holdem_class_decl;
        
        
        inline card_id card_suit_from_id(card_id id){
                return id & 0x3;
        }
        inline card_id card_rank_from_id(card_id id){
                return id >> 2;
        }

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
        template<class VectorType>
        inline std::string std_vector_to_string(VectorType const& vec) {
            std::stringstream sstr;
            std::string sep;
            sstr << "[";
            for (size_t j = 0; j != vec.size(); ++j) {
                sstr << sep << vec[j];

                sep = ",";
            }
            sstr << "]";
            return sstr.str();
        }
        
        
        // this is for sim stuff
        using StateType = std::vector<std::vector<Eigen::VectorXd> >;
        
} // ps

#endif // PS_CARDS_FWD_H
