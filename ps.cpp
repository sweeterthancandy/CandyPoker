#include "ps/heads_up.h"
#include "ps/detail/print.h"


int main(){
        #if 0
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
                                #endif
        ps::generate_cache();

}
