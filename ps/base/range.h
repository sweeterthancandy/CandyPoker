#ifndef PS_BASE_RANGE_H
#define PS_BASE_RANGE_H

#include "ps/base/cards.h"

#include <map>

namespace ps{


template<class Traits>
struct basic_holdem_range{
        
        using attr_t = typename Traits::attr_t;

        struct subrange{

                using mask_t = std::map<holdem_id, attr_t>;

                struct const_iterator{

                        using impl_t = typename mask_t::const_iterator;

                        const_iterator(subrange const* _this_,
                                       impl_t iter):
                                this_(_this_),
                                iter_(iter)
                        {}
                        const_iterator& operator++(){
                                ++iter_;
                                return *this;
                        }
                        attr_t const& attr()const{
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
                        friend bool operator!=(const_iterator const& l, const_iterator const& r){
                                return l.iter_  != r.iter_;
                        }
                        struct proxy{
                                proxy(holdem_id id, attr_t const& attr):id_{id}, attr_{&attr}{}
                                holdem_hand_decl const& hand()const{ return holdem_hand_decl::get(id_); }
                                holdem_class_decl const& class_()const{
                                        return holdem_class_decl::get(hand().class_());
                                }
                                attr_t const& attr(){return *attr_; }
                        private:
                                holdem_id id_;
                                attr_t const* attr_;
                        };
                        proxy operator*(){
                                return proxy(id(), attr());
                        }
                private:
                        subrange const* this_;
                        impl_t iter_;
                };

                explicit subrange(holdem_class_id id):
                        id_{id}
                {
                }
        
                void set_hand(holdem_id id, attr_t attr = Traits::default_() ){
                        mask_[id] = attr;
                }

                
                auto begin()const{
                        auto iter = mask_.begin();
                        return const_iterator(this, std::move(iter));
                }
                auto end()const{
                        return const_iterator(this, mask_.end());
                }
                holdem_class_decl const& class_()const{ return holdem_class_decl::get(id_); }

        private:
                holdem_class_id id_;
                mask_t mask_;
        };

        using bucket_t = std::map<holdem_class_id, subrange>;


        struct const_iterator{
                using impl_t = typename bucket_t::const_iterator;

                const_iterator(basic_holdem_range const* _this_, impl_t iter):
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
                friend bool operator!=(const_iterator const& l, const_iterator const& r){
                        return l.iter_  != r.iter_;
                }
        private:
                basic_holdem_range const* this_;
                impl_t iter_;
        };


        basic_holdem_range(basic_holdem_range const&)=default;
        basic_holdem_range()=default;

        void set_hand(holdem_id id, attr_t attr = Traits::default_()){
                auto iter = buckets_.find(holdem_hand_decl::get(id).class_());
                if( iter == buckets_.end() ){
                        buckets_.emplace( holdem_hand_decl::get(id).class_(), subrange(holdem_hand_decl::get(id).class_()));
                        return this->set_hand(id, Traits::default_());
                }
                iter->second.set_hand(id, attr);
        }
        void set_hand(holdem_hand_decl const& decl, attr_t attr = Traits::default_()){
                this->set_hand( decl.id(), attr);
        }
        void set_hand(card_id a, card_id b, attr_t attr = Traits::default_()){
                this->set_hand( holdem_hand_decl::get(a, b), attr);
        }


        void set_class(holdem_class_id id, attr_t attr = Traits::default_()){
                auto const& c = holdem_class_decl::get(id);
                for( auto const& h : c.get_hand_set() ){
                        this->set_hand(h, attr);
                }
        }
        void set_class(holdem_class_decl const& decl, attr_t attr = Traits::default_()){
                this->set_class( decl.id(), attr);
        }

        const_iterator begin()const{
                return const_iterator(this, buckets_.begin());
        }
        const_iterator end()const{
                return const_iterator(this, buckets_.end());
        }

        basic_holdem_range& append(basic_holdem_range const& that){
                for( auto byclass : that ){
                        for( auto byhand : byclass ){
                                this->set_hand(byhand.hand().id());
                        }
                }
                return *this;
        }

        friend std::ostream& operator<<(std::ostream& ostr, basic_holdem_range const& self){
                ostr << "{";
                bool first = true;
                for( auto byclass : self ){
                        for( auto byhand : byclass ){
                                if( first) first = false;
                                else ostr << ", ";
                                ostr << byhand.hand();
                        }
                }
                ostr << "}";
                return ostr;
        }
private:
        bucket_t buckets_;
};

namespace detail{
        struct boolean_range_traits{
                using attr_t = bool;
                static attr_t default_(){ return true; }
        };
        struct weighted_range_traits{
                using attr_t = double;
                static attr_t default_(){ return 1.0; }
        };
} // detail

using holdem_range          = basic_holdem_range<detail::boolean_range_traits>;
using holdem_weighted_range = basic_holdem_range<detail::weighted_range_traits>;




} // ps

#endif // PS_BASE_RANGE_H
