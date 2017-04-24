#ifndef PS_TRANSFORM_CONSOLIDATE_DUP_PRIM_H
#define PS_TRANSFORM_CONSOLIDATE_DUP_PRIM_H

namespace ps{
namespace transforms{

        struct consolidate_dup_prim : symbolic_transform{
                consolidate_dup_prim():symbolic_transform{"consolidate_dup_prim"}{}
                bool apply(symbolic_computation::handle& ptr)override{
                        if( ptr->get_kind() != symbolic_computation::Kind_Symbolic_Primitive )
                                return false;
                        auto aux_ptr{ reinterpret_cast<symbolic_primitive*>(ptr.get()) };
                        auto hash{ aux_ptr->get_hash() };
                        auto iter{ m_.find(hash) };
                        ++sigma_;
                        if( iter != m_.end() ){
                                ++hits_;
                                //PRINT_SEQ((sigma_)(hits_));
                                ptr = iter->second;
                                return true;
                        } else {
                                //PRINT_SEQ((sigma_)(hits_));
                                m_[hash] = ptr;
                                return false;
                        }
                }
                void debug(std::ostream& ostr)override{
                        ostr << "{ sigma = " << sigma_ << ", hits = " << hits_ << "}\n";
                }
        private:
                size_t sigma_ = 0;
                size_t hits_ = 0;
                std::map<std::string, symbolic_computation::handle > m_;
        };
} // transform
} // ps
#endif //PS_TRANSFORM_CONSOLIDATE_DUP_PRIM_H 
