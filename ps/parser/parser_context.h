#ifndef PS_PARSER_CONTEXT_H
#define PS_PARSER_CONTEXT_H

#include <iostream>

namespace ps{
namespace parser{


struct parser_context{
};

struct stream_parser_context : parser_context{
        explicit stream_parser_context(std::ostream& ostr):ostr_{&ostr}{}
private:
        std::ostream* ostr_;
};

} // parser
} // ps


#endif // PS_PARSER_CONTEXT_H
