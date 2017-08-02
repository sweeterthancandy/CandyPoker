#ifndef PS_CARDS_FWD_H
#define PS_CARDS_FWD_H

namespace ps{

        #if 0
        using id_type         =  std::uint16_t;

        using suit_id         = std::uint8_t;
        using rank_id         = std::uint8_t; 
        using card_id         = std::uint8_t; 
        using holdem_id       = std::uint16_t;
        using holdem_class_id = std::uint8_t; 
        #else

        using id_type =  unsigned;

        using suit_id   = unsigned;
        using rank_id   = unsigned;
        using card_id   = unsigned;
        using holdem_id = unsigned;
        using holdem_class_id = unsigned;

        #endif


        enum class suit_category{
                any_suit,
                suited,
                offsuit
        };
        
        enum class holdem_class_type{
                pocket_pair,
                suited,
                offsuit
        };


        struct suit_decl;
        struct rank_decl;
        struct holdem_hand_decl;
        struct holdem_class_decl;
} // ps

#endif // PS_CARDS_FWD_H
