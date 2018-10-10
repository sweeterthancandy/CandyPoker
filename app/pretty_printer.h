#ifndef PS_APP_PRETTY_PRINTER_H
#define PS_APP_PRETTY_PRINTER_H

#include <unordered_map>
#include <vector>
#include <string>
#include <ostream>
#include <iostream>
#include <vector>
#include <boost/accumulators/accumulators.hpp>
#include <boost/accumulators/statistics/stats.hpp>
#include <boost/accumulators/statistics/mean.hpp>
#include <boost/accumulators/statistics/min.hpp>
#include <boost/accumulators/statistics/max.hpp>
#include <boost/accumulators/statistics/sum.hpp>
#include <boost/accumulators/statistics/variance.hpp>
#include <boost/variant.hpp>




namespace ps{
namespace Pretty{

        struct LineBreakType{};
        static LineBreakType LineBreak{};

        using LineItem = boost::variant<
                std::vector<std::string>,
                LineBreakType
        >;
                


        enum RenderAdjustment{
                RenderAdjustment_Left,
                RenderAdjustment_Center,
                RenderAdjustment_Right,
        };
        struct RenderOptions{
                RenderOptions& SetAdjustment(size_t offset, RenderAdjustment adj){
                        adjustment_[offset] = adj;
                        return *this;
                }
                RenderOptions& SetWidth(size_t offset, size_t w){
                        widths_[offset] = w;
                        return *this;
                }
                template<class... Args>
                RenderOptions& Header(Args&&... args){
                        header_ = std::vector<std::string>{args...};
                        return *this;
                }
                auto const& GetHeader()const{ return header_; }

                RenderAdjustment GetAdjustment(size_t offset)const{
                        auto iter = adjustment_.find(offset);
                        if( iter == adjustment_.end())
                                return default_adjustment_;
                        return iter->second;
                }
                size_t GetWidth(size_t offset)const{
                        auto iter = widths_.find(offset);
                        return ( iter == widths_.end() ? -1 : iter->second );
                }
        private:
                RenderAdjustment default_adjustment_{RenderAdjustment_Center};
                std::unordered_map<size_t, RenderAdjustment> adjustment_;
                std::unordered_map<size_t, size_t> widths_;
                std::vector<std::string> header_;
        };
        

        namespace Detail{
                static auto repeat = [](char c, int n){
                        if(n == 0 )
                                return std::string{};
                        return std::string(n, c);
                };

                struct WidthConsumer : boost::static_visitor<void>{
                        void operator()(std::vector<std::string> const& line){
                                for( ; widths.size() < line.size(); )
                                        widths.push_back(0);
                                for( size_t i=0;i!=line.size();++i){
                                        widths[i] = std::max( widths[i], line[i].size() );
                                }
                        }
                        void operator()(LineBreakType){
                        }

                        void ApplyOptions(RenderOptions const& opts){
                                for(size_t i=0;i!=widths.size();++i){
                                        auto w = opts.GetWidth(i);
                                        if( w != static_cast<size_t>(-1) ){
                                                widths[i] = w;
                                        }
                                }
                        }
                        std::vector<size_t> widths;
                };

                struct Printer : boost::static_visitor<void>{
                        void put_(std::string const& s, size_t w, RenderAdjustment adj){

                                if( w < s.size() ){
                                        *ostr << s.substr(0,w);
                                        return;
                                } 

                                size_t left{0};
                                size_t right{0};
                                size_t d = w - s.size();
                                switch(adj){
                                case RenderAdjustment_Left:
                                        right = d;
                                        break;
                                case RenderAdjustment_Right:
                                        left = d;
                                        break;
                                case RenderAdjustment_Center:
                                        left = d / 2;
                                        right = d - left;
                                        break;
                                }
                                *ostr << repeat(' ', left) << s << repeat(' ', right);
                        }
                        void operator()(std::vector<std::string> const& line){
                                for( size_t i=0;i!=line.size();++i){
                                        *ostr << "|";
                                        put_(line[i], widths->widths[i], opts->GetAdjustment(i) );
                                }
                                *ostr << "|\n";
                        }
                        void operator()(LineBreakType){
                                std::string spacer;
                                spacer += '+';
                                for(size_t i=0;i!=widths->widths.size();++i){
                                        spacer += repeat('-', widths->widths[i]);
                                        spacer += '+';
                                }
                                *ostr << spacer << "\n";
                        }
                        Printer(std::ostream* ostr_, WidthConsumer* widths_, RenderOptions const* opts_)
                                :ostr(ostr_),
                                widths(widths_),
                                opts(opts_)
                        {}
                        std::ostream* ostr;
                        WidthConsumer* widths;
                        RenderOptions const* opts;
                };
        } // Detail

        void RenderTablePretty(std::ostream& ostr,
                               std::vector<LineItem> const& lines,
                               RenderOptions const& opts = RenderOptions{})
        {
                Detail::WidthConsumer widths;
                boost::for_each( lines, boost::apply_visitor(widths) );
                widths.ApplyOptions( opts );

                Detail::Printer p = { &ostr, &widths, &opts };
                
                boost::for_each( lines, boost::apply_visitor(p) );

                ostr.flush();
        }
        void RenderTablePretty(std::ostream& ostr,
                               std::vector<std::vector<std::string> > const& lines,
                               RenderOptions const& opts = RenderOptions{})
        {
                std::vector<LineItem> tmp;
                tmp.push_back(LineBreak);
                if( opts.GetHeader().size() ){
                        tmp.push_back( opts.GetHeader() );
                        tmp.push_back(LineBreak);
                }
                for( auto const& line : lines)
                        tmp.push_back(line);

                tmp.push_back(LineBreak);

                RenderTablePretty(ostr, tmp, opts);
        }

        template<class T>
        struct TwoDimArray{
                TwoDimArray(size_t width, size_t height, T const& DefaultValue)
                        :width_(width),
                        height_(height),
                        points_(width_*height_, DefaultValue)
                {}
                auto& Access(size_t x, size_t y){
                        //std::cerr << "Access(" << x << ", " << y << ")\n";
                        return points_.at( map_(x,y) );
                }
                auto const& Access(size_t x, size_t y)const{
                        //std::cerr << "Access(" << x << ", " << y << ")\n";
                        return points_.at( map_(x,y) );
                }
                auto Width()const{ return width_; }
                auto Height()const{ return height_; }
                
        private:
                auto map_(size_t x, size_t y)const{
                        if( ! ( x < this->Width( ) ) )
                                throw std::domain_error("x out of bound");
                        if( ! ( y < this->Height( ) ) )
                                throw std::domain_error("y out of bound");
                        return x + y * width_;
                }

                size_t width_;
                size_t height_;
                std::vector<T> points_;
        };

        /*
                A cursor is 
         */
        struct GraphPen{
                GraphPen(TwoDimArray<char>* ptr):
                        ptr_(ptr)
                {}
                void MoveTo(size_t x, size_t y){
                        //std::cerr << "MoveTo(" << x << ", " << y << ")\n";
                        if( down_ ){
                                if( x < last_x_)
                                        throw std::domain_error("x not increasing");
                                for(;;){
                                        if( x == last_x_ && y == last_y_ )
                                                break;
                                        signed xd = (signed)x - last_x_;
                                        signed yd = (signed)y - last_y_;
                                        signed abs_yd = yd;
                                        #if 0
                                        std::cout << "xd = " << xd << "\n";
                                        std::cout << "yd = " << yd << "\n";
                                        std::cout << "abs_yd = " << abs_yd << "\n";
                                        #endif
                                        if( abs_yd < 0 )
                                                abs_yd *= -1;
                                        if( xd == yd ){
                                                ++last_x_;
                                                ++last_y_;
                                                ptr_->Access(last_x_, last_y_) = '/';
                                        }else  if( xd == -yd ){
                                                ++last_x_;
                                                --last_y_;
                                                ptr_->Access(last_x_, last_y_) = '\\';
                                        } else if( xd < abs_yd ){
                                                if( 0 < yd ){
                                                        ++last_y_;
                                                } else {
                                                        --last_y_;
                                                }
                                                ptr_->Access(last_x_, last_y_) = '|';
                                        } else{
                                                ++last_x_;
                                                ptr_->Access(last_x_, last_y_) = '-';
                                        }
                                }
                        } else {
                                down_ = true;
                                ptr_->Access(x,y) = '-';
                        }
                        last_x_ = x;
                        last_y_ = y;
                }
        private:
                bool down_{false};
                size_t last_x_{0};
                size_t last_y_{0};
                TwoDimArray<char>* ptr_;
        };
        struct AnonAxis{};
        struct RealAxis{
                RealAxis(double min_, double max_)
                        :min(min_),
                        max(max_)
                {}
                double min;
                double max;
        };

        struct AlgebraicAxis{
                template<class... Args>
                explicit AlgebraicAxis(Args&&... args)
                        : points{args...}
                {}
                std::vector<std::string> points;
        };

        using AxisVariant = boost::variant<
                AnonAxis,
                RealAxis,
                AlgebraicAxis
        >;

        struct label_maker : boost::static_visitor<void>{
                explicit label_maker(size_t n_):n{n_}{}
                void operator()(RealAxis const& axis){
                        std::cout << "RealAxis\n";

                        auto d = axis.max - axis.min;
                        if( n > 2 )
                                d /= static_cast<decltype(d)>(n-1);

                        auto emit = [this](auto _){
                                labels.push_back(boost::lexical_cast<std::string>(_));
                        };

                        auto x = axis.min;
                        for(size_t idx = 0; idx + 1 < n; ++idx, x += d){
                                emit( x );
                        }
                        emit( axis.max );

                }
                void operator()(AlgebraicAxis const& axis){
                        std::cout << "AlgebraicAxis\n";
                        auto first = axis.points.begin();
                        auto last = std::next(first, std::min<size_t>(axis.points.size(), n) );
                        std::copy( first, last, std::back_inserter(labels) );
                }
                void operator()(AnonAxis const& axis){
                        std::cout << "AnonAxis\n";
                }
                auto Get(size_t idx, std::string default_value = "")const{
                        if( labels.size() < idx )
                                return labels[idx];
                        return default_value;
                }
                size_t n{2};
                std::vector<std::string> labels;
        };
        struct PrettyGraph{
                explicit PrettyGraph(TwoDimArray<char>&& bitmap)
                        : bitmap_{bitmap}
                {}
                PrettyGraph& SetXAxis(AxisVariant const& axis){
                        x_axis_ = axis;
                        return *this;
                }
                PrettyGraph& SetYAxis(AxisVariant const& axis){
                        y_axis_ = axis;
                        return *this;
                }
                PrettyGraph& SetYRealAxis(double min, double max){
                        y_axis_ = RealAxis{min, max};
                        return *this;
                }
                PrettyGraph& SetTitle(std::string const& title){
                        title_ = title;
                        return *this;
                }
                
                AxisVariant const& GetXAxis()const{
                        return x_axis_;
                }
                AxisVariant const& GetYAxis()const{
                        return y_axis_;
                }
                TwoDimArray<char> const& GetBitmap()const{
                        return bitmap_;
                }
                std::string const& GetTitle()const{
                        return title_;
                }

                
                inline
                void Display(std::ostream& ostr = std::cout)const;
        private:
                std::string title_;
                AxisVariant x_axis_{AnonAxis{}};
                AxisVariant y_axis_{AnonAxis{}};
                TwoDimArray<char> bitmap_;
        };

        void PrettyGraph::Display(std::ostream& ostr)const{

                label_maker x_labels(3);
                label_maker y_labels(3);

                boost::apply_visitor( x_labels,x_axis_);
                boost::apply_visitor( y_labels,y_axis_);

                std::map<size_t, std::string> y_label_map;
                for(size_t i=0;i!=y_labels.labels.size();++i){
                        auto y = static_cast<double>(bitmap_.Height() ) * i / (y_labels.labels.size() -1 );
                        if( i +1 == y_labels.labels.size() )
                                y = bitmap_.Height() -1;
                        y_label_map[y]  = y_labels.labels[i];
                }

                std::string header;
                header += '+';
                header += std::string( bitmap_.Width(), '-');
                header += '+';

                ostr << header << "\n";
                for(size_t y=bitmap_.Height();y!=0;){
                        --y;
                        ostr << "|";
                        for(size_t x=0;x!=bitmap_.Width();++x){
                                ostr << bitmap_.Access(x,y);
                        }
                        ostr << "|";
                        if( y_label_map.count( y ) )
                                ostr << y_label_map[y];
                        ostr << "\n";
                }
                ostr << header << "\n";

                // print bottom labels

                if( x_labels.labels.size() ){
                        ostr << " ";
                        size_t cursor = 0;
                        for(size_t i=0;i + 1 < x_labels.labels.size();++i){
                                // first we move the cursor forward
                                auto target = static_cast<double>(bitmap_.Width()) * i / ( x_labels.labels.size() - 1 );

                                if( cursor < target ){
                                        auto d = target - cursor;
                                        ostr << std::string(d, ' ');
                                        cursor +=d;
                                }


                                ostr << x_labels.labels[i];
                                cursor += x_labels.labels[i].size();

                        }
                        // the last label wants to be oritated inside

                        auto const space_left = bitmap_.Width() - cursor;
                        if( x_labels.labels.back().size() < space_left ){
                                auto d = space_left - x_labels.labels.back().size();
                                ostr << std::string(d, ' ');
                        }
                        ostr << x_labels.labels.back();
                        ostr << "\n";
                }

        }

        struct Point{
                Point(double x_, double y_)
                        :x(x_),
                        y(y_)
                {}
                friend std::ostream& operator<<(std::ostream& ostr, Point const& self){
                        ostr << "x = " << self.x;
                        ostr << ", y = " << self.y;
                        return ostr;
                }
                double x;
                double y;
        };


        
        


        void GraphDisplay(std::ostream& ostr, PrettyGraph const& graph);


        namespace ac = boost::accumulators;


        struct TimeSeriesRenderOptions{
                size_t Width{120};
                size_t Height{30};
                AxisVariant XAxis{AnonAxis{}};
                AxisVariant YAxis{AnonAxis{}};
        };

        
        PrettyGraph RenderTimeSeries(TimeSeriesRenderOptions const& opts, std::vector<Point> const& vec){
                using acc_type = ac::accumulator_set<double, 
                        ac::stats<
                                ac::tag::min,
                                ac::tag::max
                        >
                >;
                
                acc_type xacc, yacc;
                for(auto const& _ : vec){
                        xacc(_.x);
                        yacc(_.y);
                }

                auto x_min = ac::min(xacc);
                auto x_max = ac::max(xacc);
                auto y_min = ac::min(yacc);
                auto y_max = ac::max(yacc);

                auto xm = [&](auto _){
                        return ( _ - x_min ) / ( x_max - x_min ) * ( opts.Width - 1 );
                };
                auto ym = [&](auto _){
                        return ( _ - y_min ) / ( y_max - y_min ) * ( opts.Height - 1);
                };

                TwoDimArray<char> bitmap( opts.Width , opts.Height, ' ' );
                GraphPen pen(&bitmap);
                
                for(auto const& _ : vec){
                        auto u = (size_t)xm(_.x);
                        auto v = (size_t)ym(_.y);

                        pen.MoveTo(u, v);
                }

                return PrettyGraph{std::move(bitmap)};
        }
        void DisplayTimeSeries(TimeSeriesRenderOptions const& opts, std::vector<Point> const& vec){
                auto graph = RenderTimeSeries(opts, vec);
                graph
                        .SetXAxis( opts.XAxis )
                        .SetYAxis( opts.YAxis )
                        .SetTitle("Title")
                ;
                graph.Display(std::cout);
        }

} // Pretty
} // end namespace ps




namespace ps{


        template<class MatrixType>
        void pretty_print_equity_breakdown_mat(std::ostream& ostr, MatrixType const& breakdown , std::vector<std::string> const& players){

                using namespace Pretty;
                
                std::vector<std::string> title;


                title.emplace_back("range");
                title.emplace_back("equity");
                title.emplace_back("wins");
                //title.emplace_back("draws");
                #if 1
                for(size_t i=0; i != players.size() -1;++i){
                        title.emplace_back("draw_"+ boost::lexical_cast<std::string>(i+1));
                }
                
                #endif
                title.emplace_back("draw equity");
                //title.emplace_back("lose");
                title.emplace_back("sigma");
                
                std::vector< LineItem > lines;
                lines.emplace_back(title);
                lines.emplace_back(LineBreak);

                std::map<long, unsigned long long> sigma_device;
                for( size_t i=0;i!=players.size();++i){
                        for(size_t j=0; j != players.size(); ++j ){
                                sigma_device[j] += breakdown(j,i);
                        }
                }
                unsigned long long sigma = 0;
                for( size_t i=0;i!=players.size();++i){
                        sigma += sigma_device[i] / ( i +1 );
                }


                for( size_t i=0;i!=players.size();++i){

                        double equity = 0.0;
                        for(size_t j=0; j != players.size(); ++j ){
                                equity += breakdown(j,i) / ( j +1 );
                        }
                        equity /= sigma;

                        std::vector<std::string> line;

                        line.emplace_back( boost::lexical_cast<std::string>(players[i]) );
                        line.emplace_back( str(boost::format("%.4f%%") % (equity * 100)));
                        /*
                                draw_equity = \sum_i=1..n win_{i}/i
                        */
                        unsigned long long draw;
                        unsigned long long p_sigma;
                        for(size_t j=0; j != players.size(); ++j ){
                                line.emplace_back( boost::lexical_cast<std::string>(breakdown(j,i)) );
                                if( j != 0 ) draw += breakdown(j,i);
                                p_sigma += breakdown(j,i);
                        }

                        auto draw_sigma = (equity - static_cast<double>(breakdown(0,i))/sigma)*sigma;
                        line.emplace_back( str(boost::format("%.2f%%") % ( draw_sigma )));
                        #if 0
                        line.emplace_back( boost::lexical_cast<std::string>(pv.lose()) );
                        #endif
                        line.emplace_back( boost::lexical_cast<std::string>(sigma) );

                        lines.push_back(line);
                }
                RenderTablePretty(std::cout, lines);
                
        }
} // end namespace ps
#endif // PS_APP_PRETTY_PRINTER_H
