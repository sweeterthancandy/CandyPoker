#include "ps/base/holdem_class_range.h"

#include "ps/base/frontend.h"
#include "ps/detail/print.h"

namespace ps{
        holdem_class_range::holdem_class_range(std::string const& item){
                this->parse(item);
        }

        std::ostream& operator<<(std::ostream& ostr, holdem_class_range const& self){
                return ostr << detail::to_string(self,
                                                 [](auto id){
                                                        return holdem_class_decl::get(id).to_string();
                                                 } );
        }
        void holdem_class_range::parse(std::string const& item){
                auto rep = expand(frontend::parse(item));
                boost::copy( rep.to_class_vector(), std::back_inserter(*this)); 
        }
} // ps
