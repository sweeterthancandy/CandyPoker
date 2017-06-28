#include "ps/heads_up.h"
#include "ps/detail/print.h"

#if 1
namespace ps{
struct class_equity_cacher{
        struct result_t{
                result_t permutate(std::vector<int> const& perm)const{
                        result_t ret(*this);
                        if( perm[0] == 1 )
                                std::swap(ret.win, ret.lose);
                        return ret;
                }
                result_t const& append(result_t const& that){
                        win  += that.win;
                        lose += that.lose;
                        draw += that.draw;
                        return *this;
                }
                template<class Archive>
                void serialize(Archive& ar, unsigned int){
                        ar & win;
                        ar & lose;
                        ar & draw;
                }
                friend std::ostream& operator<<(std::ostream& ostr, result_t const& self){
                        auto sigma{ self.win + self.lose + self.draw };
                        auto equity{ (self.win + self.draw / 2.0 ) / sigma * 100 };
                        return ostr << boost::format("%2.2f%% (%d,%d,%d)") % equity % self.win % self.draw % self.lose;
                }
                size_t win{0};
                size_t lose{0};
                size_t draw{0};
        };

        bool load(std::string const& name){
                std::ifstream is(name);
                if( ! is.is_open() )
                        return false;
                boost::archive::text_iarchive ia(is);
                ia >> *this;
                return true;
        }
        bool save(std::string const& name)const{
                std::ofstream of(name);
                boost::archive::text_oarchive oa(of);
                oa << *this;
                return true;
        }
        template<class Archive>
        void serialize(Archive& ar, unsigned int){
                ar & cache_;          
        }

        class_equity_cacher(){
                //ec_.load("cache.bin");
        }
        
        result_t const& visit_boards( std::vector<ps::holdem_class_id> const& players){
                auto iter{ cache_.find( players) };
                if( iter != cache_.end() ){
                        return iter->second;
                }
                auto const& left{ holdem_class_decl::get(players[0]).get_hand_set() };
                auto const& right{ holdem_class_decl::get(players[1]).get_hand_set() };
                result_t ret;
                for( auto const& l : left ){
                        for( auto const& r : right ){
                                if( ! disjoint(l, r) )
                                        continue;
                                auto const& cr{ ec_.visit_boards(
                                        std::vector<ps::holdem_id>{l,r} ) };
                                ret.win  += cr.win;
                                ret.lose += cr.lose;
                               ret.draw += cr.draw;
                        }
                }
                cache_.insert(std::make_pair( players, ret) );
                return visit_boards(players);
        }
private:
        std::map<std::vector<ps::holdem_class_id>, result_t> cache_;
        equity_cacher ec_;
};
}

#if 0
void generate_hu_cache(){
        class_equity_cacher cec;

}
#endif
#endif

int main(){
        using namespace ps;
        using namespace ps::frontend;

        for(size_t i{0}; i != 13 * 13; ++ i){
                auto h{ holdem_class_decl::get(i) };
                PRINT_SEQ((i)(h)(ps::detail::to_string(h.get_hand_set())));
        } 
        auto _AKo{ holdem_class_decl::get("AKo")};
        auto _33{ holdem_class_decl::get("33") };
        auto _JTs{ holdem_class_decl::get("JTs") };

        class_equity_cacher ec;
        PRINT( ec.visit_boards(
                        std::vector<ps::holdem_class_id>{
                                _AKo, _JTs } ) );
        PRINT( ec.visit_boards(
                        std::vector<ps::holdem_class_id>{
                                _AKo, _JTs } ) );

}
