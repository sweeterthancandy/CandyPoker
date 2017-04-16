
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
#include "ps/holdem/holdem_traits.h"
#include "ps/holdem/hasher.h"
#include "ps/core/cards.h"

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
                template<class... IntType>
                explicit simulation_impl_item(IntType... h)
                        : hands{h...}
                        , dist(sizeof...(h),sizeof...(h),0)
                {
                        for(size_t i{0};i!=sizeof...(h);++i)
                                dist(i,i) = 1;
                }
                void debug()const{ std::cout << *this << "\n"; }
                std::vector<id_type> const& get_hands()const{ return hands; }
                bnu::matrix<int>& get_distribution(){ return dist; }
                bnu::matrix<int> const& get_distribution()const{ return dist; }
                std::string const& get_hash()const{
                        if( hash.empty() ){
                                hash = suit_hash( hands);
                        }
                        return hash;
                }
                friend std::ostream& operator<<(std::ostream& ostr, simulation_impl_item const& self){
                        for(size_t i{0};i != self.hands.size();++i){
                                if( i != 0)
                                        ostr << " VS ";
                                ostr << holdem_hand_decl::get(self.hands[i]);
                        }
                        return ostr;
                }
                size_t weight      = 1;
        private:
                std::vector<id_type> hands;
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
                auto equity()const{ return equity_; }

                
                friend std::ostream& operator<<(std::ostream& ostr, simulation_player const& self){
                        auto pct{ 1.0 / self.sigma_ * 100 };
                        return ostr  << boost::format("{ sigma=%d, wins=%d(%.2f), draw=%d(%.2f), equity=%.4f(%.2f) }")
                                % self.sigma_
                                % self.wins_ % ( self.wins_ * pct)
                                % self.draw_ % ( self.draw_ * pct)
                                % self.equity_ % ( self.equity_ * 100);
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
                simulation_player const&            get_player(size_t idx)const{ return players_[idx]; }
                size_t total_weight = 0;
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
                                        next.back().get_distribution() += item.get_distribution();
                                        next.back().weight += item.weight;
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

        enum class suit_category{
                any_suit,
                suited,
                unsuited
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
                        static std::regex XX_rgx{R"(^[AaKkQqJjTt[:digit:]]{2}([SsOo])?$)"};
                        std::smatch m;



                        if( std::regex_search( range, m, XX_rgx ) ){
                                std::string aux{m[0]};
                                for( char& c: aux)
                                        c = tolower(c);
                                auto r = ct_.map_rank(aux[0]);
                                auto s = ct_.map_rank(aux[1]);
                                suit_category suit{suit_category::any_suit};
                                        
                                
                                if( aux.size() == 3){
                                        switch(aux[2]){
                                        case 'S': case's': suit = suit_category::suited; break;
                                        case 'O': case'o': suit = suit_category::unsuited; break;
                                        }
                                }
                                if( r == s){
                                        assert( suit == AnySuit && "bad format");
                                        // pair
                                        detail::visit_combinations<2>( [&](long a, long b){
                                                stack_.back().get_range().set( 
                                                        hm_.make( 
                                                                ct_.make(r, a),
                                                                ct_.make(r, b)));
                                        }, 3 );

                                } else{
                                        // non pair
                                        switch(suit){
                                        case suit_category::any_suit:
                                                for( auto a : {0,1,2,3} ){
                                                        for( auto b : {0,1,2,3} ){
                                                                stack_.back().get_range().set( 
                                                                        hm_.make( 
                                                                                ct_.make(r, a),
                                                                                ct_.make(s, b)));
                                                        }
                                                }
                                                break;
                                        case suit_category::unsuited:
                                                for( auto a : {0,1,2,3} ){
                                                        for( auto b : {0,1,2,3} ){
                                                                if( a == b )
                                                                        continue;
                                                                stack_.back().get_range().set( 
                                                                        hm_.make( 
                                                                                ct_.make(r, a),
                                                                                ct_.make(s, b)));
                                                        }
                                                }
                                                break;
                                        case suit_category::suited:
                                                for( auto a : {0,1,2,3} ){
                                                        stack_.back().get_range().set( 
                                                                hm_.make( 
                                                                        ct_.make(r, a),
                                                                        ct_.make(s, a)));
                                                }
                                                break;
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
                        
                        simulation_context sim;

                        std::vector<decltype( decl_.begin()->get_range().to_vector()) > r;

                        for( auto const& p : decl_ ){
                                r.emplace_back( p.get_range().to_vector() );
                        }

                        boost::copy( decl_, std::back_inserter(sim.players_));

                        switch( decl_.size()){
                        case 2:
                                detail::visit_exclusive_combinations<2>(
                                        [&](auto a, auto b){
                                        sim.items_.emplace_back( r[0][a], r[1][b] );
                                        //std::cout << ht_.to_string(r0[a]) << " vs " << ht_.to_string(r1[b]) << "\n";
                                }, detail::true_, std::vector<size_t>{r[0].size()-1, r[1].size()-1} );
                                break;
                        case 3:
                                detail::visit_exclusive_combinations<3>(
                                        [&](auto a, auto b, auto c){
                                        sim.items_.emplace_back( r[0][a], r[1][b], r[2][c]);
                                        //std::cout << ht_.to_string(r0[a]) << " vs " << ht_.to_string(r1[b]) << "\n";
                                }, detail::true_, std::vector<size_t>{r[0].size()-1, r[1].size()-1, r[2].size()-1});
                                break;
                        default:
                                BOOST_THROW_EXCEPTION(std::domain_error("bad size "));
                        }

                        #if 1
                        sim.debug();
                        sim.optimize();
                        sim.debug();
                        #endif
                        
                        for( auto const& item : sim.get_simulation_items() ){
                                sim.total_weight += item.weight;
                        }


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
                        int equity_factor{10'000};
                        int total_weight{0};

                        for( auto const& item : ctx.get_simulation_items() ){
                                ps::equity_context ectx;
                                for( auto const& h : item.get_hands() ){
                                        ectx.add_player( ht_.to_string(h) );
                                }
                                eq_.run(ectx);

                                bnu::matrix<int> B( item.get_hands().size(), 4 );
                                bnu::matrix<int> C( players.size(),4,0);
                                PRINT(item.get_hash());
                                for( size_t i{0}; i!= ectx.get_players().size(); ++i){
                                        auto const& p{ectx.get_players()[i]};
                                        B(i, 0) = p.wins();
                                        B(i, 1) = p.draws();
                                        B(i, 2) = p.sigma();
                                        B(i, 3) = p.equity() * p.sigma();
                                        PRINT(B(i,3));
                                }


                                axpy_prod(item.get_distribution(), B, C, false);


                                sigma += C;

                                PRINT(item.weight);
                                total_weight += item.weight;
                        }
                                
                        for( size_t i{0}; i!= players.size(); ++i){
                                players[i].wins_   = sigma(i,0);
                                players[i].draw_   = sigma(i,1);
                                players[i].sigma_  = sigma(i,2);
                                players[i].equity_ = static_cast<double>(sigma(i,3)) / sigma(i,2);

                                PRINT_SEQ((sigma(i,3))(sigma(i,2)));
                                PRINT( players[i] );
                        }


                }
        private:
                card_traits ct_;
                holdem_int_traits ht_;
                holdem_int_hand_maker hm_;
        
                ps::equity_calc eq_;
        };
}
