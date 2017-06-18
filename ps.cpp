//#include "ps/symbolic.h"
//#include "ps/transforms.h"

#include <type_traits>

#include <boost/timer/timer.hpp>
#include <boost/lexical_cast.hpp>

#if 0



namespace{
        void run_driver(std::vector<ps::frontend::range> const& players)
        {
                boost::timer::auto_cpu_timer at("driver took %w seconds\n");

                using namespace ps;
                using namespace ps::frontend;
                using namespace ps::transforms;

                symbolic_computation::handle star = std::make_shared<symbolic_range>( players );

                //star->print();

                symbolic_computation::transform_schedular sch;
                calculation_context ctx;

                sch.decl<permutate_for_the_better>();
                sch.decl<remove_suit_perms>();
                sch.decl<consolidate_dup_prim>();
                sch.decl<calc_primitive>(ctx);
                //sch.decl_top_down<tree_printer>();

                {
                        boost::timer::auto_cpu_timer at("tranforms took %w seconds\n");
                        sch.execute(star);
                }
                //star->print();

                
                //star->print();

        
                decltype(star->calculate(ctx)) ret;

                {
                        boost::timer::auto_cpu_timer at("calculate took %w seconds\n");
                        ret = star->calculate(ctx);
                }

                auto fmtc = [](auto c){
                        static_assert( std::is_integral< std::decay_t<decltype(c)> >::value, "");
                        std::string mem = boost::lexical_cast<std::string>(c);
                        std::string ret;

                        for(size_t i=0;i!=mem.size();++i){
                                if( i % 3 == 0 && i != 0 )
                                        ret += ',';
                                ret += mem[mem.size()-i-1];
                        }
                        return std::string{ret.rbegin(), ret.rend()};
                };

                const char* title_fmt{ "%10s %16s %16s %12s %12s %10s %10s %20s %10s\n" };
                const char* fmt{       "%10s %16s %16s %12s %12s %10s %10s %20s %10.4f\n" };
                std::cout << boost::format(title_fmt)
                        % "n" % "wins" % "tied" % "|tied|" % "draws2" % "draws3" % "sigma" % "equity" % "equity %";
                for(size_t i{0};i!=ret.size1();++i){
                        double tied_weighted{0.0};
                        size_t tied_abs{0};
                        for( size_t j=1; j <= 3;++j){
                                tied_weighted += static_cast<double>(ret(i,j)) / (j+1);
                                tied_abs += ret(i,j);
                        }
                        std::cout << boost::format(fmt)
                                % players[i] % fmtc(ret(i,0))
                                % fmtc(tied_abs) % fmtc(static_cast<int>(tied_weighted))
                                % fmtc(ret(i,1)) % fmtc(ret(i,2)) 
                                % fmtc(ret(i,9)) % fmtc(ret(i,10)) 
                                % ( static_cast<double>(ret(i,10) ) / computation_equity_fixed_prec / ret(i,9) * 100 );
                }
        }
} // anon

 
void test1(){
        using namespace ps;
        using namespace ps::frontend;

        range p0;
        range p1;
        p0 += _AKo;
        p1 += _QQ++;
        run_driver(std::vector<frontend::range>{p0, p1});
}

#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>

namespace bpt = boost::property_tree;

struct precomputed_item{
        std::vector<ps::frontend::hand> players;
        bnu::matrix<size_t> result;
};
struct precomputed_db{
        auto contains(std::string const& hash){
                return m_.count(hash);
        }
        boost::optional<precomputed_item > get(std::string const& hash)const{
                auto iter{ m_.find(hash) };
                if( iter == m_.end())
                        return boost::none;
                return iter->second;
        }
        void append( std::vector<ps::frontend::hand> const& players,
                     bnu::matrix<size_t> const& result){
                std::string hash;
                for( size_t i{0}; i!= result.size1();++i){
                        if( i != 0)
                                hash += " vs ";
                        hash += players[i].to_string();
                }
                precomputed_item item { players, result };
                m_[hash] = item;
        }

        auto begin()const{ return m_.begin(); }
        auto end()const{ return m_.end(); }
        auto size()const{ return m_.size(); }

private:
        std::map< std::string, precomputed_item> m_;
};
        
void write(std::ostream& ostr, precomputed_db const& db){
        bpt::ptree root;
                
        std::vector<std::string> schema_ = {
                "win1",
                "win2",
                "win3",
                "win4",
                "win5",
                "win6",
                "win7",
                "win8",
                "win9",
                "sigma",
                "equity_100",
        };

        for( auto const& p : db ){

                bpt::ptree sim;
                
                auto const& players{ p.second.players };
                auto const& result{ p.second.result };

                sim.put("hash", p.first);
                sim.put("n", players.size() );

                for( size_t i{0}; i!= result.size1();++i){
                        bpt::ptree ret;
                        ret.add("hand", players[i].to_string() );
                        for(size_t j{0};j!=schema_.size();++j){
                                ret.add(schema_[j], result(i,j) );
                        }


                        // now print these for prettyness
                        
                        double tied_weighted{0.0};
                        size_t tied_abs{0};
                        for( size_t j=1; j <= 8;++j){
                                tied_weighted += static_cast<double>(result(i,j)) / (j+1);
                                tied_abs += result(i,j);
                        }
                        ret.add("tied_abs", tied_abs);
                        ret.add("tied_weighted", tied_weighted);
                        ret.add("equity", static_cast<double>(result(i,10) ) / ps::computation_equity_fixed_prec / result(i,9) * 100 );
                        
                        sim.add_child("players.player", ret);
                        
                }

                root.add_child("simulation", sim);
        }
        bpt::write_json(ostr, root);
}
void load(std::istream& ostr, precomputed_db& db){
        bpt::ptree root;
        bpt::read_json(ostr, root);

        std::vector<std::string> schema_ = {
                "win1",
                "win2",
                "win3",
                "win4",
                "win5",
                "win6",
                "win7",
                "win8",
                "win9",
                "sigma",
                "equity_100",
        };

        for( auto const& sim : root ){
                precomputed_item item;
                        
                //std::string hash = sim.second.get<std::string>("hash");
                size_t n = sim.second.get<size_t>("n");

                item.result = bnu::matrix<size_t>{n, ps::computation_size, 0};
                size_t i=0;
                
                for( auto const& player : sim.second.get_child("players") ){
                        ps::frontend::hand h{ps::holdem_hand_decl::get( player.second.get<std::string>("hand") ) };
                        item.players.emplace_back( h );
                        for(size_t j{0}; j!= schema_.size();++j){
                                item.result(i,j) = player.second.get<size_t>( schema_[j]);
                        }
                        ++i;
                }

                db.append( item.players, item.result );
        }
}



void test2(){
        using namespace ps;
        precomputed_db cache;
        equity_calc eq;
        std::mutex mtx;
        std::mutex cache_mtx;

        std::list<card_id> groups;
        for(card_id id{52}; id!= 3; ){
                --id;
                groups.push_back(id);
        }

        std::vector<std::thread> tg;
        for(size_t i=0;i!= 16; ++i){
                tg.emplace_back( [&](){

                        precomputed_db local_cache;

                        for(;;){

                                boost::optional<card_id> card;
                                mtx.lock(); 
                                if( groups.size() ){
                                        #if 0
                                        card = groups.front();
                                        groups.pop_front();
                                        #else
                                        card = groups.back();
                                        groups.pop_back();
                                        #endif
                                }
                                mtx.unlock();

                                if( ! card )
                                        break;

                                auto c{ card.get() };

                                size_t n{0};

                                detail::visit_combinations<3>( [&](auto &&... cards){


                                        #if 0
                                        ++n;
                                        if( n % 201 != 0 )
                                                return;
                                        #endif

                                        
                                        bnu::matrix<size_t> ret;
                                        std::vector<long> aux{ cards... };

                                        std::vector<holdem_id> players{ holdem_hand_decl::get(holdem_hand_decl::make_id(aux[0], aux[1])),
                                                                        holdem_hand_decl::get(holdem_hand_decl::make_id(aux[2], aux[3])) };

                                        std::vector<ps::frontend::hand> pretty_players;
                                        for( auto id : players ){
                                                pretty_players.emplace_back( id);
                                        }

                                        eq.run( ret, players );
                                        local_cache.append( pretty_players, ret);


                                        #if 0
                                        std::lock_guard<std::mutex> guard(mtx);
                                        const char* title_fmt{ "%10s %16s %16s %12s %12s %10s %10s %20s %10s\n" };
                                        const char* fmt{       "%10s %16s %16s %12s %12s %10s %10s %20s %10.4f\n" };
                                        std::cout << boost::format(title_fmt)
                                                % "n" % "wins" % "tied" % "|tied|" % "draws2" % "draws3" % "sigma" % "equity" % "equity %";
                                        auto fmtc = [](auto c){
                                                static_assert( std::is_integral< std::decay_t<decltype(c)> >::value, "");
                                                std::string mem = boost::lexical_cast<std::string>(c);
                                                std::string ret;

                                                for(size_t i=0;i!=mem.size();++i){
                                                        if( i % 3 == 0 && i != 0 )
                                                                ret += ',';
                                                        ret += mem[mem.size()-i-1];
                                                }
                                                return std::string{ret.rbegin(), ret.rend()};
                                        };
                                        for(size_t i{0};i!=ret.size1();++i){
                                                double tied_weighted{0.0};
                                                size_t tied_abs{0};
                                                for( size_t j=1; j <= 3;++j){
                                                        tied_weighted += static_cast<double>(ret(i,j)) / (j+1);
                                                        tied_abs += ret(i,j);
                                                }
                                                std::cout << boost::format(fmt)
                                                        % pretty_players[i] % fmtc(ret(i,0))
                                                        % fmtc(tied_abs) % fmtc(static_cast<int>(tied_weighted))
                                                        % fmtc(ret(i,1)) % fmtc(ret(i,2)) 
                                                        % fmtc(ret(i,9)) % fmtc(ret(i,10)) 
                                                        % ( static_cast<double>(ret(i,10) ) / computation_equity_fixed_prec / ret(i,9) * 100 );
                                        }
                                        #endif

                                }, ps::detail::true_, c -1, c  );

                                std::lock_guard<std::mutex> guard(cache_mtx);
                                static int wrote = 0;
                                std::cout << "writing (" << ++wrote << ")\n";
                                std::stringstream sstr;
                                write(sstr, local_cache);
                                load(sstr, cache);
                                std::ofstream of("cache.json");
                                write(of, cache);
                        }

                });
        }

        for( auto& t : tg)
                t.join();
                                

}
#endif

#include "ps/tree.h"
#include "ps/equity_calc_detail.h"
        
using namespace ps;
using namespace ps::frontend;

struct hu_visitor{

        template<class Int, class Vec>
        void operator()(Int a, Int b, Int c, Int d, Int e, Vec const& ranked){
                if( ranked[0] < ranked[1] ){
                        ++win;
                } else if( ranked[0] > ranked[1] ){
                        ++lose;
                } else{
                        ++draw;
                }
        }  
        hu_visitor permutate(std::vector<int> const& perm)const{
                hu_visitor ret(*this);
                if( perm[0] == 1 )
                        std::swap(ret.win, ret.lose);
                return ret;
        }
        hu_visitor const& append(hu_visitor const& that){
                win  += that.win;
                lose += that.lose;
                draw += that.draw;
                return *this;
        }
        friend std::ostream& operator<<(std::ostream& ostr, hu_visitor const& self){
                auto sigma{ self.win + self.lose + self.draw };
                auto equity{ (self.win + self.draw / 2.0 ) / sigma * 100 };
                return ostr << boost::format("%2.2f%% (%d,%d,%d)") % equity % self.win % self.draw % self.lose;
        }
        size_t win{0};
        size_t lose{0};
        size_t draw{0};
};

struct equity_cacher{
        hu_visitor visit_boards( std::vector<ps::holdem_id> const& players){
                
                std::vector< std::tuple< size_t, std::string> > player_perm;
                for(size_t i=0;i!=players.size();++i){
                        auto h{ holdem_hand_decl::get( players[i] ) };
                        player_perm.emplace_back(i, h.first().rank().to_string() +
                                                    h.second().rank().to_string() );
                }
                boost::sort(player_perm, [](auto const& left, auto const& right){
                        return std::get<1>(left) < std::get<1>(right);
                });
                std::vector<int> perm;
                std::array< int, 4> suits{0,1,2,3};
                std::array< int, 4> rev_suit_map{-1,-1,-1,-1};
                int suit_iter = 0;

                std::stringstream from, to;
                for(size_t i=0;i!=players.size();++i){
                        perm.emplace_back( std::get<0>(player_perm[i]) );
                }
                for(size_t i=0;i!=players.size();++i){
                        auto h{ holdem_hand_decl::get( players[perm[i]] ) };

                        // TODO pocket pair
                        if(     rev_suit_map[h.first().suit()] == -1 )
                                rev_suit_map[h.first().suit()] = suit_iter++;
                        if(     rev_suit_map[h.second().suit()] == -1 )
                                rev_suit_map[h.second().suit()] = suit_iter++;
                }
                for(size_t i=0;i != 4;++i){
                        if(     rev_suit_map[i] == -1 )
                                rev_suit_map[i] = suit_iter++;
                }
                std::vector< int> suit_perms;
                for(size_t i=0;i != 4;++i){
                        suit_perms.emplace_back(rev_suit_map[i]);
                }
                
                std::vector<ps::holdem_id> perm_hands;
                for(size_t i=0;i != players.size();++i){
                        auto h{ holdem_hand_decl::get( players[perm[i]] ) };
                        perm_hands.emplace_back( 
                                holdem_hand_decl::make_id(
                                        h.first().rank(),
                                        suit_perms[h.first().suit()],
                                        h.second().rank(),
                                        suit_perms[h.second().suit()]));
                }
                return do_visit_boards( perm, perm_hands );
        }
        hu_visitor do_visit_boards( std::vector<int> const& perm, std::vector<ps::holdem_id> const& players){
                auto iter = cache_.find(players);
                if( iter != cache_.end() ){
                        #if 0
                        hu_visitor v;
                        ec_.visit_boards(v, players);
                        if( v.win != iter->second.win  ||
                            v.draw != iter->second.draw ||
                            v.lose != iter->second.lose ){
                                PRINT_SEQ((v)(iter->second));
                                asseet(false);
                        }
                        #endif 
                        hu_visitor aux{ iter->second.permutate(perm) };
                        return aux;
                }

                hu_visitor v;
                ec_.visit_boards(v, players);
                cache_.insert(std::make_pair(players, v));
                return do_visit_boards(perm, players);
        }
private:
        std::map<std::vector<ps::holdem_id>, hu_visitor> cache_;
        ps::equity_calc_detail ec_;
};

void test3(){

        range p0;
        range p1;
        #if 0
        p0 += _QQ++;
        p0 += _AKs;
        p1 += _QQ++;
        #endif
        p0 = percent(100);
        p1 = percent(100);

        tree_range root{ std::vector<frontend::range>{p0, p1} };
        //root.display();
        equity_cacher ec;

        hu_visitor comp;
        for( auto c : root.children ){
                for( auto d : c.children ){
                        std::vector<holdem_id> players;
                        for( auto h : d.players) 
                                players.push_back( h.get() );
                        auto v = ec.visit_boards(players );
                        std::cout << d << " = " << v << "\n";
                        comp.append(v);
                }
        }
        std::cout << root << " = " << comp << "\n";
}

int main(){
        test3();
}
