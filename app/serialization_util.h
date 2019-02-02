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
        struct holdem_binary_strategy_s{
                holdem_binary_strategy_s()=default;
                /* implicit */ holdem_binary_strategy_s(std::vector<Eigen::VectorXd> const& state){
                        for(auto const& vec : state){
                                state_.emplace_back();
                                state_.back().resize(vec.size());
                                for(size_t idx=0;idx!=vec.size();++idx){
                                        state_.back()[idx] = vec[idx];
                                }
                        }
                }
                /* implicit */ holdem_binary_strategy_s(std::vector<std::vector<Eigen::VectorXd> > const& state){
                        for(auto const& q : state){
                                auto const& vec = q[0];
                                state_.emplace_back();
                                state_.back().resize(vec.size());
                                for(size_t idx=0;idx!=vec.size();++idx){
                                        state_.back()[idx] = vec[idx];
                                }
                        }
                }
                std::vector<Eigen::VectorXd> to_eigen()const{
                        std::vector<Eigen::VectorXd> tmp;
                        for(auto const& v : state_){
                                Eigen::VectorXd ev(v.size());
                                for(size_t idx=0;idx!=v.size();++idx){
                                        ev[idx] = v[idx];
                                }
                                tmp.push_back(std::move(ev));
                        }
                        return tmp;
                }
                std::vector<std::vector<Eigen::VectorXd> > to_eigen_vv()const{
                        auto v = to_eigen();
                        std::vector<std::vector<Eigen::VectorXd> > tmp;
                        for(auto const& _ : v){
                                tmp.emplace_back(2);
                                tmp.back()[0] = _;
                                tmp.back()[1] = Eigen::VectorXd::Ones(169) - _;
                        }
                        return tmp;
                }
                bool good()const{ return state_.size() != 0; }

        private:
                friend class boost::serialization::access;
                template<class Archive>
                void serialize(Archive & ar, const unsigned int version){
                        ar & state_;
                }
        private:
                std::vector< std::vector<double> > state_;
        };

        template<class ImplType>
        struct serialization_base_s{
                void load(std::string const& path){
                        //std::lock_guard<std::mutex> lock(mtx_);
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
                        //std::lock_guard<std::mutex> lock(mtx_);
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
                //mutable std::mutex mtx_;
                std::string path_;
        };

        struct holdem_binary_strategy_ledger_s
                : serialization_base_s<holdem_binary_strategy_ledger_s>
                , std::vector<holdem_binary_strategy_s>
        {
                using vector_type = std::vector<holdem_binary_strategy_s>;

        private:
                friend class boost::serialization::access;
                template<class Archive>
                void serialize(Archive & ar, const unsigned int version){
                        auto ptr = static_cast<vector_type*>(this);
                        ar & *ptr;
                }
        };

        struct holdem_binary_solution_set_s : serialization_base_s<holdem_binary_solution_set_s>{
                void add_solution(std::string const& key, holdem_binary_strategy_s solution){
                        std::lock_guard<std::mutex> lock(mtx_);
                        solutions_[key] = std::move(solution);
                }
                bool remove_solution(std::string const& key){
                        std::lock_guard<std::mutex> lock(mtx_);
                        auto iter = solutions_.find(key);
                        if( iter == solutions_.end())
                                return false;
                        solutions_.erase(iter);
                        return true;
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
                std::map<std::string, holdem_binary_strategy_s> solutions_;
        };

} // end namespace ps

#endif // APP_SERIALIZATION_UTIL_H
