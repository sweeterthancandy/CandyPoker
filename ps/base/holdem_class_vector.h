#ifndef PS_BASE_HOLDEM_CLASS_VECTOR_H
#define PS_BASE_HOLDEM_CLASS_VECTOR_H

#include <vector>
#include <map>

#include "ps/base/cards.h"
#include "ps/base/holdem_hand_vector.h"
#include "ps/detail/print.h"

namespace ps{

        struct holdem_class_vector : std::vector<ps::holdem_class_id>{
                template<class... Args>
                holdem_class_vector(Args&&... args)
                : std::vector<ps::holdem_class_id>{std::forward<Args>(args)...}
                {}
                friend std::ostream& operator<<(std::ostream& ostr, holdem_class_vector const& self);
                holdem_class_decl const& decl_at(size_t i)const;
                std::vector< holdem_hand_vector > get_hand_vectors()const;

                template<
                        class... Args,
                        class = std::enable_if_t< ! std::is_constructible<std::string, Args...>::value  >
                >
                void push_back(Args&&... args){
                        this->std::vector<ps::holdem_class_id>::push_back(std::forward<Args...>(args)...);
                }
                void push_back(std::string const& item){
                        this->push_back( holdem_class_decl::parse(item).id() );
                }
                template<class Archive>
                void serialize(Archive& ar, unsigned int){
                        ar & (*reinterpret_cast<std::vector<ps::holdem_class_id>*>(this));
                }
                /*
                        Each class expands to a set of hands, ie
                                AA -> {AhAc, AhAs, AhSd, AcAs, AcAd, AsAd}.
                        For this we need to take a set of classes,
                                {A,B,C},
                        can work out how many combinations. For disjoint sets
                        like {AA,KK}, this is the product of the the number
                        in each each 6^2 = 36, but for {AA,AA}, their is only
                        one combintation of this, and for {AA,AKs}, their 
                        is only three combinations
                 */
                size_t weight()const{
                        size_t sz = this->size();
                        std::vector<
                                std::vector<holdem_id> const* 
                        > hand_sets;
                        for( auto id : *this){
                                hand_sets.push_back( &holdem_class_decl::get(id).get_hand_vector() );
                        }
                        std::vector<size_t> idx_vec(sz, 0);

                        size_t count =0;
                        for(;;){
                                auto mask = 0ull;
                                for(size_t i=0;i!=sz;++i){
                                        auto u = holdem_hand_decl::get((*hand_sets[i])[idx_vec[i]]).mask();
                                        if( !!(mask & u) )
                                                goto continue_;
                                        mask |= u;
                                }
                                ++count;
                        continue_:
                                // now increment the idx_vec
                                size_t cursor = sz -1;
                                for(;cursor != -1;){
                                        if( ++idx_vec[cursor] < hand_sets[cursor]->size() ){
                                                break;
                                        }
                                        idx_vec[cursor] = 0;
                                        --cursor;
                                }
                                if( cursor == -1 )
                                        break;
                        }
                        return count;
                }
                /*
                        This is to scale back the weight() so that for all combinations
                        we have that
                                              \sigma weight() == 1.
                                First note that this is different from taking a 
                        sequence {a,b,c,d}, where a > b > c > d because when taking
                        2 holdem hands {a,b}, {c,d}, we have a > b and c > d but
                        it's not neccasarily the case a > c.

                        for this we note that by removing 2 cards {a,b} from 52, and
                        demanding that a > b, we have 52 * 51/2 combinations for the 
                        pair {a,b}, which is morphic to the first card.
                                Now consider taking 2 more cards from 50, for the
                        pair {c,d} where c > d we have 50 * 49 /2 combintations of this.
                        Now for both {{a,b}, {c,d}}, as they are exclusive we take 
                        the product to figure out how many combinations there are
                 */
                double prob()const{
                        size_t deck = 52;
                        size_t factor = 1;
                        for(size_t i=0;i!=this->size();++i, deck-=2){
                                factor *= ( deck ) * ( deck - 1 ) / 2;
                        }
                        return static_cast<double>(this->weight())/factor;
                }


                std::tuple<
                        std::vector<int>,
                        holdem_class_vector
                > to_standard_form()const;
                
                std::vector<
                       std::tuple< std::vector<int>, holdem_hand_vector >
                > to_standard_form_hands()const;

                bool is_standard_form()const;
        };
        
        namespace detail{

                template<class Policy>
                struct basic_holdem_class_deal_iterator :
                        basic_index_iterator<
                                holdem_id,
                                Policy,
                                holdem_class_vector
                        >
                {
                        using impl_t = 
                                basic_index_iterator<
                                        holdem_id,
                                        Policy,
                                        holdem_class_vector
                                >
                        ;
                        basic_holdem_class_deal_iterator():impl_t{}{}
                        basic_holdem_class_deal_iterator(size_t n):
                                impl_t(n, 13*13)
                        {
                                eat_zero_weight_();
                        }
                        basic_holdem_class_deal_iterator& operator++(){
                                impl_t::operator++();
                                eat_zero_weight_();
                                return *this;
                        }
                private:
                        void eat_zero_weight_(){
                                for(;!this->eos();){
                                        if( this->operator->()->weight() != 0 )
                                                break;
                                        impl_t::operator++();
                                }
                        }
                };
        } // detail

        // for when A < B < C
        using holdem_class_iterator = detail::basic_holdem_class_deal_iterator<strict_lower_triangle_policy>;
        // for when we have {A,B}, {B,A}
        using holdem_class_deal_iterator = detail::basic_holdem_class_deal_iterator<range_policy>;
} // ps

#endif // PS_BASE_HOLDEM_CLASS_VECTOR_H
