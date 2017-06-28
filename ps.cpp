#include "ps/heads_up.h"

int main(){
        using namespace ps;
        using namespace ps::frontend;

        for(size_t i{0}; i != 13 * 13; ++ i){
                auto h{ holdem_class_decl::get(i) };
                PRINT_SEQ((i)(h));
        } 
        PRINT( holdem_class_decl::get("AKo") );
        PRINT( holdem_class_decl::get("33") );
        PRINT( holdem_class_decl::get("79s") );
}
