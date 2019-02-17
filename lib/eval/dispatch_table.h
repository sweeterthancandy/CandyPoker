#ifndef LIB_EVAL_DISPATCH_TABLE_H
#define LIB_EVAL_DISPATCH_TABLE_H

#include "ps/eval/pass_eval_hand_instr_vec.h"

namespace ps{


struct optimized_transform_context{
        flush_mask_eval fme;
        holdem_board_decl w;
};

struct optimized_transform_base{
        virtual ~optimized_transform_base(){}
        virtual void apply(optimized_transform_context& otc, computation_context* ctx, instruction_list* instr_list, computation_result* result,
                   std::vector<typename instruction_list::iterator> const& target_list)noexcept=0;
};

struct dispatch_context{
        boost::optional<size_t> homo_num_players;
};
struct dispatch_table{
        virtual ~dispatch_table()=default;
        virtual bool match(dispatch_context const& dispatch_ctx)const=0;
        virtual std::shared_ptr<optimized_transform_base> make()const=0;
        virtual std::string name()const=0;
        // lowest first first
        virtual size_t precedence()const{ return 1000; }

        static std::vector<std::shared_ptr<dispatch_table> > const& world(){
                return get_world_();
        }
private:
        template<class DispatchTable>
        friend struct register_disptach_table;
        static std::vector<std::shared_ptr<dispatch_table> >& get_world_(){
                static std::vector<std::shared_ptr<dispatch_table> > V;
                return V;
        }
};

template<class DispatchTable>
struct register_disptach_table{
        register_disptach_table(){
                dispatch_table::get_world_()
                        .push_back(std::make_shared<DispatchTable>());
                boost::sort( dispatch_table::get_world_(), [](auto l, auto r){
                        return l->precedence() < r->precedence();
                });
        }
};
        
struct basic_sub_eval_factory{
        template<class T>
        struct bind{
                using sub_ptr_type = std::shared_ptr<T>;
                sub_ptr_type operator()(instruction_list::iterator iter, card_eval_instruction* instr)const{
                        return std::make_shared<T>(iter, instr);
                }
        };
};

// This is slightly faster due to memory checks
struct raw_sub_eval_factory{
        template<class T>
        struct bind{
                using sub_ptr_type = T*;
                sub_ptr_type operator()(instruction_list::iterator iter, card_eval_instruction* instr)const{
                        return new T(iter, instr);
                }
        };
};

// Ok I want to allocate blocks of sub objects in contihoues memory for cache reasons
template<size_t ObjectPoolSize>
struct block_sub_eval_factory{
        template<class T>
        struct bind{
                enum{ BlockSize = ObjectPoolSize * sizeof(T) };

                struct block{
                        block(){
                                head_ = &mem_[0];
                                end_  = &mem_[0] + BlockSize;
                                for(; (size_t)head_ % alignof(T) != 0 ; ++head_);
                        }
                        void* allocate(size_t sz){
                                if( head_ + sz >= end_ )
                                        return nullptr;
                                auto candidate = head_;
                                head_ += sz;
                                return candidate;
                        }
                private:
                        std::array<unsigned char, BlockSize> mem_;
                        unsigned char* head_;
                        unsigned char* end_;
                };
                using sub_ptr_type = T*;
                bind(){
                        blocks.push_back(std::make_unique<block>());
                }
                sub_ptr_type operator()(instruction_list::iterator iter, card_eval_instruction* instr){
                        auto raw = [&](){
                                auto teptr = blocks.back()->allocate(sizeof(T));
                                if( !! teptr )
                                        return teptr;
                                blocks.push_back(std::make_unique<block>());
                                return blocks.back()->allocate(sizeof(T));
                        }();
                        auto typed = reinterpret_cast<T*>(raw);
                        new(typed)T(iter, instr);
                        return typed;
                }
        private:
                std::vector<std::unique_ptr<block> > blocks;
        };
};

} // end namespace ps

#endif // LIB_EVAL_DISPATCH_TABLE_H
