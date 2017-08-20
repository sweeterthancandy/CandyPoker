#ifndef PS_BASE_SUIT_HASHER_H
#define PS_BASE_SUIT_HASHER_H

namespace ps{

struct suit_hasher{
        

        using hash_t = size_t;

        hash_t create(){ return 1; }
        hash_t append(hash_t hash, rank_id rank){
                static constexpr const std::array<int,4> suit_map = { 2,3,5,7 };
                return hash * suit_map[rank];
        }
};

} // ps

#endif // PS_BASE_SUIT_HASHER_H
