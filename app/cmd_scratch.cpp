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


                std::vector<GEdge*> EdgePath()const{
                        std::vector<GEdge*> rpath;
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
                        return std::vector<GEdge*>(rpath.rbegin(), rpath.rend());
                }

                std::string const& Name()const{ return name_; }

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

        #if 0
        struct EvalDecl{

                EvalDecl(std::shared_ptr<StrategyDecl> S)
                        :S_(S)
                {}
                void Add(GNode* node, std::shared_ptr<Eval> eval){
                        auto path = node->EdgePath();
                        
                        std::vector<Index> iv;
                        for(auto _ : path){
                                auto index = S_->FindDecisionOrThrow(_)->IndexFor(_);
                                iv.push_back(index);
                        }
                        evals_.emplace_back({std::move(iv), eval});
                }

        private:
                std::shared_ptr<StrategyDecl> S_;
                std::vector<EvalItem> items_;
        };
        #endif

        #if 0
        struct ComputationalProblem{
                struct ComputationalAtom{
                        double constant{1.0};
                        std::vector<Index> index;
                        Eigen::VectorXd value;
                };
                Eigen::VectorXd Compute(std::vector<std::vector<Eigen::VectorXd> >
        private:
                std::vector<ComputationalAtom> v_;
        };
        #endif


        struct AggregateComputer{

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


        boost::optional<StateType> Solve(double sb, double bb, double eff){

                auto G = std::make_shared<Graph>();


                #if 0
                // <0>
                auto root = G->Node("*");
                        // <1>
                        auto r = G->Node("r");
                                // <3>
                                auto rp = G->Node("rp");
                                        auto rpp = G->Node("rpp");
                                        auto rpf = G->Node("rpf");
                                auto rf = G->Node("rf");
                        // <2>
                        auto p = G->Node("p");
                                auto pp = G->Node("pp");
                                auto pf = G->Node("pf");
                        auto f = G->Node("f");
                

                // decision <0>
                auto e_0_r = G->Edge(root, r);
                auto e_0_p = G->Edge(root, p);
                auto e_0_f = G->Edge(root, f);

                // decision <1>
                auto e_1_p = G->Edge(r, rp);
                auto e_1_f = G->Edge(r, rf);

                // decision <2>
                auto e_2_p = G->Edge(p, pp);
                auto e_2_f = G->Edge(p, pf);

                // decision <3>
                auto e_3_p = G->Edge(rp, rpp);
                auto e_3_f = G->Edge(rp, rpf);


                //  The probability each of these is S[i][j][cv[p]],
                //
                //  where \sigma_{j\in J} S[i][j][cv[p]] = 1.0
                //
                //                                    i  p
                auto d_0 = std::make_shared<Decision>(0, 0);
                d_0->Add(e_0_r);
                d_0->Add(e_0_p);
                d_0->Add(e_0_f);
                auto d_1 = std::make_shared<Decision>(1, 1);
                d_1->Add(e_1_p);
                d_1->Add(e_1_f);
                auto d_2 = std::make_shared<Decision>(2, 1);
                d_2->Add(e_2_p);
                d_2->Add(e_2_f);
                auto d_3 = std::make_shared<Decision>(3, 0);
                d_3->Add(e_3_p);
                d_3->Add(e_3_f);
                #endif


                #if 0
                std::unordered_map<void*, std::shared_ptr<Decision> > D;
                D[root] = d_0;
                D[r]    = d_1;
                D[p]    = d_2;
                D[rp]   = d_3;

                std::unordered_map<void*, Index> I;
                I[e_0_r] = d_0->IndexFor(e_0_r);
                I[e_1_r] = d_0->For(e_1_r);
                I[e_2_r] = d_0->For(e_2_r);
                #endif
                
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
                

                Eigen::VectorXd v_pf(2);
                v_pf[0] = +bb;
                v_pf[1] = -bb;

                Eigen::VectorXd v_f(2);
                v_f[0] = -sb;
                v_f[1] = +sb;
                        
                Eigen::VectorXd eff_v(2);
                eff_v.fill(eff);

                std::string cache_name{".cc.bin"};
                class_cache C;
                C.load(cache_name);

                GraphColouring<size_t> D;
                GraphColouring<size_t> P;
                GraphColouring<size_t> I;

                for(auto d : *strat){
                        for(auto e : d){
                                D[e] = d.GetIndex();
                                P[e] = d.GetPlayer();
                                I[e] = d.OffsetFor(e);
                        }
                }
                

                auto tpl_pp = pp->EdgePath();
                auto tpl_pf = pf->EdgePath();
                auto tpl_f  = f ->EdgePath();

                auto make_index = [&](auto const& tpl, auto const& cv){
                        std::vector<Index> index;
                        for(auto e : tpl){
                                index.push_back( Index{ D[e], I[e], cv[P[e]] } );
                        }
                        return index;
                };

                auto comp = std::make_shared<AggregateComputer>();
                for(auto const& group : *Memory_TwoPlayerClassVector){
                        for(auto const& _ : group.vec){
                                auto const& cv = _.cv;
                

                                Eigen::VectorXd ev = C.LookupVector(cv);
                                
                                auto v_pp = 2 * eff * ev - eff_v;
                
                                auto p_pp = make_index( tpl_pp, cv);
                                auto p_pf = make_index( tpl_pf, cv);
                                auto p_f  = make_index( tpl_f , cv);

                                comp->Emplace(cv, _.prob, p_pp, v_pp);
                                comp->Emplace(cv, _.prob, p_pf, v_pf);
                                comp->Emplace(cv, _.prob, p_f , v_f );

                        }
                }

                #if 0
                Eigen::VectorXd proto(169);
                proto.fill(0.5);
                std::vector<Eigen::VectorXd> sx(2, proto);
                std::vector<std::vector<Eigen::VectorXd> > S0(2,sx);
                #endif
                auto S0 = strat->MakeDefaultState();

                auto S = S0;
                for(size_t n=0;n!=200;++n){
                        auto counter = S;

                        for(auto const& decision : *strat){

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


                                comp->Observe(Sp, po);
                                comp->Observe(Sf, fo);
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
                                comp->Evaluate(R_counter, counter);
                                
                                Eigen::VectorXd R(2);
                                R.fill(0);
                                comp->Evaluate(R, S);

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
                        return EXIT_SUCCESS;
                }
        private:
                std::vector<std::string> const& args_;
        };
        static TrivialCommandDecl<ScratchCmd> ScratchCmdDecl{"scratch"};
} // end namespace ps
