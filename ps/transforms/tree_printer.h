#ifndef PS_TRANSFORMS_TREE_PRINTER_H
#define PS_TRANSFORMS_TREE_PRINTER_H



namespace ps{
        namespace transforms{

                struct pretty_printer{

                        struct color_formatter{
                                static std::string color_red(const std::string& s){
                                        return "\033[0;36m" + s + "\033[0;m";
                                }
                                static std::string color_purple(const std::string& s){
                                        return "\033[0;35m" + s + "\033[0;m";
                                }
                                static std::string color_blue(const std::string& s){
                                        return "\033[0;34m" + s + "\033[0;m";
                                }
                        };

                        struct runtime_policy{
                                runtime_policy(bool use_color , bool print_files , size_t max_depth  )
                                        : use_color_(use_color)
                                        , print_files_(print_files)
                                        , max_depth_(max_depth)
                                {
                                }
                                std::string format_file(const std::string& s)const{
                                        if( use_color_ )
                                                return color_formatter::color_red(s);
                                        return s;
                                }
                                std::string format_dir(const std::string& s)const{
                                        if( use_color_ )
                                                return color_formatter::color_purple(s);
                                        return s;
                                }
                                std::string format_number(const std::string& s)const{
                                        if( use_color_)
                                                return color_formatter::color_blue(s);
                                        return s;
                                }
                                std::string render_number(std::uintmax_t val)const{
                                        std::string aux{ boost::lexical_cast<std::string>(val) };
                                        std::string ret;

                                        /*

                                           1 1       -> 1          0
                                           2 12      -> 12         0
                                           3 123     -> 123        0
                                           4 1234    -> 1,234      1
                                           5 12345   -> 12,234     1
                                           6 123456  -> 123,456    1
                                           7 1234567 -> 1,234,456  2

                                        */
                                        for(size_t i = 0; i!= aux.size(); ++i){
                                                if( i != 0 && ( aux.size() - i ) % 3 == 0 )
                                                        ret += ',';
                                                ret += aux[i];
                                        }
                                        return std::move(ret);
                                }
                                bool print_files()const{return print_files_;}
                                size_t max_depth()const{return max_depth_;}
                        private:
                                const bool use_color_;
                                const bool print_files_;
                                const size_t max_depth_;
                        };

                        template<typename... Args>
                        pretty_printer(Args&&... args):
                                policy_(std::forward<Args>(args)...)
                        { }
                        ~pretty_printer(){
                        }
                        void put_line_(const std::string& line){
                                if( stack_.size() ){
                                        std::for_each( stack_.begin(), std::prev(stack_.end()),
                                                [this](format_data& fd){
                                                        fd.print(std::cout);
                                        });
                                        stack_.back().print_alt(std::cout);
                                }
                                std::cout << line << "\n";
                        }
                        void operator()( file_node& fn)final{
                                if( stack_.size() > policy_.max_depth() )
                                        return;
                                if( not policy_.print_files())
                                        return;

                                std::stringstream sstr;
                                        
                                auto lines = fn.data_as<accumulator>().lines();
                                sstr << " " << 
                                        policy_.format_number(
                                                policy_.render_number(
                                                        fn.data_as<accumulator>().lines()))
                                        << " ";
                                sstr << 
                                        policy_.format_file(fn.path().filename().string());

                                put_line_(sstr.str());
                                   
                        }
                        void operator()(directory_node& dn)final{
                                if( not ( stack_.size() > policy_.max_depth() ) ){
                                        auto lines = dn.data_as<accumulator>().lines();
                                        std::stringstream sstr, sstr2;
                                        sstr << " " << policy_.render_number(dn.data_as<accumulator>().lines()) << " ";
                                        size_t size = sstr.str().size();

                                        sstr2 << 
                                                policy_.format_number(sstr.str()) << 
                                                policy_.format_dir(dn.path().filename().string());

                                        put_line_(sstr2.str());
                                        stack_.emplace_back(size);
                                } else{
                                        stack_.emplace_back(0);
                                }
                        }
                        virtual void begin_children_size(size_t n)final{
                                stack_.back().finalize(n);
                        }
                        void end_children()final{
                                stack_.pop_back();
                        }
                        void next_child()final{
                                stack_.back().next();
                        }
                private:

                        struct angle_c_traits{
                                using hor_c = boost::mpl::char_<'_'>;
                                using vert_c = boost::mpl::char_<'|'>;
                                using br_c = boost::mpl::char_<'\\'>;
                        };
                        struct straight_c_traits{
                                using hor_c = boost::mpl::char_<'_'>;
                                using vert_c = boost::mpl::char_<'|'>;
                                using br_c = boost::mpl::char_<'|'>;
                        };
                        
                        using c_traits = angle_c_traits;

                        struct format_data{
                                std::string make_(char c, char a){
                                        std::string tmp;
                                        tmp += std::string((indent_-1)/2,' ');
                                        tmp += c;
                                        tmp += std::string(indent_-tmp.size(),a);
                                        return tmp;
                                }
                                format_data(size_t indent):indent_(indent){
                                        auto sz = indent_;
                                }
                                void finalize(size_t num_children){
                                        num_children_ = num_children;

                                }
                                void print_alt(std::ostream& ostr){
                                        if( indent_ )
                                                ostr << make_(c_traits::br_c(), c_traits::hor_c());
                                }
                                void print(std::ostream& ostr){
                                        if( indent_ ){
                                                if( idx_ != num_children_ -1 ){
                                                        ostr << make_(c_traits::vert_c(), ' ');
                                                } else{
                                                        ostr << std::string(indent_,' ');
                                                }
                                        }
                                }
                                void next(){
                                        ++idx_;
                                        assert( idx_ <= num_children_ );
                                }
                        private:
                                size_t idx_{0}; // child index
                                size_t num_children_; // number of children
                                const size_t indent_; // lengh of indent
                        };
                        std::list<format_data> stack_;
                        runtime_policy policy_;
                };
