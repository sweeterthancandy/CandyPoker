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
        
        struct Index{
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
                Index IndexFor(GEdge* e)const{
                        auto begin = E.begin();
                        auto end = E.end();
                        auto iter = std::find(begin, end, e);
                        if( iter == end)
                                BOOST_THROW_EXCEPTION(std::domain_error("not an edge"));
                        return { ID_, static_cast<size_t>(std::distance(begin, iter)) };
                }

                size_t GetIndex()const{ return ID_; }
                size_t GetPlayer()const{ return player_idx_; }

        private:
                size_t ID_;
                size_t player_idx_;
                std::vector<GEdge*> E;
        };

                
        struct StrategyDecl{
                using decision_vector = std::vector<std::shared_ptr<Decision> >;
                using decision_iterator = boost::indirect_iterator<decision_vector::const_iterator>;
                decision_iterator begin()const{ return v_.begin(); }
                decision_iterator end()const{ return v_.end(); }
                void Add(std::shared_ptr<Decision> ptr){
                        v_.push_back(ptr);
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

        struct Computer{
                using StateType = std::vector<std::vector<Eigen::VectorXd> >;
                virtual ~Computer()=default;
                virtual void Evaluate(Eigen::VectorXd& R, StateType const& S)const noexcept=0;
        };

        struct AggregateComputer : Computer{
                struct Atom{
                        holdem_class_vector cv;
                        double constant{1.0};
                        std::vector<Index> index;
                        Eigen::VectorXd value;
                };
                virtual void Evaluate(Eigen::VectorXd& R, StateType const& S)const noexcept override{
                        for(auto const& _ : atoms_){
                                double c = _.constant;
                                for(auto const& idx : _.index){
                                        c *= S[idx.s][idx.choice][idx.id];
                                }
                                R += c * _.value;
                        }
                }
                void Emplace(holdem_class_vector const& cv, double constant, std::vector<Index> index, Eigen::VectorXd value){
                        atoms_.emplace_back(Atom{cv, constant, index, value});
                }
        private:
                std::vector<Atom> atoms_;
        };

        #if 0
        struct MakerConcept{
                virtual ~MakerConcept()=default;
                virtual void Emit(AggregateComputer* computer, class_cache* cache, double prob, holdem_class_vector const& cv)=0;
        };
        struct AtomMaker : MakerConcept{
                AtomMaker(StrategyDecl const& S, std::vector<Index> const& index_proto, Eigen::VectorXd pot, std::vector<size_t> const& active)
                        :S_{S}, index_proto_(index_proto),
                        pot_(pot),
                        active_(active)
                {}
                virtual void Emit(AggregateComputer* computer, class_cache* cache, double prob, holdem_class_vector const& cv){
                        auto index = index_proto_;
                        for(auto& _ : index){
                                _.id = cv[_.player];
                        }
                        computer->Emplace( prob, index, value_);
                }
        private:
                StrategyDecl const& S_;
                std::vector<Index> index_proto_;
                Eigen::VectorXd pot_;
                std::vector<size_t> active_;
        };
        #endif

        void Build(){

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

                #if 0
                auto S = std::make_shared<StrategyDecl>();
                S->Add(d_0);
                S->Add(d_1);
                #endif
                
                double sb = 0.5;
                double bb = 1.0;
                double eff = 10.0;

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


                std::array<std::shared_ptr<AggregateComputer>, 169 > hero;
                for(size_t idx=0;idx!=169;++idx){
                        hero[idx] = std::make_shared<AggregateComputer>();
                }

                auto comp = std::make_shared<AggregateComputer>();
                for(auto const& group : *Memory_TwoPlayerClassVector){
                        for(auto const& _ : group.vec){
                                auto const& cv = _.cv;
                

                                Eigen::VectorXd ev = C.LookupVector(cv);
                                
                                auto v_pp = 2 * eff * ev - eff_v;
                
                                std::vector<Index> p_pp{ {0,0, cv[0]}, {1,0, cv[1]} };
                                std::vector<Index> p_pf{ {0,0, cv[0]}, {1,1, cv[1]} };
                                std::vector<Index> p_f{  {0,1, cv[0]} };

                                comp->Emplace(cv, _.prob, p_pp, v_pp);
                                comp->Emplace(cv, _.prob, p_pf, v_pf);
                                comp->Emplace(cv, _.prob, p_f , v_f );
                                
                                hero[cv[0]]->Emplace(cv, _.prob, p_pp, v_pp);
                                hero[cv[0]]->Emplace(cv, _.prob, p_pf, v_pf);
                                hero[cv[0]]->Emplace(cv, _.prob, p_f , v_f );


                        }
                }

                Eigen::VectorXd proto(169);
                proto.fill(0.5);
                std::vector<Eigen::VectorXd> sx(2, proto);
                std::vector<std::vector<Eigen::VectorXd> > S0(2,sx);

                auto S = S0;
                for(size_t n=0;n!=2;++n){
                        auto counter = S;
                        Eigen::VectorXd p(2);
                        Eigen::VectorXd f(2);

                        auto Sp = S;
                        Sp[0][0].fill(1);
                        Sp[0][1].fill(0);

                        auto Sf = S;
                        Sf[0][0].fill(0);
                        Sf[0][1].fill(1);

                        for(size_t idx=0;idx!=169;++idx){
                                p.fill(0);
                                f.fill(0);
                                hero[idx]->Evaluate(p, Sp);
                                hero[idx]->Evaluate(f, Sf);
                                double x = ( p[0] >= f[0] ? 1.0 : 0.0 );
                                counter[0][0][idx] = x;
                                counter[0][1][idx] = 1.0 - x;
                        }

                        double norm = 0.0;
                        norm += (counter[0][0] - S[0][0]).lpNorm<2>();

                        Eigen::VectorXd R(2);
                        R.fill(0);
                        comp->Evaluate(R, counter);
                        S = counter;
                        std::cout << "vector_to_string(R) => " << vector_to_string(R) << "\n"; // __CandyPrint__(cxx-print-scalar,vector_to_string(ev))
                        std::cout << "norm => " << norm << "\n"; // __CandyPrint__(cxx-print-scalar,norm)
                        
                        pretty_print_strat(S[0][0], 1);

                }


        }
        
        struct ScratchCmd : Command{
                enum{ Debug = 1};
                explicit
                ScratchCmd(std::vector<std::string> const& args):args_{args}{}
                virtual int Execute()override{
                        Build();
                        return EXIT_SUCCESS;
                }
        private:
                std::vector<std::string> const& args_;
        };
        static TrivialCommandDecl<ScratchCmd> ScratchCmdDecl{"scratch"};
} // end namespace ps
