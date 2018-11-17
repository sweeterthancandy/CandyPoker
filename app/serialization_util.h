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
                        for(auto const& d : state){
                                std::vector<std::vector<double> > y;
                                for(auto const& c : d){
                                        std::vector<double> z(c.size());
                                        for(size_t idx=0;idx!=c.size();++idx){
                                                z[idx] = c[idx];
                                        }
                                        y.push_back(z);
                                }
                                state_.push_back(y);
                        }

                        std::cerr << "state_[0].size() => " << state_[0].size() << "\n"; // __CandyPrint__(cxx-print-scalar,state_[0].size())
                        std::cerr << "state_[0][0].size() => " << state_[0][0].size() << "\n"; // __CandyPrint__(cxx-print-scalar,state_[0][0].size())
                        std::cerr << "state_[0][1].size() => " << state_[0][1].size() << "\n"; // __CandyPrint__(cxx-print-scalar,state_[0][1].size())
                        std::cerr << "state_[1].size() => " << state_[1].size() << "\n"; // __CandyPrint__(cxx-print-scalar,state_[1].size())
                        std::cerr << "state_[1][0].size() => " << state_[1][0].size() << "\n"; // __CandyPrint__(cxx-print-scalar,state_[1][0].size())
                        std::cerr << "state_[1][1].size() => " << state_[1][1].size() << "\n"; // __CandyPrint__(cxx-print-scalar,state_[1][1].size())
                }
                std::vector<std::vector<Eigen::VectorXd> > to_eigen()const{
                        std::cerr << "state_[0].size() => " << state_[0].size() << "\n"; // __CandyPrint__(cxx-print-scalar,state_[0].size())
                        std::cerr << "state_[0][0].size() => " << state_[0][0].size() << "\n"; // __CandyPrint__(cxx-print-scalar,state_[0][0].size())
                        std::cerr << "state_[0][1].size() => " << state_[0][1].size() << "\n"; // __CandyPrint__(cxx-print-scalar,state_[0][1].size())
                        std::cerr << "state_[1].size() => " << state_[1].size() << "\n"; // __CandyPrint__(cxx-print-scalar,state_[1].size())
                        std::cerr << "state_[1][0].size() => " << state_[1][0].size() << "\n"; // __CandyPrint__(cxx-print-scalar,state_[1][0].size())
                        std::cerr << "state_[1][1].size() => " << state_[1][1].size() << "\n"; // __CandyPrint__(cxx-print-scalar,state_[1][1].size())
                        std::vector<std::vector<Eigen::VectorXd> > x;
                        fprintf(stderr, "A\n"); // __CandyTag__ 
                        for(auto const& d : state_){
                                fprintf(stderr, "B\n"); // __CandyTag__ 
                                std::vector<Eigen::VectorXd> y;
                                fprintf(stderr, "C\n"); // __CandyTag__ 
                                for(auto const& c : d){
                                        fprintf(stderr, "D\n"); // __CandyTag__ 
                                        //Eigen::VectorXd z(c.size());
                                        //fprintf(stderr, "E\n"); // __CandyTag__ 
                                        y.emplace_back();
                                        fprintf(stderr, "F\n"); // __CandyTag__ 
                                        y.back().resize(c.size());
                                        fprintf(stderr, "G\n"); // __CandyTag__ 
                                        for(size_t idx=0;idx!=c.size();++idx){
                                                fprintf(stderr, "H\n"); // __CandyTag__ 
                                                y.back()[idx] = c[idx];
                                                fprintf(stderr, "I\n"); // __CandyTag__ 
                                        }
                                        fprintf(stderr, "J\n"); // __CandyTag__ 
                                }
                                fprintf(stderr, "K\n"); // __CandyTag__ 
                                x.push_back(y);
                                fprintf(stderr, "L\n"); // __CandyTag__ 
                        }
                        fprintf(stderr, "M\n"); // __CandyTag__ 
                        std::cerr << "x[0].size() => " << x[0].size() << "\n"; // __CandyPrint__(cxx-print-scalar,x[0].size())
                        std::cerr << "x[0][0].size() => " << x[0][0].size() << "\n"; // __CandyPrint__(cxx-print-scalar,x[0][0].size())
                        std::cerr << "x[0][1].size() => " << x[0][1].size() << "\n"; // __CandyPrint__(cxx-print-scalar,x[0][1].size())
                        std::cerr << "x[1].size() => " << x[1].size() << "\n"; // __CandyPrint__(cxx-print-scalar,x[1].size())
                        std::cerr << "x[1][0].size() => " << x[1][0].size() << "\n"; // __CandyPrint__(cxx-print-scalar,x[1][0].size())
                        std::cerr << "x[1][1].size() => " << x[1][1].size() << "\n"; // __CandyPrint__(cxx-print-scalar,x[1][1].size())
                        return x;
                }
        private:
                friend class boost::serialization::access;
                template<class Archive>
                void serialize(Archive & ar, const unsigned int version){
                        ar & state_;
                }
        private:
                std::vector< std::vector< std::vector<double> > > state_;
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
