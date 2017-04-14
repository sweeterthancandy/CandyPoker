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
#include "ps/holdem/hu_equity_calculator.h"

using namespace ps;

namespace ps{

        namespace bnu = boost::numeric::ublas;


        struct holdem_int_traits{
                using card_type = card_traits::card_type;
                using hand_type = long;
                using set_type  = std::vector<long>;

                card_type get(hand_type hand, size_t i)const{
                        assert( i < 2 && "precondition failed");
                        card_type c{-1};
                        switch(i){
                        case 0: c =  hand / 52; break;
                        case 1: c =  hand % 52; break;
                        }
                        assert( c < 52 && "bad mapping");
                        return c;
                }
                std::string to_string(hand_type hand)const{
                        assert( hand <= 52 * 51 + 50 && "bad hand");
                        return ct_.to_string(get(hand,0)) +
                               ct_.to_string(get(hand,1));
                }
        private:
                card_traits ct_;
        };

        struct holdem_int_hand_maker{
                using card_type = card_traits::card_type;
                using hand_type = holdem_int_traits::hand_type;
                using set_type  = holdem_int_traits::set_type;
                
                
                #if 0
                set_type make_set(std::string const& str)const{
                        assert( str.size() % 2 == 0 );
                        set_type result;
                        for(size_t i{0};i!=str.size();i+=2)
                                result.emplace_back( ct_.make(str.substr(i,2)));
                        return std::move(result);
                }
                #endif

                hand_type make(std::string const& str)const{
                        return this->make(ct_.make(str.substr(0,2)),
                                          ct_.make(str.substr(2,2)));
                }
                hand_type make(long a, long b)const{
                        assert( a < 52 && "precondtion failed");
                        assert( b < 52 && "precondtion failed");
                        #if 0
                        PRINT_SEQ((a)(b));
                        PRINT_SEQ((ct_.to_string(a))(ct_.to_string(b)));
                        #endif
                        if( a < b )
                                std::swap(a, b);
                        return a * 52 + b;
                }
        private:
                card_traits ct_;
        };

        struct holdem_range{
                using hand_type = holdem_int_traits::hand_type;

                void set(hand_type hand){
                        assert( hand < 51 * 52 + 50 && "precondition failed");
                        std::cout << boost::format("set(%s;%s)\n") % ht_.to_string(hand) % hand;
                        impl_.insert(hand);
                }
                void debug()const{
                        boost::for_each( impl_, [&](auto _){
                                std::cout << ht_.to_string(_) << "\n";
                        });
                }
                std::vector<hand_type> to_vector()const{
                        return std::vector<hand_type>{impl_.begin(), impl_.end()};
                }
        private:
                card_traits ct_;
                holdem_int_traits ht_;
                std::set<hand_type> impl_;
        };


        namespace detail{
        }

        struct simulation_impl_item{
                using hand_type = holdem_int_traits::hand_type;

                simulation_impl_item(hand_type h0, hand_type h1)
                        : hands{h0, h1}
                        , dist(2,2)
                {
                        dist(0,0) = 1;
                        dist(1,0) = 0;
                        dist(0,1) = 0;
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


                                #if 0
                                // suit, count, mapping
                                using aux_t = std::tuple<char, size_t, char>;
                                std::map<char, size_t> count;
                                std::map<size_t, char> rev;
                                std::map<char, char> m;




                                // create mapping, suit -> count
                                for( auto i{0}; i != hash.size();i+=2){
                                        char s{ hash[i+1] };
                                        ++count[s];
                                }
                                std::vector<std::tuple<char, size_t, char> > aux;
                                // create mapping, count -> suit
                                for( auto const& p : count)
                                        aux.emplace_back(p.first, p.second, '_' );
                                boost::sort(aux, [](auto const& l, auto const& r){
                                            return std::get<1>(l) < std::get<1>(r);
                                });
                                size_t index{0};
                                for( auto iter(aux.rbegin()),end(aux.rend());iter!=end;++iter){
                                        std::get<2>(*iter) = boost::lexical_cast<char>( index++ );
                                        m[std::get<0>(*iter)] = std::get<2>(*iter);
                                }

                                auto pm = [](auto const& m){
                                        std::stringstream sstr;
                                        for( auto const& p : m){
                                                sstr << ", " << p.first << " => " << p.second;
                                        }
                                        return sstr.str();
                                };

                                for( auto i{0}; i != hash.size();i+=2){
                                        char s{ hash[i+1] };
                                        hash[i+1] = m[s];
                                }

                                PRINT(pm(count));
                                PRINT(pm(rev));
                                PRINT(pm(m));
                                PRINT_SEQ((ts)(hash));
                                #endif

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
                
                holdem_range range;
                
                auto wins()const{ return wins_; }
                auto draws()const{ return draw_; }
                auto sigma()const{ return sigma_; }
                auto equity()const{ return equity_ / sigma_; }

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

        namespace detail{
                template<int N, class V, class F, class Upper, class... Args>
		std::enable_if_t<N==0> visit_exclusive_combinations(V v, F f, Upper upper, Args&&... args){
			v(std::forward<Args>(args)...);
		}
                template<int N, class V, class F, class Upper, class... Args>
		std::enable_if_t<N!=0> visit_exclusive_combinations(V v, F f, Upper upper, Args&&... args){
			for(auto iter{upper[N-1]+1};iter!=0;){
				--iter;
				if( ! f(iter) )
					continue;
				visit_exclusive_combinations<N-1>(v, f, upper, iter, std::forward<Args>(args)...);
			}
		}

                auto true_ = [](auto...){return true; };
		
        } // namespace detail

                

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
                                                stack_.back().range.set( 
                                                        hm_.make( 
                                                                ct_.make(r, a),
                                                                ct_.make(r, b)));
                                        }, 3 );

                                } else{
                                        // non pair
                                        for( auto a : {0,1,2,3} ){
                                                for( auto b : {0,1,2,3} ){
                                                        stack_.back().range.set( 
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
                                p.range.debug();
                        }
                }

                auto compile(){
                        assert( decl_.size() == 2 && "not supported");
                        
                        simulation_context sim;

                        auto diter{decl_.begin()};
                        
                        auto const& p0{*diter++};
                        auto const& p1{*diter++};

                        auto r0{p0.range.to_vector()};
                        auto r1{p1.range.to_vector()};

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

                        #if 0
                        std::vector<std::vector<long> > comb;

                        for( auto& p : decl_ ){
                                if( comb.empty() ){
                                        p.range.for_each( [&](long hand){
                                                comb.push_back(hand);
                                        }
                                } else {
                                        std::vector<std::vector<long> > comb_next;
                                        p.range.for_each( [&](long hand){
                                                for( auto proto : comb_next ){
                                                       proto.push_back(hand); 
                                                       comb_next.push_back( std::move(proto));
                                                }
                                        }
                                }
                                        



                        }
                        #endif
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

                        bnu::matrix<int> sigma( players.size(), 3, 0);



                        for( auto const& item : ctx.get_simulation_items() ){
                                PRINT(sigma);
                                item.debug();
                                ps::equity_context ectx;
                                for( auto const& h : item.hands ){
                                        ectx.add_player( ht_.to_string(h) );
                                }
                                eq_.run(ectx);

                                bnu::matrix<int> B( item.hands.size(), 3 );
                                bnu::matrix<int> C( 2,3,0);
                                for( size_t i{0}; i!= ectx.get_players().size(); ++i){
                                        auto const& p{ectx.get_players()[i]};
                                        B(i, 0) = p.wins();
                                        B(i, 1) = p.draws();
                                        B(i, 2) = p.sigma();
                                }

                                PRINT(B);
                                PRINT(item.dist);

                                axpy_prod(item.dist, B, C, false);

                                PRINT(C);

                                sigma += C;
                        }
                        PRINT(sigma);
                }
        private:
                card_traits ct_;
                holdem_int_traits ht_;
                holdem_int_hand_maker hm_;
        
                ps::equity_calc eq_;
        };
}

int main(){
        std::cout << "hello\n";

        simulation_context_maker maker;
        simulation_calc sc;

        maker.begin_player();
        maker.add("55");
        maker.end_player();

        maker.begin_player();
        maker.add("AK");
        maker.end_player();

        maker.debug();

        auto ctx{maker.compile()};


        sc.run(ctx);





        //hu_solver_test();
}
