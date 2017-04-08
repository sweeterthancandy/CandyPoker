#include "ps/ps.h"
#include "ps/detail/print.h"

using namespace ps;
namespace{
void traits_test(){
        const char* cards [] = {
                "As", "2s", "3s", "4s", "5s", "6s", "7s", "8s", "9s", "Ts", "js", "qs", "Ks",
                "AD", "2D", "3D", "4D", "5D", "6D", "7D", "8D", "9D", "TD", "jD", "qD", "KD",
                "Ac", "2c", "3c", "4c", "5c", "6c", "7c", "8c", "9c", "Tc", "jc", "qc", "Kc",
                "Ah", "2h", "3h", "4h", "5h", "6h", "7h", "8h", "9h", "Th", "jh", "qh", "Kh" };

        card_traits t;
        for(long i=0;i!=sizeof(cards)/sizeof(void*);++i){
                long c = t.make(cards[i]);
                std::cout << cards[i] << " -> " << c << " " << t.rank(c) << " - " << t.suit(c) << "\n";
        }
        PRINT( t.make("Ac") );
        PRINT( t.make("5s") );
        PRINT( t.rank(t.make("Ah")) );
        PRINT( t.rank(t.make("Ac")) );
        PRINT( t.suit(t.make("Ah")) );
        PRINT( t.suit(t.make("Ac")) );
}

void eval_test(){
        driver d;

        PRINT( d.eval_5("AhKhQhJhTh") );
        PRINT( d.eval_5("AsKsQsJsTs") );
        PRINT( d.eval_5("AcKcQcJcTc") );
        PRINT( d.eval_5("AhAcAcAd2c") );
        PRINT( d.eval_5("AhAcAcAd2d") );

        //std::cout << d.calc("AcKs", "2s2d", "3d4c4dQdQs") << "\n";
        //std::cout << d.calc("AcKs", "2s2d", "3d4c4dQd") << "\n";
        //std::cout << d.calc("AcKs", "2s2d", "3d4c4d") << "\n";
        //std::cout << d.calc("AcKs", "2s2d", "3d4c") << "\n";
        //std::cout << d.calc("AcKs", "2s2d", "3d") << "\n";
        std::cout << d.calc("AcKs", "2s2d") << "\n";
}


void generate_test(){
        printer p;
        generate<card_traits>(p);

}
}
int main(){
        generate_test();
        traits_test();
        eval_test();
}
