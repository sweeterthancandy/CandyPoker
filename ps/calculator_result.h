#ifndef PS_CALCULATOR_RESULT_H
#define PS_CALCULATOR_RESULT_H

namespace ps{

template<size_t N>
struct detailed_result_type{
        detailed_result_type(detailed_result_type const&)=default;
        detailed_result_type():
                sigma_{0}
        {
                std::memset( data_.begin(), 0, sizeof(data_));
        }
        auto& data_access(size_t i, size_t j){
                return data_[i * N + j];
        }
        auto data_access(size_t i, size_t j)const{
                return data_[i * N + j];
        }
        size_t const* data()const{ return reinterpret_cast<size_t const*>(data_.begin()); }

        template<class Archive>
        void serialize(Archive& ar, unsigned int){
                ar & sigma_;
                ar & data_;
        }

        auto sigma()const{ return sigma_; }
        auto& sigma(){ return sigma_; }
        
private:
        size_t sigma_;

        // access float, so can have a static view for all 
        //  2,3,...9 etc without injeritace (see view_t)
        std::array<
                        size_t,
                N * N
        > data_;
};


template<size_t N>
struct detailed_observer_type{
        template<class Int, class Vec>
        void operator()(Int a, Int b, Int c, Int d, Int e, Vec const& ranked){
                /*
                        Here I need a quick way to work out the lowest rank,
                        as well as how many are of that rank, and I need to
                        find them. I think this is the quickest
                */
                auto lowest{ ranked[0] };
                size_t count{1};
                for(size_t i=1;i<ranked.size();++i){
                        if( ranked[i] == lowest ){
                                ++count;
                        } else if( ranked[i] < lowest ){
                                lowest = ranked[i]; 
                                count = 1;
                        }
                }
                for(size_t i=0;i!=ranked.size();++i){
                        if( ranked[i] == lowest ){
                                ++result.data_access(i,count-1);
                        }
                }
                ++result.sigma();
        }  
        template<class View_Type>
        void append(View_Type const& view){
                result.sigma() += view.sigma();
                for(size_t i=0;i!=N;++i){
                        for(size_t j=0;j!=N;++j){
                                result.data_access(i, j) += view.player(i).nwin(j);
                        }
                }
        }
        auto make(){ return result; }
private:
        detailed_result_type<N> result;
};



struct dyn_result_type{
        dyn_result_type(dyn_result_type const&)=default;
        dyn_result_type(size_t n):
                sigma_{0},
                n_(n)
        {
                data_.resize(n_*n_);
        }
        auto& data_access(size_t i, size_t j){
                return data_[i * n_ + j];
        }
        auto data_access(size_t i, size_t j)const{
                return data_[i * n_ + j];
        }
        size_t const* data()const{ return reinterpret_cast<size_t const*>(&data_.front()); }

        template<class Archive>
        void serialize(Archive& ar, unsigned int){
                ar & sigma_;
                ar & data_;
        }

        auto sigma()const{ return sigma_; }
        auto& sigma(){ return sigma_; }

        auto n()const{ return n_; }
        
private:
        size_t sigma_;
        size_t n_;

        // access float, so can have a static view for all 
        //  2,3,...9 etc without injeritace (see view_t)
        std::vector< size_t > data_;
};


struct dyn_observer_type{
        explicit dyn_observer_type(size_t n):result(n){}
        template<class Int, class Vec>
        void operator()(Int a, Int b, Int c, Int d, Int e, Vec const& ranked){
                /*
                        Here I need a quick way to work out the lowest rank,
                        as well as how many are of that rank, and I need to
                        find them. I think this is the quickest
                */
                auto lowest{ ranked[0] };
                size_t count{1};
                for(size_t i=1;i<ranked.size();++i){
                        if( ranked[i] == lowest ){
                                ++count;
                        } else if( ranked[i] < lowest ){
                                lowest = ranked[i]; 
                                count = 1;
                        }
                }
                for(size_t i=0;i!=ranked.size();++i){
                        if( ranked[i] == lowest ){
                                ++result.data_access(i,count-1);
                        }
                }
                ++result.sigma();
        }  
        template<class View_Type>
        void append(View_Type const& view){
                result.sigma() += view.sigma();
                for(size_t i=0;i!=result.n();++i){
                        for(size_t j=0;j!=result.n();++j){
                                result.data_access(i, j) += view.player(i).nwin(j);
                        }
                }
        }
        auto make(){ return result; }
private:
        dyn_result_type result;
};

} // ps

#endif // PS_CALCULATOR_RESULT_H
