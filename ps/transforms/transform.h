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
                        auto maker = [args...]()mutable{
                                decl_type d = { TransformKind_BottomUp,
                                                std::make_shared<Transform>(args...) };
                                return d;
                        };
                        decl_.emplace_back(std::move(maker));
                }

                bool execute( handle& root)const{
                        bool result{false};

                        enum class opcode{
                                yeild_or_apply,
                                apply
                        };

                        for( auto const& make  : decl_){
                                decl_type dt{ make() };
                                auto transform{ dt.transform };
                                boost::timer::auto_cpu_timer at( "    " + transform->get_name() +  " took %w seconds\n");
                                std::vector<std::pair<opcode, handle*>> stack;
                                stack.emplace_back(opcode::yeild_or_apply, &root);
                                transform->begin();
                                for(;stack.size();){
                                        auto p{stack.back()};
                                        stack.pop_back();
                                        
                                        switch(p.first){
                                        case opcode::yeild_or_apply:
                                                if( (*p.second)->is_non_terminal()){
                                                        stack.emplace_back( opcode::apply, p.second );
                                                        auto c{ reinterpret_cast<symbolic_non_terminal*>(p.second->get()) };
                                                        for( auto iter{c->begin()}, end{c->end()}; iter!=end;++iter){
                                                                stack.emplace_back( opcode::yeild_or_apply, &*iter );
                                                        }
                                                        break;
                                                }
                                                // fallthought
                                        case opcode::apply:{
                                                bool ret{ transform->apply( *p.second )};
                                                result = result || ret;
                                        }
                                                break;
                                        }
                                }
                                transform->debug(std::cerr);
                                transform->end();
                                std::cout.flush();
                        }
                        return result;
                }

        private:
                std::vector<maker_t> decl_;
        };
} // ps

#endif  // PS_TRANSFORM_TRANSFORM_H
