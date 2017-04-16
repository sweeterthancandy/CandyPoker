#ifndef PS_HOLDEM_RANGE_H
#define PS_HOLDEM_RANGE_H

namespace ps{
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
}

#endif // PS_HOLDEM_RANGE_H
