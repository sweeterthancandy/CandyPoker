#ifndef APP_SERIALIZATION_UTIL_H
#define APP_SERIALIZATION_UTIL_H

#include <mutex>
#include <fstream>
#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/text_oarchive.hpp>
#include <boost/archive/binary_iarchive.hpp>
#include <boost/archive/binary_oarchive.hpp>
#include <boost/serialization/base_object.hpp>
#include <boost/serialization/utility.hpp>
#include <boost/serialization/map.hpp>
#include <boost/serialization/vector.hpp>
#include <boost/serialization/assume_abstract.hpp>

namespace ps{
        // basically a wrapper class
        struct holdem_binary_strategy{
                holdem_binary_strategy()=default;
                /* implicit */ holdem_binary_strategy(std::vector<std::vector<Eigen::VectorXd> > const& state)
                {
                        std::vector<Eigen::VectorXd const*> flat_view;
                        for(auto const& d : state){
                                dims_.push_back(d.size());
                                for(auto const& c : d){
                                        std::vector<double> v;
                                        for(size_t k=0;k!=169;++k){
                                                v[k] = c[k];
                                        }
                                        state_.push_back( v );
                                }
                        }
                }
                std::vector<std::vector<Eigen::VectorXd> > to_eigen()const{
                        std::vector<std::vector<Eigen::VectorXd> > tmp(dims_.size());
                        size_t state_index = 0;
                        for(size_t idx=0;idx!=dims_.size();++idx){
                                tmp[idx].resize(dims_[idx]);
                                for(size_t j =0;j != dims_[idx];++j ){
                                        tmp[idx][j].resize(169);
                                        for(size_t k=0;k!=169;++k){
                                                tmp[idx][j][k] = state_[state_index][k];
                                        }
                                        ++state_index;
                                }
                        }
                        return tmp;
                }
        private:
                friend class boost::serialization::access;
                template<class Archive>
                void serialize(Archive & ar, const unsigned int version){
                        ar & dims_;
                        ar & state_;
                }
        private:
                std::vector<size_t> dims_;
                std::vector< std::vector<double> > state_;
        };

        template<class ImplType>
        struct serialization_base{
                void load(std::string const& path){
                        std::lock_guard<std::mutex> lock(mtx_);
                        using archive_type = boost::archive::text_iarchive;
                        std::ifstream ofs(path);
                        archive_type oa(ofs);
                        oa >> *reinterpret_cast<ImplType*>(this);
                        path_ = path;
                }
                // returns true indicte that load from disk
                // returns false to indicate that an empty object represention
                //         was created and loaded from disk
                bool try_load_or_default(std::string const& path){
                        try{
                                load(path);
                                return true;
                        }catch(...){
                                // clear it
                                auto typed = reinterpret_cast<ImplType*>(this);
                                typed->~ImplType();
                                new(typed)ImplType;
                                // now write to disk
                                save_as(path);
                                // no load it again so any error is apprent now
                                load(path);
                                return false;
                        }
                }
                void save_as(std::string const& path)const {
                        std::lock_guard<std::mutex> lock(mtx_);
                        using archive_type = boost::archive::text_oarchive;
                        std::ofstream ofs(path);
                        archive_type oa(ofs);
                        oa << *reinterpret_cast<ImplType const*>(this);
                }
                void save_()const{
                        if( path_.size() ){
                                save_as(path_);
                        }
                }
        private:
                mutable std::mutex mtx_;
                std::string path_;
        };

        struct holdem_binary_strategy_ledger : serialization_base<holdem_binary_strategy_ledger>{
                void push(holdem_binary_strategy s){
                        ledger_.emplace_back(std::move(s));
                }
                size_t size()const{ return ledger_.size(); }
                auto const& back()const{ return ledger_.back(); }
        private:
                friend class boost::serialization::access;
                template<class Archive>
                void serialize(Archive & ar, const unsigned int version){
                        ar & ledger_;
                }
        private:
                std::vector<holdem_binary_strategy> ledger_;
        };

        struct holdem_binary_solution_set : serialization_base<holdem_binary_solution_set>{
                void add_solution(std::string const& key, holdem_binary_strategy solution){
                        std::lock_guard<std::mutex> lock(mtx_);
                        solutions_.emplace(key, std::move(solution));
                }
                // there are no thread safe
                auto begin()const{ return solutions_.begin(); }
                auto end()const{ return solutions_.end(); }
                auto find(std::string const& key)const{ return solutions_.find(key); }
        private:
                friend class boost::serialization::access;
                template<class Archive>
                void serialize(Archive & ar, const unsigned int version){
                        ar & solutions_;
                }
        private:
                std::mutex mtx_;
                // in our scope we can represent everyting in a string, ie
                // we might want to encode the sb:bb:eff:stragery etc here
                std::map<std::string, holdem_binary_strategy> solutions_;
        };

} // end namespace ps

#endif // APP_SERIALIZATION_UTIL_H
