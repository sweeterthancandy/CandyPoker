#ifndef PS_BASE_RANGE_H
#define PS_BASE_RANGE_H

#include "ps/base/cards.h"

#include <unordered_map>

namespace ps{


struct range{
        
        static constexpr const double _1 = 1.0;


        struct subrange{

                using mask_t = std::map<holdem_id, double>;

                struct const_iterator{

                        using impl_t = mask_t::const_iterator;

                        const_iterator(subrange const* _this_,
                                       impl_t iter):
                                this_(_this_),
                                iter_(iter)
                        {}
                        const_iterator& operator++(){
                                ++iter_;
                                #if 0
                                for( ; iter_ != this_->mask_.end() &&
                                       iter_ == 0.0 ; ++iter_);
                                       #endif
                                return *this;
                        }
                        double weight()const{
                                return iter_->second;
                        }
                        holdem_id id()const{
                                return iter_->first;
                        }
                        holdem_hand_decl decl()const{
                                return holdem_hand_decl::get(this->id());
                        }
                        friend bool operator==(const_iterator const& l, const_iterator const& r){
                                return l.iter_  == r.iter_;
                        }
                private:
                        subrange const* this_;
                        mask_t::const_iterator iter_;
                };

                explicit subrange(holdem_class_id id):
                        id_{id}
                {
                }
        
                void set_hand(holdem_id id, double weight = _1){
                        mask_[id] = weight;
                }

                
                auto begin()const{
                        auto iter = mask_.begin();
                        #if 0
                        for(; iter != mask_.end() && *iter == 0.0; ++iter);
                        #endif
                        return const_iterator(this, std::move(iter));
                }
                auto end()const{
                        return const_iterator(this, mask_.end());
                }

        private:
                holdem_class_id id_;
                mask_t mask_;
        };

        using bucket_t = std::map<holdem_class_id, subrange>;


        struct const_iterator{
                using impl_t = bucket_t::const_iterator;

                const_iterator(range const* _this_, impl_t iter):
                        this_(_this_), iter_(iter)
                {}

                auto class_()const{ return iter_->first; }

                auto begin()const{ return iter_->second.begin(); }
                auto end()const  { return iter_->second.end();   }
                        
                const_iterator& operator++(){
                        ++iter_;
                        return *this;
                }
                subrange const& operator*()const{
                        return iter_->second;
                }


                friend bool operator==(const_iterator const& l, const_iterator const& r){
                        return l.iter_  == r.iter_;
                }
        private:
                range const* this_;
                impl_t iter_;
        };


        range(range const&)=default;
        range()=default;

        void set_hand(holdem_id id, double weight = _1){
                auto iter = buckets_.find(holdem_hand_decl::get(id).class_());
                if( iter == buckets_.end() ){
                        buckets_.emplace( holdem_hand_decl::get(id).class_(), holdem_hand_decl::get(id).class_());
                        return this->set_hand(id, _1);
                }
                iter->second.set_hand(id, weight);
        }
        void set_hand(holdem_hand_decl const& decl, double weight = _1){
                this->set_hand( decl.id(), weight);
        }
        void set_hand(card_id a, card_id b, double weight = _1){
                this->set_hand( holdem_hand_decl::get(a, b), weight);
        }


        void set_class(holdem_class_id id, double weight = _1){
                auto const& c = holdem_class_decl::get(id);
                for( auto const& h : c.get_hand_set() ){
                        this->set_hand(h, weight);
                }
        }
        void set_class(holdem_class_decl const& decl, double weight = _1){
                this->set_class( decl.id(), weight);
        }

        const_iterator begin()const{
                return const_iterator(this, buckets_.begin());
        }
        const_iterator end()const{
                return const_iterator(this, buckets_.end());
        }

private:
        bucket_t buckets_;
};

} // ps

#endif // PS_BASE_RANGE_H
