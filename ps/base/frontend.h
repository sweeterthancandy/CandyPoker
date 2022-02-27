/*

CandyPoker
https://github.com/sweeterthancandy/CandyPoker

MIT License

Copyright (c) 2019 Gerry Candy

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.

*/
#ifndef PS_HOLDEM_FRONTEND_H
#define PS_HOLDEM_FRONTEND_H

#include <string>
#include <memory>
#include <ostream>
#include <vector>

#include "ps/base/cards_fwd.h"


namespace ps {

    namespace frontend {




        class class_range_impl;
        class class_range
        {
        public:
            explicit class_range(std::shared_ptr<class_range_impl> const& impl);
            std::string to_string()const;
            holdem_class_type category()const;
            std::vector<holdem_id> const& to_holdem_vector()const;
            bool is_subset()const;
            holdem_class_id get_class_id()const;
        private:
            std::shared_ptr<class_range_impl> impl_;
        };

        class range_impl;
        // this represents a single holdem range
        class range
        {
        public:
            explicit range(std::string const& s);
            explicit range(std::shared_ptr<range_impl> const& impl);
            std::string to_string()const;
            //friend std::ostream& operator<<(std::ostream& ostr, range const& self);
            range expand()const;
            range_impl const& get_ref()const;

            std::vector<ps::holdem_class_id> to_class_vector()const;

            std::vector<class_range> to_class_range_vector()const;
        private:
            std::shared_ptr<range_impl> impl_;
        };




        /*
        This represents the different levels of range vs range

            Leve0 : top level range
            Level1 : full holdem class level range or sub class level
            Level2 : card level range
        */


        


        //holdem_range convert_to_range(frontend::range_impl const& rng);
        //holdem_range parse_holdem_range(std::string const& s);
} // end namespace frontend
} // ps

#endif // PS_HOLDEM_FRONTEND_H
