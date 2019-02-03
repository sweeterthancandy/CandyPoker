#ifndef PS_SIM__EXTRA_H
#define PS_SIM__EXTRA_H

#include <future>


namespace ps{
namespace sim{
        inline
        void DisplayStrategy(StateType const& S, size_t dp = 4){
                for(size_t idx=0;idx!=S.size();++idx){
                        pretty_print_strat(S[idx][0], dp);
                }
        }
        struct evaluate_saver{
                evaluate_saver(std::ostream& ostr):ostr_{&ostr}{
                        std::stringstream sstr;
                        sstr <<        quote("section")
                             << "," << quote("cv")
                             << "," << quote("p")
                             << "," << quote("c")
                             << "," << quote("value");
                        (*ostr_) << sstr.str() << "\n";
                }
                void decl_section(std::string const& section){
                        section_ = section;

                }
                void operator()(holdem_class_vector const& cv, double p, double c, Eigen::VectorXd const& value){
                        std::stringstream sstr;
                        sstr <<        quote(section_)
                             << "," << quote(cv)
                             << "," << quote(p)
                             << "," << quote(c)
                             << "," << quote(vector_to_string(value));
                        (*ostr_) << sstr.str() << "\n";
                }

        private:
                template<class T>
                static std::string quote(T const& _)noexcept{
                        return "\"" + boost::lexical_cast<std::string>(_)  + "\"";
                }
                std::ostream* ostr_;
                std::string section_;
        };
        
        struct strategy_saver{
                strategy_saver(std::ostream& ostr):ostr_{&ostr}{
                        std::stringstream sstr;
                        sstr <<        quote("path")
                             << "," << quote("class")
                             << "," << quote("P");
                        (*ostr_) << sstr.str() << "\n";
                }
                void operator()(std::vector<std::vector<Eigen::VectorXd> > const& S){
                        for(size_t d=0;d!=S.size();++d){
                                for(size_t idx=0;idx!=169;++idx){
                                        std::stringstream sstr;
                                        sstr <<        quote(d)
                                             << "," << quote(holdem_class_decl::get(idx))
                                             << "," << quote(S[d][0][idx]);
                                        (*ostr_) << sstr.str() << "\n";
                                }
                        }
                }

        private:
                template<class T>
                static std::string quote(T const& _)noexcept{
                        return "\"" + boost::lexical_cast<std::string>(_)  + "\"";
                }
                std::ostream* ostr_;
                std::string section_;
        };
        struct EvDetailDevice : boost::noncopyable{
                enum {Debug = false };
                EvDetailDevice(size_t pidx)
                        :pidx_(pidx)
                {
                        A.resize(169);
                        A.fill(0.0);
                        dbg0.resize(169);
                        dbg0.fill(0.0);
                        dbg1.resize(169);
                        dbg1.fill(0.0);
                        dbg2.resize(169);
                        dbg2.fill(0.0);
                }
                void operator()(holdem_class_vector const& cv, double p, double c, Eigen::VectorXd const& value){
                        A[cv[pidx_]] += p * c * value[pidx_];
                        if( Debug ){
                                dbg0[cv[pidx_]] += p;
                                dbg1[cv[pidx_]] += c;
                                dbg2[cv[pidx_]] += p * c;
                        }
                }
                void decl_section(std::string const& section){}
                void Display(std::string const& title)const{

                        if( Debug ){
                                std::cout << "\n\n-------------------- " << title << " -----------------------\n\n";
                                pretty_print_strat(A, 3);
                                std::cout << "A.sum() => " << A.sum() << "\n"; // __CandyPrint__(cxx-print-scalar,A.sum())
                                pretty_print_strat(dbg0, 3);
                                std::cout << "dbg0.sum() => " << dbg0.sum() << "\n"; // __CandyPrint__(cxx-print-scalar,dbg0.sum())
                                pretty_print_strat(dbg1, 3);
                                std::cout << "dbg1.sum() => " << dbg1.sum() << "\n"; // __CandyPrint__(cxx-print-scalar,dbg0.sum())
                                pretty_print_strat(dbg2, 3);
                                std::cout << "dbg2.sum() => " << dbg2.sum() << "\n"; // __CandyPrint__(cxx-print-scalar,dbg0.sum())
                        }
                }
                size_t pidx_;
                Eigen::VectorXd A;
                Eigen::VectorXd dbg0;
                Eigen::VectorXd dbg1;
                Eigen::VectorXd dbg2;
        };
        inline Eigen::VectorXd EvDetail(AggregateComputer const& ac,
                                        StateType const& S,
                                        size_t pidx)
        {
                EvDetailDevice dev(pidx);
                ac.Observe( S, dev);
                return dev.A;
        }

        namespace computation_kernel{
                inline
                StateType& InplaceLinearCombination(StateType& x,
                                                    StateType const& y,
                                                    double alpha)
                {
                        for(size_t i=0;i!=x.size();++i){
                                for(size_t j=0;j!=x[i].size();++j){
                                        x[i][j] *= alpha;
                                        x[i][j] += y[i][j] * ( 1.0 - alpha );
                                }
                        }
                        return x;
                }
                inline
                StateType& InplaceClamp(StateType& x, double epsilon)
                {
                        for(size_t i=0;i!=x.size();++i){
                                for(size_t j=0;j!=x[i].size();++j){
                                        for(size_t cid=0;cid!=169;++cid){
                                                if( std::fabs( x[i][j][cid] ) < epsilon ){
                                                        x[i][j][cid] = 0.0;
                                                }
                                                if( std::fabs( 1.0 - x[i][j][cid] ) < epsilon ){
                                                        x[i][j][cid] = 1.0;
                                                }
                                        }
                                }
                        }
                        return x;
                }
                #if 0
                inline
                StateType CounterStrategy(std::shared_ptr<GameTree> gt, 
                                          GraphColouring<AggregateComputer> const& AG,
                                          StateType const& S,
                                          double delta)
                {
                        auto S_counter = S;
                        for(auto const& decision : *gt){

                                auto const& eval = AG.Color(decision.CommonRoot());
                                //auto const& eval = AG[root];

                                auto sidx = decision.GetIndex();
                                auto pidx = decision.GetPlayer();

                                // assume binary
                                auto Sp = S;
                                Sp[sidx][0].fill(1);
                                Sp[sidx][1].fill(0);

                                auto Sf = S;
                                Sf[sidx][0].fill(0);
                                Sf[sidx][1].fill(1);

                                EvDetailDevice po{pidx};
                                EvDetailDevice fo{pidx};


                                eval.Observe(Sp, po);
                                eval.Observe(Sf, fo);

                                po.Display(boost::lexical_cast<std::string>(decision.GetIndex()) + "push");
                                fo.Display(boost::lexical_cast<std::string>(decision.GetIndex()) + "fold");
                                
                                Eigen::VectorXd surface(169);
                                surface.fill(0.0);
                                surface = po.A - fo.A;

                                /*
                                 * want a forcing, such that  
                                 *      0 -> -A
                                 *      1 -> +B
                                 *      0.5 -> 0
                                 */
                                Eigen::VectorXd forcing(169);
                                forcing.fill(0.0);
                                auto scale = surface.maxCoeff();
                                for(size_t cid=0;cid!=169;++cid){
                                        auto a = S[sidx][0][cid];
                                        auto aa = a * a;
                                        auto b = S[sidx][1][cid];
                                        auto bb = b * b;
                                        auto c = a - b;
                                        forcing[cid] = scale * delta * c;
                                }

                                surface += forcing;

                                for(size_t idx=0;idx!=169;++idx){
                                        #if 0
                                        double x = ( po.A[idx] - 1e-3 > fo.A[idx] ? 1.0 : 0.0 );
                                        S_counter[sidx][0][idx] = x;
                                        S_counter[sidx][1][idx] = 1.0 - x;
                                        #endif
                                        // on edge cases we push
                                        double x = ( surface[idx] >= 0.0 ? 1.0 : 0.0 );
                                        S_counter[sidx][0][idx] = x;
                                        S_counter[sidx][1][idx] = 1.0 - x;

                                }
                        }
                        return S_counter;
                }
                #endif
                inline
                StateType CounterStrategy(std::shared_ptr<GameTree> gt, 
                                          GraphColouring<AggregateComputer> const& AG,
                                          StateType const& S,
                                          double delta)
                {
                        auto S_counter = S;
                        std::vector<
                                std::function<void()>
                        > tasks;
                        for(auto const& decision : *gt){
                                auto atom = [&](){
                                        auto const& eval = AG.Color(decision.CommonRoot());
                                        //auto const& eval = AG[root];

                                        auto sidx = decision.GetIndex();
                                        auto pidx = decision.GetPlayer();

                                        // assume binary
                                        auto Sp = S;
                                        Sp[sidx][0].fill(1);
                                        Sp[sidx][1].fill(0);

                                        auto Sf = S;
                                        Sf[sidx][0].fill(0);
                                        Sf[sidx][1].fill(1);

                                        EvDetailDevice po{pidx};
                                        EvDetailDevice fo{pidx};


                                        eval.Observe(Sp, po);
                                        eval.Observe(Sf, fo);

                                        po.Display(boost::lexical_cast<std::string>(decision.GetIndex()) + "push");
                                        fo.Display(boost::lexical_cast<std::string>(decision.GetIndex()) + "fold");
                                        
                                        Eigen::VectorXd surface(169);
                                        surface.fill(0.0);
                                        surface = po.A - fo.A;

                                        /*
                                         * want a forcing, such that  
                                         *      0 -> -A
                                         *      1 -> +B
                                         *      0.5 -> 0
                                         */
                                        Eigen::VectorXd forcing(169);
                                        forcing.fill(0.0);
                                        auto scale = surface.maxCoeff();
                                        for(size_t cid=0;cid!=169;++cid){
                                                auto a = S[sidx][0][cid];
                                                auto aa = a * a;
                                                auto b = S[sidx][1][cid];
                                                auto bb = b * b;
                                                auto c = a - b;
                                                forcing[cid] = scale * delta * c;
                                        }

                                        surface += forcing;

                                        for(size_t idx=0;idx!=169;++idx){
                                                #if 0
                                                double x = ( po.A[idx] - 1e-3 > fo.A[idx] ? 1.0 : 0.0 );
                                                S_counter[sidx][0][idx] = x;
                                                S_counter[sidx][1][idx] = 1.0 - x;
                                                #endif
                                                // on edge cases we push
                                                double x = ( surface[idx] >= 0.0 ? 1.0 : 0.0 );
                                                S_counter[sidx][0][idx] = x;
                                                S_counter[sidx][1][idx] = 1.0 - x;

                                        }
                                };
                                tasks.emplace_back(atom);
                        }

                        #if 1
                        std::vector<std::future<void> > futs;
                        for(auto& t : tasks ){
                                futs.push_back( std::async(std::launch::async, t) );
                        }
                        for(auto& f : futs){
                                f.get();
                        }
                        #else
                        for(auto& t : tasks ){
                                t();
                        }
                        #endif
                        return S_counter;
                }
                /*
                 * Returns a vector
                 *        (v0,v1,...,vn),
                 * such that vi is the number of cid's for which strategy si
                 * has mixed solutions. 
                 */
                struct MixedVectorResult{
                        std::vector<size_t>              mask;
                        std::vector<holdem_class_vector> cards;
                };
                inline
                MixedVectorResult MixedVector(std::shared_ptr<GameTree>,
                                                StateType const& S ){
                        //std::vector<size_t> v(S.size(), 0);
                        MixedVectorResult result;
                        result.mask.resize(S.size());
                        result.cards.resize(S.size());
                        for(size_t idx=0;idx!=S.size();++idx){
                                auto const& t = S[idx][0];
                                for(size_t cid=0;cid!=169;++cid){
                                        if( t[cid] != 1.0 && t[cid] != 0.0 ){
                                                //++v[idx];
                                                ++result.mask[idx];
                                                result.cards[idx].push_back(cid);
                                        }
                                }
                        }
                        return result;
                }
                /*
                 * Returns a vector
                 *        (v0,v1,...,vn),
                * such that vi is the number of cid's which is different from
                * the canonical counter strategy. 
                *     For example, if S is indeed a gto optimal strategy, we
                * would have that every non-mixed cid should be equal to the
                * counter strategy. Of coure it's not really this simple, but 
                * as a measure of convergenct
                */
                struct GammaVectorResult{
                        std::vector<size_t>              mask;
                        std::vector<holdem_class_vector> cards;
                        size_t                           level;
                        size_t                           sigma;
                };
                inline
                GammaVectorResult GammaVector( std::shared_ptr<GameTree> gt,
                                                 GraphColouring<AggregateComputer> const& AG,
                                                 StateType const& S)
                {
                        GammaVectorResult result;
                        result.mask.resize(S.size());
                        result.cards.resize(S.size());

                        // we have a mixed solution where the counter strategy 
                        // has only one cid different from our solutuon.
                        auto counter_nf = CounterStrategy(gt, AG, S, 0.0);
                        for(size_t idx=0;idx!=S.size();++idx){
                                auto A = S[idx][0] - counter_nf[idx][0];
                                for(size_t cid=0;cid!=169;++cid){
                                        if( A[cid] != 0.0 ){
                                                ++result.mask[idx];
                                                result.cards[idx].push_back(cid);
                                        }
                                }
                        }
                        for(size_t idx=0;idx!=S.size();++idx){
                                result.mask[idx] = result.cards[idx].size();
                        }
                        result.level = *std::max_element(result.mask.begin(), result.mask.end());
                        result.sigma = std::accumulate(result.mask.begin(), result.mask.end(), static_cast<size_t>(0));
                        return result;
                }
                inline
                bool IsMinMixedSolution(std::vector<size_t> const& gamma_vec){
                        enum{
                                T_Zero        = 0,
                                T_One         = 1,
                                T_Two         = 2,
                                T_ThreeOrMore = 3,
                        };
                        std::array<size_t, 4> M = {0, 0, 0, 0 };
                        for(size_t idx=0;idx!=gamma_vec.size();++idx){
                                auto j = std::min<size_t>(gamma_vec[idx], 3);
                                ++M[j];
                        }
                        if( M[T_ThreeOrMore] == 0 && M[T_Two] <= 1 ){
                                return true;
                        }
                        return false;
                }
                #if 0
                size_t MinMixedSolutionMetric(std::vector<size_t> const& gamma_vec){
                        size_t metric = 0;
                        for(size_t idx=0;idx!=gamma_vec.size();++idx){
                                metric += gamma_vec[idx] * gamma_vec[idx];
                        }
                        return metric;
                }
                #endif
        } // end namespace computation_kernel

        // Hmm don't like this. Need to figure out how make these less coupled
        struct Solution{
                /*
                 * This represents the strategy
                 */
                StateType S;
                Eigen::VectorXd EV;
                
                /*
                 * Because it's expensive for calculate
                 * the Counter strategy, we keep it here on
                 * the object rather than do the calculation
                 * multiple times
                 */
                StateType Counter;
                Eigen::VectorXd Counter_EV;

                double Norm;

                /*
                 * This is one of the classifications of the
                 * soltion, which represents how many cid's 
                 * are mixed soltions (not 0 or 1).
                 */
                std::vector<size_t> Mixed;
                std::vector<holdem_class_vector> MixedCards;
                /*
                 * This represents how many cid's are different
                 * from the counter strategy. This would never be
                 * all zero, but ideally all 1's, which is the manifestation
                 * of the optional solution being mixed in one card
                 */
                std::vector<size_t> Gamma;
                std::vector<holdem_class_vector> GammaCards;

                size_t Level;
                size_t Total;


                friend bool operator<(Solution const& l, Solution const& r){ 
                        if( l.Level != r.Level ){
                                return l.Level < r.Level;
                        } 
                        if( l.Total != r.Total ){
                                return l.Total < r.Total;
                        }
                        return l.Norm < r.Norm;
                }

                static Solution MakeWithDeps(std::shared_ptr<GameTree> gt,
                                             GraphColouring<AggregateComputer> const& AG,
                                             StateType const& S)
                {
                        auto root = gt->Root();
                        auto ev = AG.Color(root).ExpectedValue(S);

                        auto S_counter = computation_kernel::CounterStrategy(gt, AG, S, 0.0);
                        auto counter_ev = AG.Color(root).ExpectedValue(S_counter);

                        auto d = ev - counter_ev;

                        auto norm = d.lpNorm<Eigen::Infinity>();

                        auto mv_ret = computation_kernel::MixedVector( gt, S );
                        auto gv_ret = computation_kernel::GammaVector( gt, AG, S );

                        #if 0
                        auto level = *std::max_element(gv.begin(), gv.end());
                        auto Total = std::accumulate(gv.begin(), gv.end(), static_cast<size_t>(0));
                        #endif

                        return {S, ev, S_counter, counter_ev, norm, mv_ret.mask, mv_ret.cards, gv_ret.mask, gv_ret.cards, gv_ret.level, gv_ret.sigma};
                }
        };
        
        
        // this basically consumes a sequence of Solutions
        //      (S0,S1,S2,...),
        // and produces a sequence of best solutiins of the form,
        //      (B0,B1,B2,...).
        //
        // For example the sequence
        //      (5,3,6,3,1,55,3,...),
        // would correspond to 
        //      (5,5,6,6,6,55,55,...)
        //
        struct SequenceConsumer{
                enum Control{
                        Ctrl_Rejected,
                        Ctrl_Accepted,
                        Ctrl_Perfect,
                };
                // is_better(head, candidate)
                using te_is_better_type = std::function<bool(Solution const&, Solution const&)>;
                explicit SequenceConsumer(te_is_better_type const &is_better = std::less<Solution>{})
                        :is_better_{is_better}
                {}
                Control Consume(Solution const& sol){

                        if( seq_.size() ){
                                if( ! is_better_(sol, seq_.back() ) ){
                                        return Ctrl_Rejected;
                                }
                        }
                        seq_.push_back(sol);
                        Display();
                        if( sol.Level == 0 )
                                return Ctrl_Perfect;
                        return Ctrl_Accepted;
                }

                operator bool()const{
                        return Condition();
                }
                bool Condition()const{
                        if( seq_.empty()) 
                                return false;
                        return true;
                }
                boost::optional<Solution> AsOptSolution(){
                        if( ! Condition() )
                                return {};
                        return seq_.back();
                }
                boost::optional<StateType> AsOptState(){
                        if( ! Condition() )
                                return {};
                        return seq_.back().S;
                }

                void Display()const{
                        std::vector<Pretty::LineItem> dbg;
                        dbg.push_back(std::vector<std::string>{"Level", "Total", "|.|", "Gamma", "Mixed"});
                        dbg.push_back(Pretty::LineBreak);

                        for(auto const& sol : seq_){

                                std::vector<std::string> dbg_line;
                                dbg_line.push_back(boost::lexical_cast<std::string>(sol.Level));
                                dbg_line.push_back(boost::lexical_cast<std::string>(sol.Total));
                                dbg_line.push_back(boost::lexical_cast<std::string>(sol.Norm));
                                dbg_line.push_back(detail::to_string(sol.Gamma));
                                dbg_line.push_back(detail::to_string(sol.Mixed));
                                dbg.push_back(std::move(dbg_line));
                        }


                        Pretty::RenderTablePretty(std::cout, dbg);
                }
        private:
                std::vector<Solution> seq_;
                te_is_better_type is_better_;
        };


} // end namespace sim
} // end namespace ps

#endif // PS_SIM__EXTRA_H
