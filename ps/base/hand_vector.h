#ifndef PS_HAND_SET_H
#define PS_HAND_SET_H

#include <vector>

#include "ps/base/cards_fwd.h"
#include "ps/detail/print.h"

namespace ps{


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
        };

        #if 0
        struct holdem_hand_vector_permutation : holdem_hand_vector{
                using perm_type = std::vector<int>;
                template<class... Args>
                holdem_hand_vector_permutation(perm_type perm, Args&&... args)
                        : holdem_hand_vector{ std::forwards<Args>(args)... }
                        , perms_(std::move(perm))
                {}
        private:
                std::vector<int> perm_;
        };
        #endif


        struct holdem_class_vector : std::vector<ps::holdem_class_id>{
                template<class... Args>
                holdem_class_vector(Args&&... args)
                : std::vector<ps::holdem_id>{std::forward<Args>(args)...}
                {}
                friend std::ostream& operator<<(std::ostream& ostr, holdem_class_vector const& self);
                holdem_class_decl const& decl_at(size_t i)const;
                std::vector< holdem_hand_vector > get_hand_vectors()const;
        };
} // ps

#endif // PS_HAND_SET_H
