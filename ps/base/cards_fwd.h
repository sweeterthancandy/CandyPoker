#ifndef PS_CARDS_FWD_H
#define PS_CARDS_FWD_H

#include <string>
#include <cstdint>

#include <Eigen/Dense>

#include <boost/log/trivial.hpp>
#define PS_LOG(level) BOOST_LOG_TRIVIAL(level) << "[" << BOOST_CURRENT_FUNCTION << "] "

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
        
        
        // this is for sim stuff
        using StateType = std::vector<std::vector<Eigen::VectorXd> >;
        
} // ps

#endif // PS_CARDS_FWD_H
