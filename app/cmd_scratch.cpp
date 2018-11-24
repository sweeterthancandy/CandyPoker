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
                Decision(size_t ID, std::string const& action, size_t player_idx)
                        :ID_(ID),
                        action_{action},
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

                std::string const& PrettyAction()const{ return action_; }
                
        private:
                size_t ID_;
                std::string action_;
                size_t player_idx_;
                std::vector<GEdge*> E;
        };

        using StateType = std::vector<std::vector<Eigen::VectorXd> >;

                


        /*
         * This capture the bulk of the computation. Evaluating the EV of a poker strategy
         * can be represented as 
         *      EV[S] = \sigma cv \in CV P(cv) * EV[S|cv]
         *            =          ...           * ( \sigma e \in E EV[S|e,cv],
         *    which means that the expected value of S, EV[S] -> R^n, is equial to the weighted
         * sum of the expected vlaue of each deal and terminal event. For hu
         */
        struct Computer{

                explicit Computer(std::string const& name):name_{name}{}

                struct Atom{
                        holdem_class_vector cv;
                        double constant{1.0};
                        std::vector<Index> index;
                        Eigen::VectorXd value;
                };
                // obs( deal, P(deal), P(e|S,deal), Value)
                template<class Observer>
                void Observe(StateType const& S, Observer& obs)const noexcept {
                        obs.decl_section(Name());
                        for(auto const& _ : atoms_){
                                //double c = _.constant;
                                double c = 1.0;
                                for(auto const& idx : _.index){
                                        c *= S[idx.s][idx.choice][idx.id];
                                }
                                obs(_.cv, _.constant, c, _.value);
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

                std::string const& Name()const{ return name_; }
        private:
                std::string name_;
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

        struct MakerConcept{
                virtual ~MakerConcept()=default;
                virtual void Emit(IndexMakerConcept* im, class_cache const* cache, double p, holdem_class_vector const& cv)const=0;
                virtual std::vector<std::string> to_string_line()const=0;

                static void Display( std::vector<std::shared_ptr<MakerConcept> > const& v){
                        std::vector<Pretty::LineItem> lines;
                        lines.push_back(std::vector<std::string>{"Path", "Impl", "Active", "Pot"});
                        lines.push_back(Pretty::LineBreak);
                        for(auto const& ptr : v){
                                lines.push_back( ptr->to_string_line());
                        }
                        Pretty::RenderTablePretty(std::cout, lines);
                }
        };
        inline std::string path_to_string(std::vector<GEdge const*> const& path){
                std::stringstream pathsstr;
                pathsstr << path.front()->From()->Name();
                for(auto p : path){
                        pathsstr << "," << p->To()->Name();
                }
                return pathsstr.str();
        }

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
                virtual std::vector<std::string> to_string_line()const override{
                        std::vector<std::string> line;
                        line.push_back(path_to_string(path_));
                        if( active_.size() == 1 ){
                                line.push_back("Static");
                        } else {
                                line.push_back("Eval");
                        }
                        line.push_back(detail::to_string(active_));
                        line.push_back(vector_to_string(pot_));
                        return line;
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
        struct Raise{
                size_t player_idx;
                double amt;
                void operator()(PushFoldState& S)const{
                        S.Stacks[player_idx] -= amt;
                        S.Pot[player_idx] += amt;
                }
        };
        using PushFoldOperator = std::function<void(PushFoldState&)>;

        struct GameTree{
                /*
                 * We need to color each non-terminal node which which player's
                 * turn it is
                 */
                virtual void ColorPlayers(GraphColouring<size_t>& P)const=0;
                /*
                 * We color each node with an operator, this is how we figure 
                 * out what state we will be in for the terminal
                 */
                virtual void ColorOperators(GraphColouring<PushFoldOperator>& ops)const=0;
                
                virtual void ColorPrettyActions(GraphColouring<std::string>& A)const=0;
                /*
                 * We create the inital state of the game, this is used 
                 * for evaulting terminal nodes
                 */
                virtual PushFoldState InitialState()const=0;
                virtual GNode* Root()=0;
                virtual size_t NumPlayers()const=0;

                virtual std::string StringDescription()const=0;

                /*
                 * We have this because we need to visit every permutation of the form
                 *                  ( (P(cv_0),cv_0), (P(cv_1),cv_1), ... )
                 * which that 
                 *                  \sigma_n P(cv_n) = 1.
                 * As an implementation detail, we cache this in memory, so we have this
                 * function
                 */
                virtual void VisitProbabilityDist(std::function<void(double, holdem_class_vector const& cv)> const& V)const noexcept=0;


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
        
        protected:
                void BuildStrategy(){
                        /*
                         * We not iterate over the graph, and for each non-terminal node
                         * we allocate a decision to that node. This Decision then
                         * takes the player index, and strategy index.
                         */
                        GraphColouring<size_t> P;
                        GraphColouring<std::string> A;
                        ColorPlayers(P);
                        ColorPrettyActions(A);
                        std::vector<GNode*> stack{Root()};
                        size_t dix = 0;
                        for(;stack.size();){
                                auto head = stack.back();
                                stack.pop_back();
                                if( head->IsTerminal() )
                                        continue;
                                auto d = std::make_shared<Decision>(dix++, A[head], P[head]);
                                this->Add(d);
                                for( auto e : head->OutEdges() ){
                                        d->Add(e);
                                        stack.push_back(e->To());
                                }
                        }
                }
        private:
                std::vector<std::shared_ptr<Decision> > v_;
        };


        struct IndexMaker : IndexMakerConcept{
                explicit IndexMaker(GameTree const& S){
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


        struct GameTreeTwoPlayer : GameTree{
                GameTreeTwoPlayer(double sb, double bb, double eff){
                        // <0>
                        root = G.Node("*");
                                // <1>
                                p = G.Node("p");
                                        pp = G.Node("pp");
                                        pf = G.Node("pf");
                                f = G.Node("f");

                        // decision <0>
                        e_0_p = G.Edge(root, p);
                        e_0_f = G.Edge(root, f);

                        // decision <1>
                        e_1_p = G.Edge(p, pp);
                        e_1_f = G.Edge(p, pf);
                
                        state0.Active.insert(0);
                        state0.Active.insert(1);
                        state0.Pot.resize(2);
                        state0.Pot[0] = sb;
                        state0.Pot[1] = bb;
                        state0.Stacks.resize(2);
                        state0.Stacks[0] = eff - sb;
                        state0.Stacks[1] = eff - bb;

                        std::stringstream sstr;
                        sstr << "GameTreeTwoPlayer:" << sb << ":" << bb << ":" << eff;
                        sdesc_ = sstr.str();
                        
                        BuildStrategy();

                }
                virtual std::string StringDescription()const override{ return sdesc_; }
                virtual void ColorPlayers(GraphColouring<size_t>& P)const override{
                        P[root] = 0;
                        P[p]    = 1;
                }
                virtual void ColorOperators(GraphColouring<PushFoldOperator>& ops)const override{
                        ops[e_0_p] = Push{0};
                        ops[e_0_f] = Fold{0};
                        ops[e_1_p] = Push{1};
                        ops[e_1_f] = Fold{1};
                }
                virtual void ColorPrettyActions(GraphColouring<std::string>& A)const override{
                        A[root] = "sb push/fold";
                        A[p]    = "bb call/fold, given bb push";
                }
                virtual PushFoldState InitialState()const override{
                        return state0;
                }
                virtual GNode* Root()override{ return root; }
                virtual size_t NumPlayers()const override{ return 2; }
                
                virtual void VisitProbabilityDist(std::function<void(double, holdem_class_vector const& cv)> const& V)const noexcept override{
                        for(auto const& group : *Memory_TwoPlayerClassVector){
                                for(auto const& _ : group.vec){
                                        V(_.prob, _.cv);
                                }
                        }
                }
        private:
                Graph G;
                GNode* root;
                GNode* p;
                GNode* pp;
                GNode* pf;
                GNode* f;
                GEdge* e_0_p;
                GEdge* e_0_f;
                GEdge* e_1_p;
                GEdge* e_1_f;
                PushFoldState state0;
                std::string sdesc_;
        };






        struct GameTreeTwoPlayerRaiseFold : GameTree{
                GameTreeTwoPlayerRaiseFold(double sb, double bb, double eff, double raise)
                        : raise_{raise}
                {
                        // <0>
                        root = G.Node("*");
                                // <1>
                                r = G.Node("r");
                                        rp = G.Node("rp");
                                                // <2>
                                                rpp = G.Node("rpp");
                                                rpf = G.Node("rpf");
                                        rf = G.Node("rf");
                                f = G.Node("f");

                        // decision <0>
                        e_0_r = G.Edge(root, r);
                        e_0_f = G.Edge(root, f);

                        // decision <1>
                        e_1_p = G.Edge(r, rp);
                        e_1_f = G.Edge(r, rf);

                        // decision <2>
                        e_2_p = G.Edge(rp, rpp);
                        e_2_f = G.Edge(rp, rpf);
                
                        state0.Active.insert(0);
                        state0.Active.insert(1);
                        state0.Pot.resize(2);
                        state0.Pot[0] = sb;
                        state0.Pot[1] = bb;
                        state0.Stacks.resize(2);
                        state0.Stacks[0] = eff - sb;
                        state0.Stacks[1] = eff - bb;

                        std::stringstream sstr;
                        sstr << "GameTreeTwoPlayerRaiseFold:" << sb << ":" << bb << ":" << eff;
                        sdesc_ = sstr.str();
                        
                        BuildStrategy();

                }
                virtual std::string StringDescription()const override{ return sdesc_; }
                virtual void ColorPlayers(GraphColouring<size_t>& P)const override{
                        P[root] = 0;
                        P[r]    = 1;
                        P[rp]   = 0;
                }
                virtual void ColorOperators(GraphColouring<PushFoldOperator>& ops)const override{
                        ops[e_0_r] = Raise{0, raise_};
                        ops[e_0_f] = Fold{0};

                        ops[e_1_p] = Push{1};
                        ops[e_1_f] = Fold{1};

                        ops[e_2_p] = Push{0};
                        ops[e_2_f] = Fold{0};
                }
                virtual void ColorPrettyActions(GraphColouring<std::string>& A)const override{
                        A[root] = "sb raise/fold";
                        A[r]    = "bb call/fold, given bb raise";
                        A[rp]   = "sb call/fold, given sb raise, bb push";
                }
                virtual PushFoldState InitialState()const override{
                        return state0;
                }
                virtual GNode* Root()override{ return root; }
                virtual size_t NumPlayers()const override{ return 2; }
                
                virtual void VisitProbabilityDist(std::function<void(double, holdem_class_vector const& cv)> const& V)const noexcept override{
                        for(auto const& group : *Memory_TwoPlayerClassVector){
                                for(auto const& _ : group.vec){
                                        V(_.prob, _.cv);
                                }
                        }
                }
        private:
                double raise_;
                Graph G;
                GNode* root;
                GNode* r;
                GNode* rp;
                GNode* rf;
                GNode* rpp;
                GNode* rpf;
                GNode* f;
                GEdge* e_0_r;
                GEdge* e_0_f;
                GEdge* e_1_p;
                GEdge* e_1_f;
                GEdge* e_2_p;
                GEdge* e_2_f;
                PushFoldState state0;
                std::string sdesc_;
        };




        
        
        struct GameTreeThreePlayer : GameTree{
                GameTreeThreePlayer(double sb, double bb, double eff){
                        // <0>
                        root = G.Node("*");
                                // <1>
                                p = G.Node("p");
                                        // <3>
                                        pp = G.Node("pp");
                                                ppp = G.Node("ppp");
                                                ppf = G.Node("ppf");
                                        // <4>
                                        pf = G.Node("pf");
                                                pfp = G.Node("pfp");
                                                pff = G.Node("pff");
                                // <2>
                                f = G.Node("f");
                                        // <5>
                                        fp = G.Node("fp");
                                                fpp = G.Node("fpp");
                                                fpf = G.Node("fpf");
                                        ff = G.Node("ff");

                        // decision <0>
                        e_0_p = G.Edge(root, p);
                        e_0_f = G.Edge(root, f);

                        // decision <1>
                        e_1_p = G.Edge(p, pp);
                        e_1_f = G.Edge(p, pf);
                        
                        // decision <2>
                        e_2_p = G.Edge(f, fp);
                        e_2_f = G.Edge(f, ff);

                        // decision <3>
                        e_3_p = G.Edge(pp, ppp);
                        e_3_f = G.Edge(pp, ppf);

                        // decision <4>
                        e_4_p = G.Edge(pf, pfp);
                        e_4_f = G.Edge(pf, pff);
                        
                        // decision <5>
                        e_5_p = G.Edge(fp, fpp);
                        e_5_f = G.Edge(fp, fpf);

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

                        std::stringstream sstr;
                        sstr << "GameTreeThreePlayer:" << sb << ":" << bb << ":" << eff;
                        sdesc_ = sstr.str();
                        
                        BuildStrategy();
                }
                virtual std::string StringDescription()const override{ return sdesc_; }
                virtual void ColorPlayers(GraphColouring<size_t>& P)const override{
                        P[root] = 0;
                        P[p]    = 1;
                        P[f]    = 1;
                        P[pp]   = 2;
                        P[pf]   = 2;
                        P[fp]   = 2;
                }
                virtual void ColorOperators(GraphColouring<PushFoldOperator>& ops)const override{
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
                }
                virtual void ColorPrettyActions(GraphColouring<std::string>& A)const override{
                        A[root] = "btn push/fold";
                        A[p]    = "sb call/fold, given btn push";
                        A[f]    = "sb push/fold, given btn fold";
                        A[pp]   = "bb call/fold, given btn push, sb call";
                        A[pf]   = "bb call/fold, given btn push, sb fold";
                        A[fp]   = "bb call/fold, given btn fold, sb push";
                }
                virtual PushFoldState InitialState()const override{
                        return state0;
                }
                virtual GNode* Root()override{ return root; }
                virtual size_t NumPlayers()const override{ return 3; }
                
                virtual void VisitProbabilityDist(std::function<void(double, holdem_class_vector const& cv)> const& V)const noexcept override{
                        for(auto const& group : *Memory_ThreePlayerClassVector){
                                for(auto const& _ : group.vec){
                                        V(_.prob, _.cv);
                                }
                        }
                }
        private:
                Graph G;
                PushFoldState state0;
                GNode* root;
                GNode* p;
                GNode* pp;
                GNode* ppp;
                GNode* ppf;
                GNode* pf;
                GNode* pfp;
                GNode* pff;
                GNode* f;
                GNode* fp;
                GNode* fpp;
                GNode* fpf;
                GNode* ff;
                GEdge* e_0_p;
                GEdge* e_0_f;
                GEdge* e_1_p;
                GEdge* e_1_f;
                GEdge* e_2_p;
                GEdge* e_2_f;
                GEdge* e_3_p;
                GEdge* e_3_f;
                GEdge* e_4_p;
                GEdge* e_4_f;
                GEdge* e_5_p;
                GEdge* e_5_f;
                std::string sdesc_;
        };

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

        StateType CounterStrategy(std::shared_ptr<GameTree> gt, GraphColouring<AggregateComputer> const& AG, StateType const& S,
                                  double delta){
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

        boost::optional<StateType> Solve(holdem_binary_strategy_ledger_s& ledger, std::shared_ptr<GameTree> gt)
        {
                std::cout << "Solve\n"; // __CandyPrint__(cxx-print-scalar,Solve)



                auto root   = gt->Root();
                auto N      = gt->NumPlayers();
                auto state0 = gt->InitialState();



                
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
                GraphColouring<PushFoldOperator> ops;
                gt->ColorOperators(ops);

                for(auto t : root->TerminalNodes() ){

                        auto path = t->EdgePath();
                        auto ptr = std::make_shared<Computer>(path_to_string(path));
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
                IndexMaker im(*gt);
                        
                MakerConcept::Display(maker_dev);

                auto nv = [&](double prob, holdem_class_vector const& cv){
                        for(auto& m : maker_dev ){
                                m->Emit(&im, &C, prob, cv );
                        }
                };
                gt->VisitProbabilityDist(nv);



                decltype(gt->MakeDefaultState()) S0;
                if( ledger.size()){
                        S0 = ledger.back().to_eigen_vv();
                } else {
                        S0 = gt->MakeDefaultState();
                }

                auto S = S0;

                std::function<void(std::vector<std::vector<Eigen::VectorXd> > const&, Eigen::VectorXd const&, Eigen::VectorXd const&, double norm)> obs;
                struct Printer{
                        Printer(size_t n):n_{n}{
                                std::vector<std::string> title;
                                title.push_back("n");
                                for(size_t idx=0;idx!=n_;++idx){
                                        std::stringstream sstr;
                                        sstr << "EV[" << idx << "]";
                                        title.push_back(sstr.str());
                                }
                                for(size_t idx=0;idx!=n_;++idx){
                                        std::stringstream sstr;
                                        sstr << "CEV[" << idx << "]";
                                        title.push_back(sstr.str());
                                }
                                title.push_back("|.|");
                                lines_.emplace_back(std::move(title));
                                lines_.emplace_back(Pretty::LineBreak);
                        }
                        void operator()(std::vector<std::vector<Eigen::VectorXd> > const& S,
                                        Eigen::VectorXd const& ev,
                                        Eigen::VectorXd const& ev_counter,
                                        double norm){
                                std::vector<std::string> l;
                                l.push_back(boost::lexical_cast<std::string>(count_++));
                                for(size_t idx=0;idx!=n_;++idx){
                                        l.push_back(boost::lexical_cast<std::string>(ev[idx]));
                                }
                                for(size_t idx=0;idx!=n_;++idx){
                                        l.push_back(boost::lexical_cast<std::string>(ev_counter[idx]));
                                }
                                l.push_back(boost::lexical_cast<std::string>(norm));
                                size_t zero_or_one = 0;
                                size_t total = 0;
                                for(size_t idx=0;idx!=169;++idx){
                                        double epsilon = 1e-5;
                                        for(size_t j=0;j!=S.size();++j, ++total){
                                                if( S[j][0][idx] < epsilon || 1 - S[j][0][idx] < epsilon ){
                                                        ++zero_or_one;
                                                }
                                        }
                                }
                                l.push_back(boost::lexical_cast<std::string>(zero_or_one));
                                l.push_back(boost::lexical_cast<std::string>(total));
                                l.push_back(boost::lexical_cast<std::string>(S[0][0].lpNorm<Eigen::Infinity>()));

                                lines_.emplace_back(std::move(l));
                        }
                        void Display()const{

                                Pretty::RenderOptions opts;
                                opts.SetAdjustment(1, Pretty::RenderAdjustment_Left);
                                opts.SetAdjustment(2, Pretty::RenderAdjustment_Left);
                                opts.SetAdjustment(3, Pretty::RenderAdjustment_Left);

                                Pretty::RenderTablePretty(std::cout, lines_, opts);
                        }
                private:
                        size_t n_;
                        size_t count_{0};
                        std::vector<Pretty::LineItem> lines_;
                };
                Printer printer{N};

                enum{ MaxIter = 1000 };

                double delta = 0.01 / 8;

                for(size_t n=1;n!=MaxIter;++n){
                        boost::timer::auto_cpu_timer at;


                        Eigen::VectorXd ev(N);
                        ev.fill(0);
                        AG[root].Evaluate(ev, S);

                        auto S_counter = CounterStrategy(gt, AG, S, delta);

                        Eigen::VectorXd ev_counter(N);
                        ev_counter.fill(0);
                        AG[root].Evaluate(ev_counter, S_counter);

                        double factor = 0.05;
                        for(size_t i=0;i!=gt->NumDecisions();++i){
                                for(size_t j=0;j!=gt->Depth(i);++j){
                                        S[i][j] = S[i][j] * ( 1.0 - factor ) + factor * S_counter[i][j];
                                }
                        }
                        #if 1
                        // now we clamp, the idea is that the onyl way to get to
                        //    clamp_epsilon is if we have had had a continus iterations of
                        //  in one dir
                        double clamp_epsilon = 0.0001;
                        for(size_t i=0;i!=gt->NumDecisions();++i){
                                for(size_t j=0;j!=gt->Depth(i);++j){
                                        for(size_t cid=0;cid!=169;++cid){
                                                if( std::fabs( S[i][j][cid] ) < clamp_epsilon ){
                                                        S[i][j][cid] = 0.0;
                                                }
                                                if( std::fabs( 1.0 - S[i][j][cid] ) < clamp_epsilon ){
                                                        S[i][j][cid] = 1.0;
                                                }
                                        }
                                }
                        }
                        #endif
                        double norm = ( ev - ev_counter ).lpNorm<Eigen::Infinity>();

                        #if 0
                        for(size_t i=0;i!=gt->NumDecisions();++i){
                                for(size_t j=0;j!=gt->Depth(i);++j){
                                        S[i][j] = S[i][j] * ( 1.0 - 1.0/n ) + 1.0 / n * S_counter[i][j];
                                }
                        }
                        #endif

                        ledger.push(S);
                        if( n != 2 )
                                ledger.save_();

                        //factor = norm;
                        //
                        printer(S, ev, ev - ev_counter, norm);

                        //if( norm < 0.000001 ){
                        if( S == S_counter ){
                                #if 0
                                std::ofstream of("eval.csv");
                                std::ofstream sof("S.csv");
                                evaluate_saver es(of);
                                AG[root].Observe(S, es);
                                strategy_saver ss(sof);
                                ss(S);
                                #endif
                                printer.Display();
                                auto p0 = EvDetail(AG[root], S, 0);
                                auto p1 = EvDetail(AG[root], S, 1);
                                pretty_print_strat(p0, 3);
                                pretty_print_strat(p1, 3);
                                std::cout << "------------ effect of forcing ========\n";
                                auto counter_nf = CounterStrategy(gt, AG, S, 0.0);
                                pretty_print_strat(S[0][0] - counter_nf[0][0], 2);
                                pretty_print_strat(S[1][0] - counter_nf[1][0], 2);
                                return S;
                        }

                        #if 1
                        std::cout << "norm => " << norm << "\n"; // __CandyPrint__(cxx-print-scalar,norm)
                        for(size_t idx=0;idx!=S.size();++idx){
                                pretty_print_strat(S[idx][0], 10);
                        }
                        #endif

                }
                printer.Display();
                return boost::none;
        }

        struct Driver{
                // enable this for debugging
                enum{ NoPersistent = true };
                Driver(){
                        if( ! NoPersistent ){
                                ss_.try_load_or_default(".ss.bin");
                        }
                }
                ~Driver(){
                        // will do nothing when no save
                        ss_.save_();
                }
                boost::optional<std::vector<std::vector<Eigen::VectorXd> >> FindOrBuildSolution(std::shared_ptr<GameTree> gt){
                        auto key = gt->StringDescription();

                        if( ! NoPersistent ){
                                auto iter = ss_.find(key);
                                if( iter != ss_.end()){
                                        return iter->second.to_eigen_vv();
                                }
                        }

                        auto ledger_key = ".ledger/" + key;
                        holdem_binary_strategy_ledger_s ledger;
                        if( ! NoPersistent ){
                                ledger.try_load_or_default(ledger_key);
                        }

                        auto sol = Solve(ledger, gt);

                        if( sol ){
                                ss_.add_solution(key, *sol);
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
                        enum { Dp = 2 };
                        size_t n = 2;
                        double sb = 0.5;
                        double bb = 1.0;
                        #if 1
                        Driver dvr;
                        dvr.Display();
                        std::vector<Eigen::VectorXd> S;


                        std::shared_ptr<GameTree> any_gt;


                        for(double eff = 10.0;eff - 1e-4 < 10.0; eff += 1.0 ){
                        //for(double eff = 11.0;eff - 1e-4 < 11.0; eff += 1.0 ){
                                std::cout << "eff => " << eff << "\n"; // __CandyPrint__(cxx-print-scalar,eff)

                                std::shared_ptr<GameTree> gt;
                                #if 1
                                if( n == 2 ){
                                        gt = std::make_shared<GameTreeTwoPlayer>(sb, bb, eff);
                                } else {
                                        gt = std::make_shared<GameTreeThreePlayer>(sb, bb, eff);
                                }
                                #else
                                gt = std::make_shared<GameTreeTwoPlayerRaiseFold>(sb, bb, eff, 2.0);
                                #endif
                                any_gt = gt;
                                auto opt = dvr.FindOrBuildSolution(gt);
                                auto root   = gt->Root();
                                if( opt ){
                                        pretty_print_strat(opt.get()[0][0], 1);
                                        pretty_print_strat(opt.get()[1][0], 1);
                                        for(; S.size() < opt->size();){
                                                S.emplace_back();
                                                S.back().resize(169);
                                                S.back().fill(0.0);
                                        }
                                        for(size_t j=0;j!=opt->size();++j){
                                                for(size_t idx=0;idx!=169;++idx){
                                                        if( std::fabs(opt.get()[j][0][idx] - 1.0 ) < 1e-2 ){
                                                                S[j][idx] = eff;
                                                        }
                                                }
                                        }
                                }
                        }
                        for( auto const& _ : *any_gt){
                                std::cout << "\n            " << _.PrettyAction() << "\n\n";
                                pretty_print_strat(S[_.GetIndex()], Dp);

                        }

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
