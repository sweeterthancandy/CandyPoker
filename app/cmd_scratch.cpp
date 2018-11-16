#include "ps/base/cards.h"
#include "ps/support/command.h"
#include "ps/detail/print.h"
#include "ps/eval/class_cache.h"
#include "ps/eval/holdem_class_vector_cache.h"
#include "app/pretty_printer.h"

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
                friend std::ostream& operator<<(std::ostream& ostr,         Decision const& self){
                        ostr << "ID_ = " << self.ID_;
                        ostr << ", player_idx_ = " << self.player_idx_;
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
        private:
                std::vector<std::shared_ptr<Decision> > v_;
        };


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
                virtual void Emit(Computer* vc, IndexMakerConcept* im, class_cache const* cache, double p, holdem_class_vector const& cv)const=0;
                virtual std::string to_string()const=0;
        };
        struct StaticEmit : MakerConcept{
                StaticEmit(std::vector<GEdge const*> path, std::vector<size_t> const& active, Eigen::VectorXd pot)
                        :path_(path),
                        active_(active),
                        pot_(pot)
                {}
                virtual void Emit(Computer* vc, IndexMakerConcept* im, class_cache const* cache, double p, holdem_class_vector const& cv)const override{
                        
                        auto index = im->MakeIndex(path_, cv); 
                        
                        Eigen::VectorXd v(pot_.size());
                        v.fill(0);
                        v -= pot_;

                        if( active_.size() == 1 ){
                                v[active_[0]] += pot_.sum();

                                //vc->Emplace(cv, p, im
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

                        vc->Emplace(cv, p, index, v);
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

        boost::optional<StateType> Solve(double sb, double bb, double eff){

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

                auto d_0 = std::make_shared<Decision>(0, 0);
                d_0->Add(e_0_p);
                d_0->Add(e_0_f);
                auto d_1 = std::make_shared<Decision>(1, 1);
                d_1->Add(e_1_p);
                d_1->Add(e_1_f);

                auto strat = std::make_shared<StrategyDecl>();
                strat->Add(d_0);
                strat->Add(d_1);
                


                std::string cache_name{".cc.bin"};
                class_cache C;
                C.load(cache_name);

                auto tpl_pp = pp->EdgePath();
                auto tpl_pf = pf->EdgePath();
                auto tpl_f  = f ->EdgePath();


                Eigen::VectorXd v_blinds(2);
                v_blinds[0] += sb;
                v_blinds[1] += bb;

                auto vv_f = v_blinds;
                auto vv_p = v_blinds;
                vv_p[0] = eff;
                auto vv_pf = vv_p;
                auto vv_pp = vv_p;
                vv_pp[1] = eff;

                PushFoldState state0;
                state0.Active.insert(0);
                state0.Active.insert(1);
                state0.Pot.resize(2);
                state0.Pot[0] = sb;
                state0.Pot[1] = bb;
                state0.Stacks.resize(2);
                state0.Stacks[0] = eff - sb;
                state0.Stacks[1] = eff - bb;

                GraphColouring<PushFoldOperator> ops;
                ops[e_0_p] = Push{0};
                ops[e_0_f] = Fold{0};
                ops[e_1_p] = Push{1};
                ops[e_1_f] = Fold{1};
                
                std::vector<std::shared_ptr<MakerConcept> > maker_dev;

                auto T = root->TerminalNodes();
                GraphColouring<std::shared_ptr<Computer> > TC;
                GraphColouring<AggregateComputer> AG;
                std::vector<Computer*> tc_aux;
                for(auto t : T ){
                        auto state = state0;
                        auto path = t->EdgePath();
                        for( auto e : path ){
                                ops[e](state);
                        }
                        state.Display();
                        std::vector<size_t> perm(state.Active.begin(), state.Active.end());
                        maker_dev.push_back(std::make_shared<StaticEmit>(path, perm, state.Pot) );
                        
                        auto ptr = std::make_shared<Computer>();
                        TC[t] = ptr;
                        tc_aux.push_back(ptr.get());

                        for(auto e : path ){
                                AG[e->From()].push_back(ptr); 
                        }
                }




                IndexMaker im(*strat);
                

                for(auto const& group : *Memory_TwoPlayerClassVector){
                        for(auto const& _ : group.vec){
                                for(size_t idx=0;idx!=maker_dev.size();++idx){
                                        maker_dev[idx]->Emit(tc_aux[idx], &im, &C, _.prob, _.cv );
                                }
                        }
                }




                auto S0 = strat->MakeDefaultState();

                auto S = S0;
                for(size_t n=0;n!=200;++n){
                        auto counter = S;

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
                                        counter[sidx][0][idx] = x;
                                        counter[sidx][1][idx] = 1.0 - x;
                                }
                        }


                        double factor = 0.05;
                        for(size_t i=0;i!=strat->NumDecisions();++i){
                                for(size_t j=0;j!=strat->Depth(i);++j){
                                        S[i][j] = S[i][j] * ( 1.0 - factor ) + factor * counter[i][j];
                                }
                        }
                        

                        if( n % 10 ){
                                Eigen::VectorXd R_counter(2);
                                R_counter.fill(0);
                                AG[root].Evaluate(R_counter, counter);
                                
                                Eigen::VectorXd R(2);
                                R.fill(0);
                                AG[root].Evaluate(R, S);

                                double norm = ( R - R_counter ).lpNorm<2>();
                                
                                if( norm < 0.0001 ){
                                        return counter;
                                }
                                
                                std::cout << "vector_to_string(R) => " << vector_to_string(R) << "\n"; // __CandyPrint__(cxx-print-scalar,vector_to_string(ev))
                                std::cout << "norm => " << norm << "\n"; // __CandyPrint__(cxx-print-scalar,norm)
                                pretty_print_strat(S[0][0], 1);
                                pretty_print_strat(S[1][0], 1);
                        }

                }
                return boost::none;


        }
        
        struct ScratchCmd : Command{
                enum{ Debug = 1};
                explicit
                ScratchCmd(std::vector<std::string> const& args):args_{args}{}
                virtual int Execute()override{
                        double sb = 0.5;
                        double bb = 1.0;
                        #if 0
                        struct Pair{
                                double eff;
                                StateType S;
                        };
                        std::vector<Pair> meta;
                        for(double eff = 10.0;eff != 20.0; eff += 1.0 ){
                                auto opt = Solve(sb, bb, eff);
                                if( opt ){
                                        meta.push_back(Pair{eff, std::move(*opt)});
                                }
                        }
                        #endif
                        auto opt = Solve(sb, bb, 10.0);
                        if( opt ){
                                pretty_print_strat(opt.get()[0][0], 0);
                                pretty_print_strat(opt.get()[1][0], 0);
                        }
                        return EXIT_SUCCESS;
                }
        private:
                std::vector<std::string> const& args_;
        };
        static TrivialCommandDecl<ScratchCmd> ScratchCmdDecl{"scratch"};
} // end namespace ps
