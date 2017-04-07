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
