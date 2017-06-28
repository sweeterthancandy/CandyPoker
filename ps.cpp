#include "ps/heads_up.h"
#include "ps/detail/print.h"


int main(){
        using namespace ps;
        using namespace ps::frontend;

        for(size_t i{0}; i != 13 * 13; ++ i){
                auto h{ holdem_class_decl::get(i) };
                PRINT_SEQ((i)(h)(ps::detail::to_string(h.get_hand_set())));
        } 
        equity_cacher ec;
        ec.load("cache.bin");
        class_equity_cacher cec(ec);
        #if 0
        auto _AKo{ holdem_class_decl::parse("AKo")};
        auto _33 { holdem_class_decl::parse("33" )};
        auto _JTs{ holdem_class_decl::parse("JTs")};

        PRINT( cec.visit_boards(
                        std::vector<ps::holdem_class_id>{
                                _AKo, _JTs } ) );
                                #endif
        for(holdem_class_id x{0}; x != 169;++x){
                for(holdem_class_id y{0}; y != 169;++y){
                        auto hero{ holdem_class_decl::get(x) };
                        auto villian{ holdem_class_decl::get(y) };
                        PRINT_SEQ((hero)(villian)( cec.visit_boards(
                                        std::vector<ps::holdem_class_id>{
                                                x,y } ) ));
                }
        }

}
