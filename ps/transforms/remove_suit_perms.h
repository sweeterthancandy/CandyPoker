#ifndef PS_TRANSFORM_REMOVE_SUIT_PERMS_H
#define PS_TRANSFORM_REMOVE_SUIT_PERMS_H

namespace ps{
namespace transforms{

        struct remove_suit_perms : symbolic_transform{
                remove_suit_perms():symbolic_transform{"remove_suit_perms"}{}
                bool apply(symbolic_computation::handle& ptr)override{
                        if( ptr->get_kind() != symbolic_computation::Kind_Symbolic_Suit_Perm )
                                return false;
                        auto aux_ptr{ reinterpret_cast<symbolic_suit_perm*>(ptr.get()) };
                        assert( aux_ptr->get_children().size() == 1 && "unexpected");
                        auto child = aux_ptr->get_children().front();
                        ptr = child;
                        return true;
                }
        };
} // transform
} // ps

#endif // PS_TRANSFORM_REMOVE_SUIT_PERMS_H
