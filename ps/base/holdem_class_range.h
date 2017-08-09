#ifndef PS_BASE_HOLDEM_CLASS_RANGE_H
#define PS_BASE_HOLDEM_CLASS_RANGE_H

namespace ps{

        struct holdem_class_range : std::vector<ps::holdem_class_id>{
                template<
                        class... Args,
                        class = std::enable_if_t< ! std::is_constructible<std::string, Args...>::value  >
                >
                holdem_class_range(Args&&... args)
                : std::vector<ps::holdem_id>{std::forward<Args>(args)...}
                {}
                holdem_class_range(std::string const& item){
                        this->parse(item);
                }
                friend std::ostream& operator<<(std::ostream& ostr, holdem_class_range const& self){
                        return ostr << detail::to_string(self,
                                                         [](auto id){
                                                                return holdem_class_decl::get(id).to_string();
                                                         } );
                }
                void parse(std::string const& item){
                        auto rep = expand(frontend::parse(item));
                        boost::copy( rep.to_class_vector(), std::back_inserter(*this)); 
                }
        };
} // ps
#endif //  PS_BASE_HOLDEM_CLASS_RANGE_H
