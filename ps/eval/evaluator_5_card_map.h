#ifndef PS_EVAL_EVALUATOR_5_CARD_MAP_H
#define PS_EVAL_EVALUATOR_5_CARD_MAP_H


#include "ps/eval/evaluator.h"

#include <sstream>
#include <mutex>

#include <boost/range/algorithm.hpp>
#include <boost/timer/timer.hpp>
#include "ps/detail/visit_combinations.h"
#include "ps/detail/print.h"

#include "ps/base/generate.h"

namespace ps{
namespace prime_rank_map{
        
        //                                                       2 3 4 5  6  7  8  9  T  J  Q  K  A
        static constexpr std::array<std::uint32_t, 13> Primes = {2,3,5,7,11,13,17,19,23,27,29,31,37};

        using prime_rank_t = std::uint32_t;

        inline
        prime_rank_t append(prime_rank_t val, rank_id rank)noexcept{
                return val * Primes[rank];
        }

        inline
        prime_rank_t create()noexcept{ return 1; }
        inline
        prime_rank_t create(rank_vector const& rv) noexcept{
                auto val = create();
                for(auto id : rv )
                        val = append(val, id);
                return val;
        }
        template<class... Args>
        prime_rank_t create(Args... args) noexcept{
                auto val = create();
                int _[] = {0,  (val = append(val, args),0)...};
                return val;
        }
        template<class... Args>
        prime_rank_t create_from_cards(Args... args) noexcept{
                auto val = create();
                int _[] = {0,  (val = append(val, card_rank_from_id(args)),0)...};
                return val;
        }
        inline
        const prime_rank_t max()noexcept{
                return create(12,12,12,12,11,11,11);
        }

} // end namespace prime_rank_map
} // end namespace ps

namespace ps{
namespace prime_suit_map{
        
        //                                                       2 3 4 5  6  7  8  9  T  J  Q  K  A
        static constexpr std::array<std::uint32_t, 13> Primes = {2,3,5,7,11,13,17,19,23,27,29,31,37};

        using prime_suit_t = std::uint32_t;
        
        inline
        prime_suit_t append(prime_suit_t val, suit_id suit)noexcept{
                return val * Primes[suit];
        }
        inline
        prime_suit_t create()noexcept{ return 1; }
        template<class... Args>
        prime_suit_t create(Args... args) noexcept{
                auto val = create();
                int _[] = {0,  (val = append(val, args),0)...};
                return val;
        }
        template<class... Args>
        prime_suit_t create_from_cards(Args... args) noexcept{
                auto val = create();
                int _[] = {0,  (val = append(val, card_suit_from_id(args)),0)...};
                return val;
        }

        inline bool is_flush(prime_suit_t val)noexcept{
                switch(val){
                case 2*2*2*2*2:
                case 3*3*3*3*3:
                case 5*5*5*5*5:
                case 7*7*7*7*7:
                        return true;
                default:
                        return false;
                }
        }


} // end namespace prime_suit_map
} // end namespace

namespace ps{

struct evaluator_5_card_map{
        evaluator_5_card_map(){

                struct V{
                        void begin(std::string const&){}
                        void end(){}
                        void next( bool f, long a, long b, long c, long d, long e){
                                auto m = prime_rank_map::create(a,b,c,d,e);
                                if( f )
                                        self_->flush_map_[m] = order_;
                                else
                                        self_->rank_map_[m] = order_;
                                ++order_;
                        }
                        evaluator_5_card_map* self_;
                        size_t order_{1};
                };
                V v = {this};
                flush_map_.resize( 37 * 37 * 37 * 37 * 31 +1 );
                rank_map_.resize( 37 * 37 * 37 * 37 * 31 +1 );
                visit_poker_rankings(v);
        }


        static evaluator_5_card_map* instance(){
                static auto ptr = new evaluator_5_card_map;
                return ptr;
        }
        ranking_t rank(long a, long b, long c, long d, long e)const{

                auto f_aux = prime_suit_map::create_from_cards(a,b,c,d,e);
                auto m     = prime_rank_map::create_from_cards(a,b,c,d,e);

                ranking_t ret;
                if( prime_suit_map::is_flush(f_aux) ){
                        ret = flush_map_[m];
                } else{
                        ret = rank_map_[m];
                }
                return ret;
        }
        ranking_t rank(long a, long b, long c, long d, long e, long f)const{
                std::array<ranking_t, 6> aux { 
                        rank(  b,c,d,e,f),
                        rank(a,  c,d,e,f),
                        rank(a,b,  d,e,f),
                        rank(a,b,c,  e,f),
                        rank(a,b,c,d,  f),
                        rank(a,b,c,d,e  )
                };
                return * std::min_element(aux.begin(), aux.end());
        }
        ranking_t rank(long a, long b, long c, long d, long e, long f, long g)const{
                std::array<ranking_t, 7> aux = {
                        rank(  b,c,d,e,f,g),
                        rank(a,  c,d,e,f,g),
                        rank(a,b,  d,e,f,g),
                        rank(a,b,c,  e,f,g),
                        rank(a,b,c,d,  f,g),
                        rank(a,b,c,d,e,  g),
                        rank(a,b,c,d,e,f  )
                };
                return * std::min_element(aux.begin(), aux.end());
        }
private:
        std::vector<ranking_t> flush_map_;
        std::vector<ranking_t> rank_map_;
};

} // ps
#endif // PS_EVAL_EVALUATOR_5_CARD_MAP_H
