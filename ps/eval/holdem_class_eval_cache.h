#ifndef PS_EVAL_HOLDEM_CLASS_EVAL_CACHE_H
#define PS_EVAL_HOLDEM_CLASS_EVAL_CACHE_H


#include "ps/support/singleton_factory.h"
#include "ps/base/holdem_class_vector.h"
#include "ps/eval/equity_breakdown.h"
#include "ps/eval/equity_breakdown_matrix.h"

namespace ps{
        
struct holdem_class_eval_cache{
        virtual equity_breakdown* lookup(holdem_class_vector const& vec)=0;

        // this is a different mechinism, handles the locking it self 
        std::shared_ptr<equity_breakdown> try_lookup_perm(holdem_class_vector const& vec){
                auto t = vec.to_standard_form();
                this->lock();
                auto ptr = this->lookup( std::get<1>(t) );
                this->unlock();
                if( ptr == nullptr )
                        return std::shared_ptr<equity_breakdown>();
                return std::make_shared<equity_breakdown_matrix>(*ptr, std::get<0>(t));
        }
        virtual void commit(holdem_class_vector vec, equity_breakdown const& breakdown)=0;
        virtual void display()const=0;
        virtual void lock()=0;
        virtual void unlock()=0;
        virtual bool load(std::string const& name)=0;
        virtual bool save(std::string const& name)=0;
};

using holdem_class_eval_cache_factory = support::singleton_factory<holdem_class_eval_cache>;

} // ps
#endif // PS_EVAL_HOLDEM_CLASS_EVAL_CACHE_H
