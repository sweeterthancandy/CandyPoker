#ifndef PS_EVAL_HOLDEM_CLASS_EVAL_CACHE_IMPL_H
#define PS_EVAL_HOLDEM_CLASS_EVAL_CACHE_IMPL_H

#include <fstream>

#include "ps/support/singleton_factory.h"
#include "ps/eval/holdem_class_eval_cache.h"
#include "ps/eval/equity_breakdown_matrix.h"


namespace ps{
        
struct holdem_class_eval_cache_impl : holdem_class_eval_cache{

        equity_breakdown* lookup(holdem_class_vector const& vec){
                auto iter = cache_.find(vec);
                if( iter == cache_.end())
                        return nullptr;
                return &iter->second;
        }
        void commit(holdem_class_vector vec, equity_breakdown const& breakdown){
                cache_.emplace(std::move(vec), breakdown);
        }

        void display()const{
                std::cout << "DISPLAY BEGIN\n";
                for( auto const& item : cache_ ){
                        PRINT(item.first);
                        std::cout << item.second << "\n";
                }
                std::cout << "DISPLAY END\n";
        }

        // work with std::lock_guard etc
        void lock(){
                mtx_.lock();
        }
        void unlock(){
                mtx_.unlock();
        }
        bool load(std::string const& name);
        bool save(std::string const& name);
        template<class Archive>
        void serialize(Archive& ar, unsigned int){
                ar & cache_;
        }
private:
        mutable std::mutex mtx_;
        std::map< holdem_class_vector, equity_breakdown_matrix> cache_;
};

using holdem_class_eval_cache_factory = support::singleton_factory<holdem_class_eval_cache>;

} // ps
#endif // PS_EVAL_HOLDEM_CLASS_EVAL_CACHE_IMPL_H
