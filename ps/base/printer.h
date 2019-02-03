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
#ifndef PS_PRINTER_H
#define PS_PRINTER_H

namespace ps{

struct printer{
        printer():order_{1}{}

        void next( bool f, long a, long b, long c, long d, long e){
                *sstr_ << ( f ? "f " : "  " )
                        << traits_.rank_to_string(a)
                        << traits_.rank_to_string(b) 
                        << traits_.rank_to_string(c) 
                        << traits_.rank_to_string(d) 
                        << traits_.rank_to_string(e)
                        << "  => " << order_
                        << "\n";
                ++order_;
                ++count_;
        }
        void begin(std::string const& name){
                name_ = name;
                sstr_ = new std::stringstream;
                count_ = 0;
        }
        void end(){
                std::cout << "BEGIN " << name_ << " - " << count_ << "\n";
                std::cout << sstr_->str();
                std::cout << "END\n\n";
        }
private:
        size_t order_;
        std::string name_;
        std::stringstream* sstr_;
        size_t count_;
        card_traits traits_;
};

} // namespace ps

#endif // PS_PRINTER_H
