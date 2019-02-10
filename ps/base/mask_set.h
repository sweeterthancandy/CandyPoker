#ifndef PS_BASE_MASK_SET_H
#define PS_BASE_MASK_SET_H


namespace ps{


struct mask_set{
        void add(size_t mask){
                masks_.push_back(mask);
        }
        size_t count_disjoint(size_t that)const noexcept{
                size_t count = 0;
                for(auto mask : masks_ ){
                        if( ( mask & that ) == 0 ){
                                ++count;
                        }
                }
                return count;
        }
        size_t size()const{ return masks_.size(); }
        friend bool operator<(mask_set const& l, mask_set const& r)noexcept{
                return l.masks_ < r.masks_;
        }
private:
        std::vector<size_t> masks_;
};



/*
        weirdly this is slower
 */
#if 0

struct mask_set{
        void add(size_t mask){
                masks_.push_back(mask);
                union_ |= mask;
        }
        size_t count_disjoint(size_t that)const noexcept{
                #if 1
                #if 0
                static size_t hit = 0;
                static size_t total = 0;
                ++total;
                #endif
                if( ( that & union_ ) == 0 ){
                        #if 0
                        ++hit;
                        std::cout << "hit => " << hit << "\n"; // __CandyPrint__(cxx-print-scalar,hit)
                        std::cout << "total => " << total << "\n"; // __CandyPrint__(cxx-print-scalar,total)
                        #endif
                        return size();
                }
                if( size() == 1 )
                        return 0;
                #endif
                size_t count = 0;
                for(auto mask : masks_ ){
                        if( ( mask & that ) == 0 ){
                                ++count;
                        }
                }
                return count;
        }
        size_t size()const{ return masks_.size(); }
private:
        std::vector<size_t> masks_;
        size_t union_{0};
};
#endif



} // end namespace ps

#endif // PS_BASE_MASK_SET_H
