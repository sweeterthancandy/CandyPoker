#ifndef PS_HOLDEM_HASHER_H
#define PS_HOLDEM_HASHER_H

namespace ps{

        std::string suit_hash(std::vector<id_type>const& hands, std::vector<id_type> const& board = std::vector<id_type>{} ){
                using std::get;
                std::string hash;

                for( id_type h : hands ){
                        hash += holdem_hand_decl::get(h).to_string();
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
                return hash;
        }
}

#endif
