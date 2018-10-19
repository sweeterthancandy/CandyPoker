#ifndef PS_CORE_CARDS_H
#define PS_CORE_CARDS_H

#include <vector>
#include <set>
#include <map>


#include "ps/base/cards_fwd.h"
#include "ps/detail/void_t.h"
#include "ps/detail/print.h"
#include "ps/support/index_sequence.h"


namespace ps{
        
        struct card_vector : std::vector<card_id>{
                template<class... Args>
                card_vector(Args&&... args):std::vector<card_id>{std::forward<Args>(args)...}{}

                size_t mask()const{
                        size_t m{0};
                        for( auto id : *this ){
                                m |= ( static_cast<size_t>(1) << id );
                        }
                        return m;
                }
                static inline card_vector from_bitmask(size_t mask);

                friend inline std::ostream& operator<<(std::ostream& ostr, card_vector const& self);
        };

        struct suit_decl{
                suit_decl(suit_id id, char sym, std::string const& name)
                        : id_{id}, sym_{sym}, name_{name}
                {}
                auto id()const{ return id_; }
                std::string to_string()const{ return std::string{sym_}; }
                friend std::ostream& operator<<(std::ostream& ostr, suit_decl const& self){
                        return ostr << self.to_string();
                }
                bool operator<(suit_decl const& that)const{
                        return id_ < that.id_;
                }
                static suit_decl const& get(suit_id id);
                static suit_decl const& parse(std::string const& s);
                static suit_decl const& parse(char c){ return parse(std::string{c}); }
                operator suit_id()const{ return id_; }
        private:
                suit_id id_;
                char sym_;
                std::string name_;
        };

        struct rank_decl{
                rank_decl(rank_id id, char sym)
                        : id_{id}, sym_{sym}
                {}
                auto id()const{ return id_; }
                std::string to_string()const{ return std::string{sym_}; }
                friend std::ostream& operator<<(std::ostream& ostr, rank_decl const& self){
                        return ostr << self.to_string();
                }
                bool operator<(rank_decl const& that)const{
                        return id_ < that.id_;
                }
                static rank_decl const& get(rank_id id);
                static rank_decl const& parse(std::string const& s);
                static rank_decl const& parse(char c){ return parse(std::string{c}); }
                operator rank_id()const{ return id_; }
        private:
                rank_id id_;
                char sym_;
        };


        struct card_decl{
                card_decl( suit_decl const& s, rank_decl const& r):
                        id_{make_id(s.id(),r.id())}
                        ,suit_{s}, rank_{r}
                {}
                auto id()const{ return id_; }
                size_t mask()const{ return static_cast<size_t>(1) << id(); }
                std::string to_string()const{
                        return rank_.to_string() + 
                               suit_.to_string();
                }
                // id & 0x3
                suit_decl const& suit()const{ return suit_; }
                // id >> 2
                rank_decl const& rank()const{ return rank_; }
                friend std::ostream& operator<<(std::ostream& ostr, card_decl const& self){
                        return ostr << self.to_string();
                }
                bool operator<(card_decl const& that)const{
                        return id_ < that.id_;
                }
                static card_decl const& get(card_id id);
                inline static card_decl const& parse(std::string const& s){
                        assert( s.size() == 2 && "precondition failed");
                        return get( rank_decl::parse(s.substr(0,1)).id() * 4 +
                                    suit_decl::parse(s.substr(1,1)).id()   );
                }
                operator card_id()const{ return id_; }
                static card_id make_id( suit_id s, rank_id r){
                        return s + r * 4;
                }
        private:
                card_id id_;
                suit_decl suit_;
                rank_decl rank_;
        };

        struct holdem_hand_decl{
                static constexpr const holdem_id max_id = 52 * 51 / 2;
                // a must be the biggest
                holdem_hand_decl( card_decl const& a, card_decl const& b):
                        id_{ make_id(a.id(), b.id()) },
                        first_{a},
                        second_{b}
                {
                }
                auto id()const{ return id_; }
                size_t mask()const{
                        return first().mask() | second().mask();
                }
                std::string to_string()const{
                        return first_.to_string() + 
                               second_.to_string();
                }
                friend std::ostream& operator<<(std::ostream& ostr, holdem_hand_decl const& self){
                        return ostr << self.to_string();
                }
                bool operator<(holdem_hand_decl const& that)const{
                        return id_ < that.id_;
                }
                card_decl const& first()const{ return first_; }
                card_decl const& second()const{ return second_; }
                static holdem_hand_decl const& get(holdem_id id);
                static holdem_hand_decl const& get(card_id x, card_id y){
                        return get( make_id(x,y) );
                }
                static holdem_hand_decl const& parse(std::string const& s);
                static holdem_id make_id( rank_id r0, suit_id s0, rank_id r1, suit_id s1);
                inline static holdem_id make_id( card_id x, card_id y){
                        static std::array<holdem_id, 52*52> proto{
                                [](){
                                        size_t id{0};
                                        std::array<holdem_id, 52*52> result;
                                        for( char a{52};a!=1;){
                                                --a;
                                                for( char b{a};b!=0;){
                                                        --b;
                                                        result[a * 52 + b ] = id;
                                                        result[b * 52 + a ] = id;
                                                        ++id;
                                                }
                                        }
                                        //PRINT_SEQ((id)((52*52-52)/2));
                                        return std::move(result);
                                }()
                        };
                        assert( x < 52 && "precondition failed");
                        assert( y < 52 && "precondition failed");
                        return proto[x * 52 + y];
                }
                operator holdem_id()const{ return id_; }
                holdem_class_id class_()const;
                static double prob(holdem_id c0, holdem_id c1);
                template<class... Args,
                        class _ = detail::void_t<
                                        std::enable_if_t<
                                                std::is_same<std::decay_t<Args>, holdem_hand_decl>::value>...
                        >
                >
                static bool disjoint(Args&&... args){
                        size_t mask{0};
                        int aux[] = {0, ( mask |= args.mask(), 0)...};
                        return __builtin_popcount(mask)*2 == sizeof...(args);
                }
                template<class... Args,
                        class _ = detail::void_t<
                                        std::enable_if_t<
                                                std::is_integral<std::decay_t<Args> >::value>...
                        >
                >
                static bool disjoint_id(Args&&... args){
                        return disjoint( holdem_hand_decl::get(args)... );
                }
        private:
                holdem_id id_;
                card_decl first_;
                card_decl second_;

        };
        
        /*
                Hand vector is a vector of hands
         */
        struct holdem_hand_vector : std::vector<ps::holdem_id>{
                template<class... Args>
                holdem_hand_vector(Args&&... args)
                : std::vector<ps::holdem_id>{std::forward<Args>(args)...}
                {}
                holdem_hand_decl const& decl_at(size_t i)const;
                friend std::ostream& operator<<(std::ostream& ostr, holdem_hand_vector const& self);
                auto find_injective_permutation()const;
                bool disjoint()const;
                bool is_standard_form()const;
                size_t mask()const;
                card_vector to_card_vector()const;
        };

        struct holdem_hand_iterator :
                basic_index_iterator<
                        holdem_id,
                        strict_lower_triangle_policy,
                        holdem_hand_vector
                >
        {
                using impl_t = 
                        basic_index_iterator<
                                holdem_id,
                                strict_lower_triangle_policy,
                                holdem_hand_vector
                        >
                ;
                holdem_hand_iterator():impl_t{}{}
                holdem_hand_iterator(size_t n):
                        impl_t(n, 52 * 51 / 2)
                {}
        };

        struct holdem_class_decl{
                static constexpr const holdem_class_id max_id = 13 * 13;
                holdem_class_decl(holdem_class_type cat,
                                  rank_decl const& a,
                                  rank_decl const& b);
                auto const& get_hand_set()const{ return hand_set_; }
                auto const& get_hand_vector()const{ return hand_id_set_; }
                auto id()const{ return id_; }
                auto category()const{ return cat_; }
                std::string to_string()const;
                friend std::ostream& operator<<(std::ostream& ostr, holdem_class_decl const& self){
                        return ostr << self.to_string();
                }
                bool operator<(holdem_class_decl const& that)const{
                        return id_ < that.id_;
                }
                size_t weight()const{
                        switch(cat_){
                        case holdem_class_type::suited:
                                return 4;
                        case holdem_class_type::offsuit:
                                return 12;
                        case holdem_class_type::pocket_pair:
                                return 6;
                        }
                }
                double prob()const{
                        constexpr size_t half{ ( 13 * 13 - 13 ) / 2 };
                        constexpr size_t sigma{ 
                                13   *  6 +
                                half * 12 +
                                half * 4
                        };
                        return static_cast<double>(weight()) / sigma;
                }
                decltype(auto) first()const{ return first_; }
                decltype(auto) second()const{ return second_; }
                static holdem_class_decl const& get(holdem_id id);
                static holdem_class_decl const& parse(std::string const& s);
                // any bijection will do, nice to keep the mapping within a char
                static holdem_class_id make_id(holdem_class_type cat, rank_id x, rank_id y);
                operator holdem_class_id()const{ return id_; }

                // TODO generlaize these
                static size_t weight(holdem_class_id c0, holdem_class_id c1);
                static double prob(holdem_class_id c0, holdem_class_id c1);

        private:
                holdem_class_id id_;
                holdem_class_type cat_;
                rank_decl first_;
                rank_decl second_;
                std::vector<holdem_hand_decl> hand_set_;
                std::vector<holdem_id> hand_id_vec_;
                holdem_hand_vector hand_id_set_;
        };


        struct rank_vector : std::vector<rank_id>{
                template<class... Args>
                rank_vector(Args&&... args):std::vector<rank_id>{std::forward<Args>(args)...}{}

                friend std::ostream& operator<<(std::ostream& ostr, rank_vector const& self);
        };
        struct holdem_class_range : std::vector<ps::holdem_class_id>{
                template<
                        class... Args,
                        class = std::enable_if_t< ! std::is_constructible<std::string, Args...>::value  >
                >
                holdem_class_range(Args&&... args)
                : std::vector<ps::holdem_id>{std::forward<Args>(args)...}
                {}
                holdem_class_range(std::string const& item);
                friend std::ostream& operator<<(std::ostream& ostr, holdem_class_range const& self);
                void parse(std::string const& item);
        };
        
        
        struct holdem_class_vector : std::vector<ps::holdem_class_id>{
                template<class... Args>
                holdem_class_vector(Args&&... args)
                : std::vector<ps::holdem_class_id>{std::forward<Args>(args)...}
                {}
                friend std::ostream& operator<<(std::ostream& ostr, holdem_class_vector const& self);
                holdem_class_decl const& decl_at(size_t i)const;
                std::vector< holdem_hand_vector > get_hand_vectors()const;

                std::string to_string()const{
                        std::stringstream sstr;
                        sstr << *this;
                        return sstr.str();
                }

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


                std::tuple<
                        std::vector<int>,
                        holdem_class_vector
                > 
                to_standard_form()const;
                
                std::vector<
                       std::tuple< std::vector<int>, holdem_hand_vector >
                > to_standard_form_hands()const;
                
                bool is_standard_form()const;

                auto prob()const{
                        BOOST_ASSERT(size() == 2 );
                        return holdem_class_decl::prob(at(0), at(1));
                }
        };
        
        struct holdem_class_iterator :
                basic_index_iterator<
                holdem_class_id,
                ordered_policy,
                holdem_class_vector
                >
        {
                using impl_t = 
                        basic_index_iterator<
                                holdem_class_id,
                                ordered_policy,
                                holdem_class_vector
                        >
                        ;
                holdem_class_iterator():impl_t{}{}
                holdem_class_iterator(size_t n):
                        impl_t(n, holdem_class_decl::max_id)
                {}
        };
        struct holdem_class_perm_iterator :
                basic_index_iterator<
                holdem_class_id,
                range_policy,
                holdem_class_vector
                >
        {
                using impl_t = 
                        basic_index_iterator<
                                holdem_class_id,
                                range_policy,
                                holdem_class_vector
                        >
                        ;
                holdem_class_perm_iterator():impl_t{}{}
                holdem_class_perm_iterator(size_t n):
                        impl_t(n, holdem_class_decl::max_id)
                {}
        };
        typedef std::tuple< std::vector<int>, holdem_class_vector> standard_form_result;
        inline std::ostream& operator<<(std::ostream& ostr, standard_form_result const& self){
                return ostr << detail::to_string(std::get<0>(self))
                        << " x "
                        << std::get<1>(self);
        }
        
        typedef std::tuple< std::vector<int>, holdem_hand_vector> standard_form_hands_result;
        inline std::ostream& operator<<(std::ostream& ostr, standard_form_hands_result const& self){
                return ostr << detail::to_string(std::get<0>(self))
                        << " x "
                        << std::get<1>(self);
        }
        
        struct holdem_class_range_vector : std::vector<holdem_class_range>{
                template<class... Args>
                holdem_class_range_vector(Args&&... args)
                : std::vector<holdem_class_range>{std::forward<Args>(args)...}
                {}
                friend std::ostream& operator<<(std::ostream& ostr, holdem_class_range_vector const& self);

                void push_back(std::string const& s);

                // Return this expand, ie 
                //        {{AA,KK},{22}} => {AA,22}, {KK,22}
                std::vector<holdem_class_vector> get_cross_product()const;
                // Returns this as a vector of
                //        (matrix, standard-form-hand-vector)
                std::vector<
                       std::tuple< std::vector<int>, holdem_hand_vector >
                > to_standard_form()const;
                // Returns this as a vector of
                //        (matrix, standard-form-class-vector)
                std::vector<
                       std::tuple< std::vector<int>, holdem_class_vector >
                > to_class_standard_form()const;

        };

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
        
        
        std::ostream& operator<<(std::ostream& ostr, card_vector const& self){
                return ostr << detail::to_string(self, [](auto id){
                        return card_decl::get(id).to_string();
                });
        }
        card_vector card_vector::from_bitmask(size_t mask){
                card_vector vec;
                for(size_t i=0;i!=52;++i){
                        if( mask & card_decl::get(i).mask() ){
                                vec.push_back(i);
                        }
                }
                return std::move(vec);
        }
                

} // end namespace cards

#include "ps/base/decl.h"

#endif // PS_CORE_CARDS_H
