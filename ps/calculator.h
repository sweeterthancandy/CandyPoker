#ifndef PS_CALCULATOR_H
#define PS_CALCULATOR_H

namespace ps{

/*
        For equity calculations, creating pre-computation
        databases is important, which leads to a situation of
        a database of every hand vs hand situation, where
        for every unique hand vs hand we have a 3-tuple
        (win,draw,lose). For hand vs hand we can trivally 
        take into account the case where
                        a vs b -> (win,draw,lose)
                =>      b vs a -> (lose,draw,win).
        However, for n vs n players, each unique situation
        will create a n x n matrix of results, and the size 
        of the cache created might be too large.
                Also, most of our calculation is only
        concerned with the equity% of the hand, which leads
        to a situation where we want different information
        from the calculator.
                The idea of creating an N parameter for
        each structure, is that if I was to cache every
        2-9 player calculations, without thinking too much
        I would of thought it would be more effiecent to
        split up each n-player calculation by it'self



*/

namespace detail{

/*
        Win Draw Draw2 Draw3 Draw4 Draw5 Draw6 Draw7 Draw8 Draw9
 */
template<class Int_Type, class N>
struct basic_detailed_calculation_result{
        Int_Type sigma;
        std::array<
                std::array<
                        Int_Type,
                        N
                >,
                N
        > data;

        template<class Int, class Vec>
        void operator()(Int a, Int b, Int c, Int d, Int e, Vec const& ranked){
                /*
                        Here I need a quick way to work out the lowest rank,
                        as well as how many are of that rank, and I need to
                        find them. I think this is the quickest
                */
                auto lowest{ ranked[0] };
                size_t count{0};
                for(size_t i=1;i!=ranked.size();++i){
                        if( ranked[i] == lowest )
                                ++count;
                        else if( ranked[i] < lowest ){
                                lowest = *iter;
                                count = 1;
                        }
                }
                for(size_t i=0;i!=ranked.size();++i){
                        if( ranked[i] == lowest ){
                                data[
                        }
                }
                
                 
        }  

};



#if 0
template<class Float_Type, class N>
struct basic_detailed_calculation_result{
        Int_Type sigma;
        std::array<
                Float_Type,
                N
        > data;
};
#endif

} // detail

template<class Result_Type>
struct calculator_N{
};




}

#endif // PS_CALCULATOR_H
