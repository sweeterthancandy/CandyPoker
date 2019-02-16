#ifndef LIB_EVAL_DISPATCH_TABLE_H
#define LIB_EVAL_DISPATCH_TABLE_H


namespace ps{


struct optimized_transform_context{
        flush_mask_eval fme;
        holdem_board_decl w;
};

struct optimized_transform_base{
        virtual ~optimized_transform_base(){}
        virtual void apply(optimized_transform_context& otc, computation_context* ctx, instruction_list* instr_list, computation_result* result,
                   std::vector<typename instruction_list::iterator> const& target_list)=0;
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

} // end namespace ps

#endif // LIB_EVAL_DISPATCH_TABLE_H
