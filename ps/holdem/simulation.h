
#include <cctype>
#include <list>
#include <sstream>
#include <regex>

#include <boost/optional.hpp>
#include <boost/lexical_cast.hpp>

#include <boost/numeric/ublas/matrix.hpp>
#include <boost/numeric/ublas/operation.hpp>
#include <boost/numeric/ublas/io.hpp>

#include "ps/ps.h"
#include "ps/detail/print.h"
#include "ps/holdem/equity_calc.h"
#include "ps/holdem/holdem_range.h"

namespace ps{

        namespace bnu = boost::numeric::ublas;



        /*
         *
         * this class represent a simulation item, for example
         * Ako,QQ+ vs 22+ will both have AA vs QQ and QQ vs AA,
         * we we create a simulation item for AA vs QQ in the
         * form,
         *                   | 1  1 | |wins|
         *                   | 1  1 | |lose|
         *
         * ie we apply the result to both players. consider also
         *  55 vs AKo, we have 
         *              5s5c vs AsKs
         *              5s5d vs AsKs
         *              5s5h vs AsKs
         *              5c5d vs AsKs
         *              ...
         * etc, which can be represented with
         *              5s5d vs AsKs
         *
         *              | 12    0 |
         *              |  0   12 |
         *
         *
         */
        struct simulation_impl_item{
                using hand_type = holdem_int_traits::hand_type;

                simulation_impl_item(hand_type h0, hand_type h1)
                        : hands{h0, h1}
                        , dist(2,2,0)
                {
                        dist(0,0) = 1;
                        dist(1,1) = 1;
                }

                auto get_hash()const{
                        if( hash.empty() ){
                                using std::get;

                                for( auto const& h : hands ){
                                        hash += ht_.to_string(h);
                                }

                                auto ts(hash);

                                std::vector<std::tuple<char, size_t, char> > aux;
                                enum{
                                        Ele_Suit,
                                        Ele_Count,
                                        Ele_Hash
                                };

                                for( auto i{0}; i != hash.size();i+=2){
                                        char s{ hash[i+1] };
                                        auto iter = std::find_if( aux.begin(), aux.end(),
                                                               [&](auto const& _){ return s == std::get<Ele_Suit>(_); });
                                        if( iter == aux.end() ){
                                                aux.emplace_back( s, 1, '_' );
                                        } else {
                                                ++get<Ele_Count>(*iter);
                                        }
                                }

                                size_t mapped{0};
                                for( size_t target = 4 + 1; target !=0; ){
                                        --target;

                                        for( auto& v : aux ){
                                                if( get<Ele_Count>(v) == target ){
                                                        get<Ele_Hash>(v) = boost::lexical_cast<char>(mapped++);
                                                }
                                        }
                                }

                                for( auto i{0}; i != hash.size();i+=2){
                                        char s{ hash[i+1] };
                                        auto iter = std::find_if( aux.begin(), aux.end(),
                                                               [&](auto const& _){ return s == std::get<Ele_Suit>(_); });
                                        hash[i+1] = get<Ele_Hash>(*iter);
                                }


                                // maybe swap pocket pairs
                                assert( hash.size() % 4 == 0 );
                                for( auto i{0}; i != hash.size();i+=4){
                                        if( hash[i] == hash[i+2]  ){
                                                if( hash[i+1] > hash[i+3] ){
                                                        std::swap(hash[i+1], hash[i+3]);
                                                }
                                        }
                                }
                        }

                        return hash;
                }
                void debug()const{
                        get_hash();
                        std::cout << ht_.to_string(hands[0]) 
                                << " vs " 
                                << ht_.to_string(hands[1])
                                << " - " << get_hash() 
                                << " - " << dist
                                << "\n";

                }
                
                holdem_int_traits ht_;

                std::vector<hand_type> hands;
                bnu::matrix<int>    dist;

                mutable std::string    hash;
        };
        
        struct simulation_player{

                simulation_player():wins_{0}, draw_{0}, sigma_{0}, equity_{0}{}
                
                
                holdem_range& get_range(){ return range_; }
                holdem_range const& get_range()const{ return range_; }
                auto wins()const{ return wins_; }
                auto draws()const{ return draw_; }
                auto sigma()const{ return sigma_; }
                auto equity()const{ return equity_ / sigma_; }

                
                friend std::ostream& operator<<(std::ostream& ostr, simulation_player const& self){
                        auto pct{ 1.0 / self.sigma_ * 100 };
                        return ostr  << boost::format("{ sigma=%d, wins=%d(%.2f), draw=%d(%.2f), equity=%.2f(%.2f) }")
                                % self.sigma_
                                % self.wins_ % ( self.wins_ * pct)
                                % self.draw_ % ( self.draw_ * pct)
                                % self.equity_ % ( self.equity_ * pct);
                }
        private:
                friend class simulation_calc;
                holdem_range range_;
                size_t wins_;
                size_t draw_;
                size_t sigma_;
                double equity_;
        };


        struct simulation_context{

        public:
                void debug()const{
                        boost::for_each( items_, [](auto& _){ _.debug(); });
                }

                std::vector<simulation_impl_item> const& get_simulation_items()const{ return items_; }
                std::vector<simulation_player>    & get_players(){ return players_; }
        private:

                friend struct simulation_context_maker;

                void optimize(){
                        if( items_.empty())
                                return;
                        boost::sort(
                                items_, 
                                [](auto const& left, auto const& right){ 
                                        return left.get_hash() < right.get_hash();
                                }
                        );
                        std::vector<simulation_impl_item> next;

                        for( auto const& item : items_){
                                if( next.size() && next.back().get_hash() == item.get_hash()){
                                        next.back().dist += item.dist;
                                        continue;
                                }
                                next.emplace_back( item );
                        }
                        items_ = std::move(next);
                }

                card_traits ct_;
                holdem_int_traits ht_;

                std::vector<simulation_impl_item> items_;
                std::vector<simulation_player> players_;

        };

                

        struct simulation_context_maker{

                void begin_player(){
                        stack_.emplace_back();
                }
                void end_player(){
                        decl_.emplace_back(stack_.back());
                        stack_.pop_back();
                }
                void add(std::string const& range){
                        static std::regex XX_rgx{R"(^[AaKkQqJjTt[:digit:]]{2}$)"};
                        std::smatch m;
                        if( std::regex_search( range, m, XX_rgx ) ){
                                std::string aux{m[0]};
                                for( char& c: aux)
                                        c = tolower(c);
                                auto r = ct_.map_rank(aux[0]);
                                auto s = ct_.map_rank(aux[1]);
                                if( r == s){
                                        // pair
                                        detail::visit_combinations<2>( [&](long a, long b){
                                                PRINT_SEQ((a)(b));
                                                stack_.back().get_range().set( 
                                                        hm_.make( 
                                                                ct_.make(r, a),
                                                                ct_.make(r, b)));
                                        }, 3 );

                                } else{
                                        // non pair
                                        for( auto a : {0,1,2,3} ){
                                                for( auto b : {0,1,2,3} ){
                                                        stack_.back().get_range().set( 
                                                                hm_.make( 
                                                                        ct_.make(r, a),
                                                                        ct_.make(s, b)));
                                                }
                                        }
                                }
                        } else{
                                BOOST_THROW_EXCEPTION(std::domain_error("unknown syntax (" + range + ")"));
                        }
                }
                void debug()const{
                        PRINT_SEQ((decl_.size())(stack_.size()));
                        for( auto& p : decl_ ){
                                p.get_range().debug();
                        }
                }

                auto compile(){
                        assert( decl_.size() == 2 && "not supported");
                        
                        simulation_context sim;

                        auto diter{decl_.begin()};
                        
                        auto const& p0{*diter++};
                        auto const& p1{*diter++};

                        auto r0{p0.get_range().to_vector()};
                        auto r1{p1.get_range().to_vector()};

                        PRINT_SEQ((r0.size())(r1.size()));

                        boost::copy( decl_, std::back_inserter(sim.players_));

                        switch( decl_.size()){
                        case 2:
                                detail::visit_exclusive_combinations<2>(
                                        [&](long a, long b){
                                        sim.items_.emplace_back( r0[a], r1[b] );
                                        //std::cout << ht_.to_string(r0[a]) << " vs " << ht_.to_string(r1[b]) << "\n";
                                }, detail::true_, std::vector<size_t>{r0.size()-1, r1.size()-1} );
                                break;
                        default:
                                BOOST_THROW_EXCEPTION(std::domain_error("bad size "));
                        }

                        sim.debug();
                        sim.optimize();
                        sim.debug();

                        return std::move(sim);

                }
                

        private:
                card_traits ct_;
                holdem_int_traits ht_;
                holdem_int_hand_maker hm_;
                std::list<simulation_player> decl_;
                std::list<simulation_player> stack_; 
       };
        

        struct simulation_calc{
                void run(simulation_context& ctx){

                        auto& players{ ctx.get_players() };

                        bnu::matrix<int> sigma( players.size(), 4, 0);
                        #if 0
                        std::vector<double> equity( players.size(), 0.0);


                        auto equity_sigma = 0;
                        for( auto const& item : ctx.get_simulation_items() ){

                        }
                        #endif

                        for( auto const& item : ctx.get_simulation_items() ){
                                ps::equity_context ectx;
                                for( auto const& h : item.hands ){
                                        ectx.add_player( ht_.to_string(h) );
                                }
                                eq_.run(ectx);

                                bnu::matrix<int> B( item.hands.size(), 4 );
                                bnu::matrix<int> C( 2,4,0);
                                for( size_t i{0}; i!= ectx.get_players().size(); ++i){
                                        auto const& p{ectx.get_players()[i]};
                                        B(i, 0) = p.wins();
                                        B(i, 1) = p.draws();
                                        B(i, 2) = p.sigma();

                                        //equity[i] += p.equity();
                                }


                                axpy_prod(item.dist, B, C, false);


                                sigma += C;
                        }
                                
                        for( size_t i{0}; i!= players.size(); ++i){
                                players[i].wins_   = sigma(i,0);
                                players[i].draw_   = sigma(i,1);
                                players[i].sigma_  = sigma(i,2);
                        }


                }
        private:
                card_traits ct_;
                holdem_int_traits ht_;
                holdem_int_hand_maker hm_;
        
                ps::equity_calc eq_;
        };
}
