#include "ps/base/cards.h"
#include "ps/support/command.h"
#include "ps/detail/print.h"
#include "ps/eval/class_cache.h"
#include "ps/eval/holdem_class_vector_cache.h"
#include "app/pretty_printer.h"
#include "app/serialization_util.h"

#include <boost/timer/timer.hpp>

namespace ps{

        struct GNode;

        struct GEdge{
                GEdge(GNode* from, GNode* to)
                        :from_(from),
                        to_(to)
                {}
                GNode* From()const{ return from_; }
                GNode* To()const{ return to_; }

                friend std::ostream& operator<<(std::ostream& ostr, GEdge const& e);

        private:
                friend struct Graph;
                GNode* from_;
                GNode* to_;
        };

        struct GNode{
                explicit GNode(std::string const& name)
                        :name_(name)
                {}


                std::vector<GEdge const*> EdgePath()const{
                        std::vector<GEdge const*> rpath;
                        GNode const* head = this;
                        for(;; ){
                                // assume tree
                                if( head->in_.size() != 1 ){
                                        break;
                                }
                                auto e = head->in_.back();
                                rpath.push_back(head->in_.back());
                                head = rpath.back()->From();
                        }
                        return std::vector<GEdge const*>(rpath.rbegin(), rpath.rend());
                }

                std::string const& Name()const{ return name_; }
                
                std::vector<GNode*> TerminalNodes(){
                        std::vector<GNode*> terminals;
                        std::vector<GNode*> stack{this};
                        for(;stack.size();){
                                auto head = stack.back();
                                stack.pop_back();
                                if( head->IsTerminal() ){
                                        terminals.push_back(head);
                                } else{
                                        for(auto e : head->out_){
                                                stack.push_back(e->To());
                                        }
                                }
                        }
                        return terminals;
                }
                bool IsTerminal()const{ return out_.empty(); }
        

                friend std::ostream& operator<<(std::ostream& ostr, GNode const& self){
                        ostr << "{name=" << self.name_ << "}";
                        return ostr;
                }

                auto const& OutEdges(){ return out_; }

        private:
                friend struct Graph;
                std::string name_;
                std::vector<GEdge*> in_;
                std::vector<GEdge*> out_;
        };

        std::ostream& operator<<(std::ostream& ostr, GEdge const& e){
                return ostr << "{from=" << e.from_->Name() << ", to=" << e.to_->Name() << "}";
        }

        struct Graph{
                GNode* Node(std::string const& name_){
                        N.push_back(std::make_shared<GNode>(name_));
                        return N.back().get();
                }
                GEdge* Edge(GNode* a, GNode* b){
                        auto e = std::make_shared<GEdge>(a, b);
                        a->out_.push_back(e.get());
                        b->in_ .push_back(e.get());
                        E.push_back(e);
                        return E.back().get();
                }
        private:
                std::vector<std::shared_ptr<GNode> > N;
                std::vector<std::shared_ptr<GEdge> > E;
        };

        template<class T>
        struct GraphColouring : std::unordered_map<void const*, T>{
                T const& Color(void const* e)const{
                        auto iter = this->find(e);
                        if( iter == this->end())
                                BOOST_THROW_EXCEPTION(std::domain_error("no colour"));
                        return iter->second;
                }
        };
        
        struct Index{
                friend std::ostream& operator<<(std::ostream& ostr, Index const& self){
                        ostr << "{s = " << self.s;
                        ostr << ", choice = " << self.choice;
                        ostr << ", id = " << (int)self.id << "}";
                        return ostr;
                }
                size_t s;
                size_t choice;
                holdem_class_id id{static_cast<holdem_class_id>(-1)};
        };

        struct Decision{
                Decision(size_t ID, size_t player_idx)
                        :ID_(ID),
                        player_idx_(player_idx)
                {}
                friend std::ostream& operator<<(std::ostream& ostr, Decision const& self){
                        ostr << "ID_ = " << self.ID_;
                        ostr << ", player_idx_ = " << self.player_idx_;
                        ostr << ", CommonRoot() -> " << self.CommonRoot()->Name();
                        return ostr;
                }
                void Add(GEdge* e){ E.push_back(e); }
                size_t OffsetFor(GEdge const* e)const{
                        auto begin = E.begin();
                        auto end = E.end();
                        auto iter = std::find(begin, end, e);
                        if( iter == end)
                                BOOST_THROW_EXCEPTION(std::domain_error("not an edge"));
                        return static_cast<size_t>(std::distance(begin, iter));
                }
                Index IndexFor(GEdge const* e, holdem_class_id cid)const{
                        return { ID_, OffsetFor(e), cid};
                }

                size_t GetIndex()const{ return ID_; }
                size_t GetPlayer()const{ return player_idx_; }

                auto begin()const{ return E.begin(); }
                auto end()const{ return E.end(); }

                size_t size()const{ return E.size(); }

                auto const& Edges()const{ return E; }

                GNode* CommonRoot()const{
                        // should all be the same 
                        return E.back()->From();
                }
                
        private:
                size_t ID_;
                size_t player_idx_;
                std::vector<GEdge*> E;
        };

        using StateType = std::vector<std::vector<Eigen::VectorXd> >;

                
        struct StrategyDecl{
                using decision_vector = std::vector<std::shared_ptr<Decision> >;
                using decision_iterator = boost::indirect_iterator<decision_vector::const_iterator>;
                decision_iterator begin()const{ return v_.begin(); }
                decision_iterator end()const{ return v_.end(); }
                void Add(std::shared_ptr<Decision> ptr){
                        v_.push_back(ptr);
                }
                size_t NumDecisions()const{ return v_.size(); }
                size_t Depth(size_t j)const{ return v_[j]->size(); }

                StateType MakeDefaultState()const{
                        std::vector<std::vector<Eigen::VectorXd> > S;
                        for(auto const& d : *this){
                                Eigen::VectorXd proto(169);
                                proto.fill(1.0 / d.size());
                                S.emplace_back(d.size(), proto);
                        }
                        return S;
                }
                void Display()const{
                        for(auto d : v_){
                                std::cout << *d << "\n";
                        }
                }
        private:
                std::vector<std::shared_ptr<Decision> > v_;
        };


        /*
         * This capture the bulk of the computation. Evaluating the EV of a poker strategy
         * can be represented as 
         *      EV[S] = \sigma cv \in CV P(cv) * EV[S|cv]
         *            =          ...           * ( \sigma e \in E EV[S|e,cv],
         *    which means that the expected value of S, EV[S] -> R^n, is equial to the weighted
         * sum of the expected vlaue of each deal and terminal event. For hu
         */
        struct Computer{

                struct Atom{
                        holdem_class_vector cv;
                        double constant{1.0};
                        std::vector<Index> index;
                        Eigen::VectorXd value;
                };
                template<class Observer>
                void Observe(StateType const& S, Observer& obs)const noexcept {
                        for(auto const& _ : atoms_){
                                double c = _.constant;
                                for(auto const& idx : _.index){
                                        c *= S[idx.s][idx.choice][idx.id];
                                }
                                obs(_.cv, c, _.value);
                        }
                }
                template<class Filter>
                void EvaluateFiltered(Eigen::VectorXd& R, StateType const& S, Filter&& filter)const noexcept {
                        for(auto const& _ : atoms_){
                                if( ! filter(_.cv) )
                                        continue;
                                double c = _.constant;
                                for(auto const& idx : _.index){
                                        c *= S[idx.s][idx.choice][idx.id];
                                }
                                R += c * _.value;
                        }
                }
                void Evaluate(Eigen::VectorXd& R, StateType const& S)const noexcept {
                        EvaluateFiltered(R, S, [](auto&&){return true; });
                }
                void Emplace(holdem_class_vector const& cv, double constant, std::vector<Index> index, Eigen::VectorXd value){
                        atoms_.emplace_back(Atom{cv, constant, index, value});
                }
        private:
                std::vector<Atom> atoms_;
        };

        struct AggregateComputer : std::vector<std::shared_ptr<Computer> >{
                template<class Observer>
                void Observe(StateType const& S, Observer& obs)const noexcept {
                        for(auto const& ptr : *this){
                                ptr->Observe(S, obs);
                        }
                }
                template<class Filter>
                void EvaluateFiltered(Eigen::VectorXd& R, StateType const& S, Filter&& filter)const noexcept {
                        for(auto const& ptr : *this){
                                ptr->EvaluateFiltered(R, S, filter);
                        }
                }
                void Evaluate(Eigen::VectorXd& R, StateType const& S)const noexcept {
                        EvaluateFiltered(R, S, [](auto&&){return true; });
                }
        };

        /*
         * For solving poker push-fold, we need some representation of a numerical solution.
         * For push fold, I'm representing the hu solution as
         *                  {0,1} x {push,fold} x {0,1}^169,
         * corresponding to sb push/fold choice for 0, and bb call/fold decision given sb
         * push for 1. either [0][0][JJ] is the probailty that sb push's JJ, whilst
         * [1][1][QQ] is the probabily that bb calls with QQ.
         *    I capture that inside the the std::vector<Index>, which corresponds how to
         * assign a probability to a path. We want to evaluate the probability of 
         * event (terminal) happening for a certain deal, ei
         *          P(e=PP|cv={AKs,33}) = S[0][0][AKs]*S[1][0][33].
         *    Now we will always have
         *          P(cv={AKs,33}) = P(e=PP,cv={AKs,33}) + P(e=PF,cv={AKs,33}) + P(e=F,cv={AKs,33}) = 1,
         *    Ie that it forms a probability distribution.
         */
        struct IndexMakerConcept{
                virtual ~IndexMakerConcept()=default;
                virtual std::vector<Index> MakeIndex(std::vector<GEdge const*> const& path, holdem_class_vector const& cv)const=0;
        };
        struct IndexMaker : IndexMakerConcept{
                explicit IndexMaker(StrategyDecl const& S){
                        for(auto d : S){
                                for(auto e : d){
                                        D[e] = d.GetIndex();
                                        P[e] = d.GetPlayer();
                                        I[e] = d.OffsetFor(e);
                                }
                        }
        
                }
                virtual std::vector<Index> MakeIndex(std::vector<GEdge const*> const& path, holdem_class_vector const& cv)const override{
                        std::vector<Index> index;
                        for(auto e : path){
                                index.push_back( Index{ D.Color(e), I.Color(e), cv[P.Color(e)] } );
                        }
                        return index;
                }
        private:
                GraphColouring<size_t> D;
                GraphColouring<size_t> P;
                GraphColouring<size_t> I;
        };

        struct MakerConcept{
                virtual ~MakerConcept()=default;
                virtual void Emit(IndexMakerConcept* im, class_cache const* cache, double p, holdem_class_vector const& cv)const=0;
                virtual std::string to_string()const=0;
        };
        struct StaticEmit : MakerConcept{
                StaticEmit(std::shared_ptr<Computer> comp, std::vector<GEdge const*> path, std::vector<size_t> const& active, Eigen::VectorXd pot)
                        :comp_{comp},
                        path_(path),
                        active_(active),
                        pot_(pot)
                {}
                virtual void Emit(IndexMakerConcept* im, class_cache const* cache, double p, holdem_class_vector const& cv)const override{
                        
                        auto index = im->MakeIndex(path_, cv); 
                        
                        Eigen::VectorXd v(pot_.size());
                        v.fill(0);
                        v -= pot_;

                        if( active_.size() == 1 ){
                                v[active_[0]] += pot_.sum();
                        } else {
                                holdem_class_vector auxcv;
                                for(auto idx : active_ ){
                                        auxcv.push_back(cv[idx]);
                                }
                                auto ev = cache->LookupVector(auxcv);
                                Eigen::VectorXd evaux(cv.size());
                                evaux.fill(0);
                                size_t ev_idx = 0;
                                for(auto idx : active_ ){
                                        evaux[idx] = ev[ev_idx];
                                        ++ev_idx;
                                }
                                evaux *= pot_.sum();

                                v += evaux;
                        }

                        comp_->Emplace(cv, p, index, v);
                }
                virtual std::string to_string()const override{
                        std::stringstream sstr;
                        std::stringstream pathsstr;
                        pathsstr << path_.front()->From()->Name();
                        for(auto p : path_){
                                pathsstr << "," << p->To()->Name();
                        }
                        if( active_.size() == 1 ){
                                sstr << "Static{";
                                sstr << "path=" << pathsstr.str();
                        } else {
                                sstr << "Eval  {";
                                sstr << "path=" << pathsstr.str();
                        }
                        return sstr.str();
                }

        private:
                std::shared_ptr<Computer> comp_;
                std::vector<GEdge const*> path_;
                std::vector<size_t> active_;
                Eigen::VectorXd pot_;
        };

        struct PushFoldState{
                std::set<size_t> Active;
                Eigen::VectorXd Pot;
                Eigen::VectorXd Stacks;
                void Display()const{
                        std::cout << "detail::to_string(Active) => " << detail::to_string(Active) << "\n"; // __CandyPrint__(cxx-print-scalar,detail::to_string(Active))
                        std::cout << "vector_to_string(Pot) => " << vector_to_string(Pot) << "\n"; // __CandyPrint__(cxx-print-scalar,vector_to_string(Pot))
                        std::cout << "vector_to_string(Stacks) => " << vector_to_string(Stacks) << "\n"; // __CandyPrint__(cxx-print-scalar,vector_to_string(Stacks))
                }
        };
        struct Fold{
                size_t player_idx;
                void operator()(PushFoldState& S)const{
                        S.Active.erase(player_idx);
                }
        };
        struct Push{
                size_t player_idx;
                void operator()(PushFoldState& S)const{
                        auto x = S.Stacks[player_idx];
                        S.Stacks[player_idx] = 0.0;
                        S.Pot[player_idx] += x;
                }
        };
        using PushFoldOperator = std::function<void(PushFoldState&)>;

        boost::optional<StateType> Solve(holdem_binary_strategy_ledger_s& ledger, size_t n, double sb, double bb, double eff){
                std::cout << "Solve\n"; // __CandyPrint__(cxx-print-scalar,Solve)


                #if 1
                auto G = std::make_shared<Graph>();
                
                // <0>
                auto root = G->Node("*");
                        // <1>
                        auto p = G->Node("p");
                                auto pp = G->Node("pp");
                                auto pf = G->Node("pf");
                        auto f = G->Node("f");



                // decision <0>
                auto e_0_p = G->Edge(root, p);
                auto e_0_f = G->Edge(root, f);

                // decision <1>
                auto e_1_p = G->Edge(p, pp);
                auto e_1_f = G->Edge(p, pf);
                
                /*
                 * We need to color each non-terminal node which which player's
                 * turn it is
                 */
                GraphColouring<size_t> P;
                P[root] = 0;
                P[p]    = 1;
                
                /*
                 * We color each node with an operator, this is how we figure 
                 * out what state we will be in for the terminal
                 */
                GraphColouring<PushFoldOperator> ops;
                ops[e_0_p] = Push{0};
                ops[e_0_f] = Fold{0};
                ops[e_1_p] = Push{1};
                ops[e_1_f] = Fold{1};


                size_t N = 2;

                /*
                 * We create the inital state of the game, this is used 
                 * for evaulting terminal nodes
                 */
                PushFoldState state0;
                state0.Active.insert(0);
                state0.Active.insert(1);
                state0.Pot.resize(2);
                state0.Pot[0] = sb;
                state0.Pot[1] = bb;
                state0.Stacks.resize(2);
                state0.Stacks[0] = eff - sb;
                state0.Stacks[1] = eff - bb;


                #else


                auto G = std::make_shared<Graph>();
                
                // <0>
                auto root = G->Node("*");
                        // <1>
                        auto p = G->Node("p");
                                // <3>
                                auto pp = G->Node("pp");
                                        auto ppp = G->Node("ppp");
                                        auto ppf = G->Node("ppf");
                                // <4>
                                auto pf = G->Node("pf");
                                        auto pfp = G->Node("pfp");
                                        auto pff = G->Node("pff");
                        // <2>
                        auto f = G->Node("f");
                                // <5>
                                auto fp = G->Node("fp");
                                        auto fpp = G->Node("fpp");
                                        auto fpf = G->Node("fpf");
                                auto ff = G->Node("ff");



                // decision <0>
                auto e_0_p = G->Edge(root, p);
                auto e_0_f = G->Edge(root, f);

                // decision <1>
                auto e_1_p = G->Edge(p, pp);
                auto e_1_f = G->Edge(p, pf);
                
                // decision <2>
                auto e_2_p = G->Edge(f, fp);
                auto e_2_f = G->Edge(f, ff);

                // decision <3>
                auto e_3_p = G->Edge(pp, ppp);
                auto e_3_f = G->Edge(pp, ppf);

                // decision <4>
                auto e_4_p = G->Edge(pf, pfp);
                auto e_4_f = G->Edge(pf, pff);
                
                // decision <5>
                auto e_5_p = G->Edge(fp, fpp);
                auto e_5_f = G->Edge(fp, fpf);

                
                /*
                 * We need to color each non-terminal node which which player's
                 * turn it is
                 */
                GraphColouring<size_t> P;
                P[root] = 0;
                P[p]    = 1;
                P[f]    = 1;
                P[pp]   = 2;
                P[pf]   = 2;
                P[fp]   = 2;
                P[ff]   = 2;
                
                /*
                 * We color each node with an operator, this is how we figure 
                 * out what state we will be in for the terminal
                 */
                GraphColouring<PushFoldOperator> ops;
                ops[e_0_p] = Push{0};
                ops[e_0_f] = Fold{0};

                ops[e_1_p] = Push{1};
                ops[e_1_f] = Fold{1};
                ops[e_2_p] = Push{1};
                ops[e_2_f] = Fold{1};

                ops[e_3_p] = Push{2};
                ops[e_3_f] = Fold{2};
                ops[e_4_p] = Push{2};
                ops[e_4_f] = Fold{2};
                ops[e_5_p] = Push{2};
                ops[e_5_f] = Fold{2};


                size_t N = 3;


                /*
                 * We create the inital state of the game, this is used 
                 * for evaulting terminal nodes
                 */
                PushFoldState state0;
                state0.Active.insert(0);
                state0.Active.insert(1);
                state0.Active.insert(2);
                state0.Pot.resize(3);
                state0.Pot[0] = 0.0;
                state0.Pot[1] = sb;
                state0.Pot[2] = bb;
                state0.Stacks.resize(3);
                state0.Stacks[0] = eff;
                state0.Stacks[1] = eff - sb;
                state0.Stacks[2] = eff - bb;


                #endif


                /*
                 * We not iterate over the graph, and for each non-terminal node
                 * we allocate a decision to that node. This Decison then
                 * takes the player index, and strategy index.
                 */
                std::vector<GNode*> stack{root};
                auto strat = std::make_shared<StrategyDecl>();
                size_t dix = 0;
                for(;stack.size();){
                        auto head = stack.back();
                        stack.pop_back();
                        if( head->IsTerminal() )
                                continue;
                        auto d = std::make_shared<Decision>(dix++, P[head]);
                        strat->Add(d);
                        for( auto e : head->OutEdges() ){
                                d->Add(e);
                                stack.push_back(e->To());
                        }
                }


                for( auto t : root->TerminalNodes()){
                        std::cout << "t->Name() => " << t->Name() << "\n"; // __CandyPrint__(cxx-print-scalar,t->Name())
                }
                strat->Display();

                
                /*
                 * To simplify the construction, we first create a object which represents
                 * the teriminal state of the game, for hu we have 3 terminal states
                 *                        {f,pf,pp},
                 * with the first two being independent of the dealt hand, whilst the 
                 * last state pp required all in equity evaluation. We create a vector
                 * of each of these terminal states, then which create Computer objects
                 * for fast computation. This is mainly to simplify the code.
                 */
                std::vector<std::shared_ptr<MakerConcept> > maker_dev;

                /*
                 * Now we go over all the terminal nodes, which is basically a path to
                 * something that can be evaluated. 
                 */
                GraphColouring<AggregateComputer> AG;
                std::vector<Computer*> tc_aux;
                for(auto t : root->TerminalNodes() ){

                        auto path = t->EdgePath();
                        auto ptr = std::make_shared<Computer>();
                        AG[t].push_back(ptr);
                        for(auto e : path ){
                                AG[e->From()].push_back(ptr); 
                        }

                        /*
                         * Work out the terminal state, independent of the deal, and
                         * make an factory object
                         */
                        auto state = state0;
                        for( auto e : path ){
                                ops[e](state);
                        }
                        std::vector<size_t> perm(state.Active.begin(), state.Active.end());
                        maker_dev.push_back(std::make_shared<StaticEmit>(ptr, path, perm, state.Pot) );

                }




                
                std::string cache_name{".cc.bin"};
                class_cache C;
                C.load(cache_name);

                IndexMaker im(*strat);
                //for(auto const& group : *Memory_TwoPlayerClassVector){
                for(auto const& group : *Memory_TwoPlayerClassVector){
                        for(auto const& _ : group.vec){
                                for(auto& m : maker_dev ){
                                        m->Emit(&im, &C, _.prob, _.cv );
                                }
                        }
                }




                decltype(strat->MakeDefaultState()) S0;
                if( ledger.size()){
                        S0 = ledger.back().to_eigen_vv();
                } else {
                        S0 = strat->MakeDefaultState();
                }

                auto S = S0;

                std::function<void(std::vector<std::vector<Eigen::VectorXd> > const&, Eigen::VectorXd const&, double norm)> obs;
                struct Printer{
                        Printer(size_t n):n_{n}{
                                if(  n ==  2){
                                        lines_.emplace_back( std::vector<std::string>{"EV[0]", "EV[1]", "|.|"});
                                        lines_.emplace_back(Pretty::LineBreak);
                                }
                        }
                        void operator()(std::vector<std::vector<Eigen::VectorXd> > const&, Eigen::VectorXd const& ev, double norm){
                                lines_.emplace_back(
                                        std::vector<std::string>{
                                                    boost::lexical_cast<std::string>(ev[0]),
                                                    boost::lexical_cast<std::string>(ev[1]),
                                                    boost::lexical_cast<std::string>(norm)
                                        }
                                );
                                Pretty::RenderTablePretty(std::cout, lines_);
                        }
                private:
                        size_t n_;
                        std::vector<Pretty::LineItem> lines_;
                };
                Printer printer{N};
                obs = printer;

                for(size_t n=0;n!=200;++n){
                        boost::timer::auto_cpu_timer at;
                        auto S_counter = S;
                        auto S_Before = S;

                        enum{ InnerLoop = 3 };
                        for(size_t inner=0;inner!=InnerLoop;++inner){
                                for(auto const& decision : *strat){

                                        auto head = decision.CommonRoot();
                                        auto const& eval = AG[head];

                                        auto sidx = decision.GetIndex();
                                        auto pidx = decision.GetPlayer();

                                        // assume binary
                                        auto Sp = S;
                                        Sp[sidx][0].fill(1);
                                        Sp[sidx][1].fill(0);

                                        auto Sf = S;
                                        Sf[sidx][0].fill(0);
                                        Sf[sidx][1].fill(1);

                                        struct observer : boost::noncopyable{
                                                observer(size_t pidx)
                                                        :pidx_(pidx)
                                                {
                                                        A.fill(0.0);
                                                }
                                                void operator()(holdem_class_vector const& cv, double c, Eigen::VectorXd const& value){
                                                        A[cv[pidx_]] += c * value[pidx_];
                                                }
                                                size_t pidx_;
                                                std::array<double, 169> A;
                                        };
                                        observer po{pidx};
                                        observer fo{pidx};


                                        eval.Observe(Sp, po);
                                        eval.Observe(Sf, fo);
                                        for(size_t idx=0;idx!=169;++idx){
                                                double x = ( po.A[idx] >= fo.A[idx] ? 1.0 : 0.0 );
                                                S_counter[sidx][0][idx] = x;
                                                S_counter[sidx][1][idx] = 1.0 - x;
                                        }
                                }


                                double factor = 0.05;
                                for(size_t i=0;i!=strat->NumDecisions();++i){
                                        for(size_t j=0;j!=strat->Depth(i);++j){
                                                S[i][j] = S[i][j] * ( 1.0 - factor ) + factor * S_counter[i][j];
                                        }
                                }
                        }
                        ledger.push(S);
                        

                        Eigen::VectorXd R_counter(N);
                        R_counter.fill(0);
                        AG[root].Evaluate(R_counter, S_counter);

                        Eigen::VectorXd R_Before(N);
                        R_Before.fill(0);
                        AG[root].Evaluate(R_Before, S_Before);

                        double norm = ( R_Before - R_counter ).lpNorm<2>();

                        if( obs ){
                                obs(S, R_Before, norm);
                        }

                        if( norm < 0.0001 ){
                                return S_counter;
                        }

                        std::cout << "vector_to_string(R_Before) => " << vector_to_string(R_Before) << "\n"; // __CandyPrint__(cxx-print-scalar,vector_to_string(ev))
                        std::cout << "norm => " << norm << "\n"; // __CandyPrint__(cxx-print-scalar,norm)
                        for(size_t idx=0;idx!=S.size();++idx){
                                pretty_print_strat(S[idx][0], 1);
                        }

                }
                return boost::none;


        }

        struct Driver{
                Driver(){
                        ss_.try_load_or_default(".ss.bin");
                }
                ~Driver(){
                        ss_.save_();
                }
                boost::optional<std::vector<std::vector<Eigen::VectorXd> >> FindOrBuildSolution(size_t n, double sb, double bb, double eff){
                        std::stringstream sstr;
                        sstr << n << ":" << sb << ":" << bb << ":" << eff;
                        auto key = sstr.str();
                        #if 1
                        auto iter = ss_.find(key);
                        if( iter != ss_.end()){
                                return iter->second.to_eigen_vv();
                        }
                        #endif

                        auto ledger_key = ".ledger/" + key;
                        holdem_binary_strategy_ledger_s ledger;
                        ledger.try_load_or_default(ledger_key);
                        #if 0
                        if( ledger.size() ){
                                ss_.add_solution(key, ledger.back());
                                ss_.save_();
                                ledger.save_();
                                return ledger.back().to_eigen_vv();
                        }
                        #endif
                        auto sol = Solve(ledger, n, sb, bb, eff);

                        if( sol ){
                                ss_.add_solution(key, *sol);
                                #if 0
                                holdem_binary_strategy_s aux(*sol);
                                aux.to_eigen_vv();
                                pretty_print_strat(sol.get()[0][0], 1);
                                pretty_print_strat(sol.get()[1][0], 1);
                                auto copy = ss_.find(key)->second.to_eigen_vv();
                                pretty_print_strat(copy[0][0], 1);
                                pretty_print_strat(copy[1][0], 1);
                                ss_.save_();
                                holdem_binary_solution_set_s ss_copy;
                                ss_copy.load(".ss.bin");
                                copy = ss_copy.find(key)->second.to_eigen_vv();
                                pretty_print_strat(copy[0][0], 1);
                                pretty_print_strat(copy[1][0], 1);
                                #endif
                        }
                        return sol; // might be nothing
                }
                void Display()const{
                        for(auto const& _ : ss_){
                                std::cout << "_.first => " << _.first << "\n"; // __CandyPrint__(cxx-print-scalar,_.first)
                        }
                }
        private:
                holdem_binary_solution_set_s ss_;
        };
        
        struct ScratchCmd : Command{
                enum{ Debug = 1};
                explicit
                ScratchCmd(std::vector<std::string> const& args):args_{args}{}
                virtual int Execute()override{
                        double sb = 0.5;
                        double bb = 1.0;
                        #if 0
                        Driver dvr;
                        auto S = dvr.FindOrBuildSolution(2, sb, bb, 10.0 );
                        pretty_print_strat(S[0][0], 1);
                        pretty_print_strat(S[1][0], 1);
                        #endif
                        #if 1
                        Driver dvr;
                        dvr.Display();
                        std::vector<Eigen::VectorXd> S(2);
                        S[0].resize(169);
                        S[0].fill(0);
                        S[1].resize(169);
                        S[1].fill(0);


                        for(double eff = 2.0;eff - 1e-4 < 25.0; eff += 0.05 ){
                                std::cout << "eff => " << eff << "\n"; // __CandyPrint__(cxx-print-scalar,eff)
                                auto opt = dvr.FindOrBuildSolution(2, sb, bb, eff );
                                if( opt ){
                                        for(size_t j=0;j!=2;++j){
                                                for(size_t idx=0;idx!=169;++idx){
                                                        if( std::fabs(opt.get()[j][0][idx] - 1.0 ) < 1e-2 ){
                                                                S[j][idx] = eff;
                                                        }
                                                }
                                        }
                                }
                        }
                        pretty_print_strat(S[0], 2);
                        pretty_print_strat(S[1], 2);
                        #endif
                        #if 0
                        auto opt = Solve(sb, bb, 10.0);
                        if( opt ){
                                pretty_print_strat(opt.get()[0][0], 0);
                                pretty_print_strat(opt.get()[1][0], 0);
                        }
                        #endif
                        return EXIT_SUCCESS;
                }
        private:
                std::vector<std::string> const& args_;
        };
        static TrivialCommandDecl<ScratchCmd> ScratchCmdDecl{"scratch"};
} // end namespace ps
