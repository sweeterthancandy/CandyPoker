#ifndef PS_HOLDEM_RANGE_H
#define PS_HOLDEM_RANGE_H

namespace ps{
        struct holdem_range{

                void set(id_type hand){
                        assert( hand < 51 * 52 + 50 && "precondition failed");
                        //std::cout << boost::format("set(%s;%s)\n") % ht_.to_string(hand) % hand;
                        impl_.insert(hand);
                }
                void debug()const{
                        //boost::for_each( impl_, [&](auto _){
                                //std::cout << ht_.to_string(_) << "\n";
                        //});
                }
                std::vector<id_type> to_vector()const{
                        return std::vector<id_type>{impl_.begin(), impl_.end()};
                }
        private:
                card_traits ct_;
                std::set<id_type> impl_;
        };
}

#endif // PS_HOLDEM_RANGE_H
