#ifndef PS_TRANSFORM_TRANSFORM_H
#define PS_TRANSFORM_TRANSFORM_H

#include <boost/timer/timer.hpp>

namespace ps{

        struct symbolic_transform{

                explicit symbolic_transform(std::string name):name_{name}{}

                using handle = symbolic_computation::handle;

                virtual ~symbolic_transform()=default;

                virtual void begin(){}
                virtual void end(){}
                virtual bool apply(handle&)=0;
                virtual void debug(std::ostream& ostr){}
                virtual std::string get_name(){ return name_; }
                virtual void begin_children(size_t idx){}
                virtual void end_children(){}
                virtual void next_child(){}
        private:
                std::string name_;
        };
        
        struct symbolic_computation::transform_schedular{


                enum TransformKind{
                        TransformKind_BottomUp,
                        TransformKind_TopDown,
                        TransformKind_OnlyTerminals
                };

                enum {
                        Element_Kind,
                        Element_Transform
                };
                
                struct decl_type{
                        TransformKind                       kind;
                        std::shared_ptr<symbolic_transform> transform;
                };

                using maker_t = std::function<decl_type()>;

                template<class Transform, class... Args>
                void decl(Args&&... args){
                        do_decl_<Transform>(TransformKind_BottomUp, std::forward<Args>(args)...);
                }
                template<class Transform, class... Args>
                void decl_top_down(Args&&... args){
                        do_decl_<Transform>(TransformKind_TopDown, std::forward<Args>(args)...);
                }
        private:
                template<class Transform, class... Args>
                void do_decl_(TransformKind kind, Args&&... args){
                        auto maker = [kind,args...]()mutable{
                                decl_type d = { kind,
                                                std::make_shared<Transform>(args...) };
                                return d;
                        };
                        decl_.emplace_back(std::move(maker));
                }
        public:

                bool execute( handle& root)const{
                        bool result{false};
                        bool r{false};

                        enum class opcode{
                                yeild_or_apply,
                                apply
                        };

                        for( auto const& make  : decl_){
                                decl_type dt{ make() };
                                auto transform{ dt.transform };
                                auto kind     { dt.kind };
                                boost::timer::auto_cpu_timer at( "    " + transform->get_name() +  " took %w seconds\n");

                                switch(kind){
                                case TransformKind_BottomUp:
                                        r = execute_bottom_up( transform, root );
                                        break;
                                case TransformKind_TopDown:
                                        r = execute_top_down( transform, root);
                                }
                                result = result || r;
                        }
                        return false;
                }
        private:
                bool execute_bottom_up( std::shared_ptr<symbolic_transform>& t, handle& root)const{
                        bool result{false};
                        if( root->is_non_terminal() ){
                                auto c{ reinterpret_cast<symbolic_non_terminal*>(root.get()) };
                                t->begin_children( c->size() );
                                for( auto iter{c->begin()}, end{c->end()}; iter!=end;++iter){
                                        bool s{execute_bottom_up( t, *iter)};
                                        result = result || s;
                                        t->next_child();
                                }
                                t->end_children();
                        }
                        bool r{t->apply( root )};
                        result = result || r;
                        return result;

                }
                bool execute_top_down( std::shared_ptr<symbolic_transform>& t, handle& root)const{
                        bool result{false};
                        bool r{t->apply( root )};
                        result = result || r;
                        if( root->is_non_terminal() ){
                                auto c{ reinterpret_cast<symbolic_non_terminal*>(root.get()) };
                                t->begin_children( c->size() );
                                for( auto iter{c->begin()}, end{c->end()}; iter!=end;++iter){
                                        bool s{execute_top_down( t, *iter)};
                                        result = result || s;
                                        t->next_child();
                                }
                                t->end_children();
                        }
                        return result;
                }
        private:
                std::vector<maker_t> decl_;
        };
} // ps

#endif  // PS_TRANSFORM_TRANSFORM_H
