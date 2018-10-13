#ifndef PS_EVAL_CLASS_CACHE_H
#define PS_EVAL_CLASS_CACHE_H


namespace boost{
namespace serialization{
        struct access;
} // end namespace serialization
} // end namespace boost

namespace ps{

struct equity_view : std::vector<double>{
        equity_view(matrix_t const& breakdown){
                sigma_ = 0;
                size_t n = breakdown.rows();
                std::map<long, unsigned long long> sigma_device;
                for( size_t i=0;i!=n;++i){
                        for(size_t j=0; j != n; ++j ){
                                sigma_device[j] += breakdown(j,i);
                        }
                }
                for( size_t i=0;i!=n;++i){
                        sigma_ += sigma_device[i] / ( i +1 );
                }


                for( size_t i=0;i!=n;++i){

                        double equity = 0.0;
                        for(size_t j=0; j != n; ++j ){
                                equity += breakdown(j,i) / ( j +1 );
                        }
                        equity /= sigma_;
                        push_back(equity);
                }
        }
        unsigned long long sigma()const{ return sigma_; }
private:
        unsigned long long sigma_;
};

struct class_cache{
        void add(std::vector<holdem_class_id> vec, std::vector<double> equity){
                cache_.emplace(std::move(vec), std::move(equity));
        }
	std::vector<double> const* Lookup(std::vector<holdem_class_id> const& vec)const{
                auto iter = cache_.find(vec);
                if( iter == cache_.end())
                        return nullptr;
                return &iter->second;
	}
        Eigen::VectorXd LookupVector(holdem_class_vector const& vec)const{
                #if 0
                if( std::is_sorted(vec.begin(), vec.end()) ){
                        Eigen::VectorXd tmp(vec.size());
                        auto ptr = Lookup(vec);
                        BOOST_ASSERT(pyt);
                        for(size_t idx=0;idx!=vec.size();++idx){
                                tmp(idx) = vec[idx];
                        }
                        return tmp;
                }
                #endif
                // find the permuation index
                std::array<
                        std::tuple<size_t, holdem_class_id>
                        , 9
                > aux;
                for(size_t idx=0;idx!=vec.size();++idx){
                        aux[idx] = std::make_tuple(idx, vec[idx]);
                }
                std::sort(aux.begin(), aux.begin() + vec.size(), [](auto const& l, auto const& r){
                          return std::get<1>(l) < std::get<1>(r);
                });

                // I think this is quicker than copying from aux
                auto copy = vec;
                std::sort(copy.begin(), copy.end());
                        
                // find the underlying
                auto ptr = Lookup(copy);
                BOOST_ASSERT(ptr);

                // copy to a vector
                Eigen::VectorXd tmp(vec.size());
                for(size_t idx=0;idx!=vec.size();++idx){
                        auto mapped_idx = std::get<0>(aux[idx]);
                        tmp(mapped_idx) = (*ptr)[idx];
                }
                return tmp;
        }
	void save(std::string const& filename);
	void load(std::string const& filename);
	#if 0
	void save(std::string const& filename){
		// make an archive
		std::ofstream ofs(filename);
		boost::archive::text_oarchive oa(ofs);
		oa << *this;
	}

	void load(std::string const& filename)
	{
		// open the archive
		std::ifstream ifs(filename);
		boost::archive::text_iarchive ia(ifs);

		// restore the schedule from the archive
		cache_.clear();
		ia >> *this;
        }
	#endif
	
private:
        friend class boost::serialization::access;
        template<class Archive>
        void serialize(Archive & ar, const unsigned int version){
                ar & cache_;
        }
private:
        std::map<std::vector<holdem_class_id>, std::vector<double> > cache_;
};

struct class_cache_maker{
        void create(size_t n, class_cache* cache, std::string const& file_name){
                computation_pass_manager mgr;
                mgr.add_pass<pass_class2cards>();
                mgr.add_pass<pass_permutate>();
                mgr.add_pass<pass_sort_type>();
                mgr.add_pass<pass_collect>();
                mgr.add_pass<pass_eval_hand_instr>();

                size_t count = 0;
                enum{ MaxCount = 50 };
                auto save = [&](){
                        std::cout << "Saving...\n";
                        cache->save(file_name);
                        std::cout << "Done\n";
                };
                for(holdem_class_iterator iter(n),end;iter!=end;++iter){
                        auto vec = *iter;
                        BOOST_ASSERT( vec.is_standard_form() );
                        if( cache->Lookup(vec) )
                                continue;
                        instruction_list instr_list;
                        instr_list.push_back(std::make_shared<class_vec_instruction>(vec));
                        computation_context comp_ctx{n};
                        auto result = mgr.execute(&comp_ctx, &instr_list);
                        BOOST_ASSERT( result );
                        equity_view view( *result );
                        enum{ Debug = true };
                        if( Debug ){
                                #if 0
                                std::vector<std::string> s;
                                for(auto _ : vec){
                                        s.push_back(holdem_class_decl::get(_).to_string());
                                }
                                pretty_print_equity_breakdown_mat(std::cout, *result, s);
                                #endif
                                std::cout << vec << " -> " << detail::to_string(view) << "\n";
                        }


			cache->add(vec, view);
                        if( ++count == MaxCount ){
                                count = 0;
                                save();
                        }
                }
                save();
        }
};

} // end namespace ps

#endif // PS_EVAL_CLASS_CACHE_H
