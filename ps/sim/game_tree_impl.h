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
#ifndef PS_SIM_GAME_TREE_IMPL_H
#define PS_SIM_GAME_TREE_IMPL_H


namespace ps{
namespace sim{

        struct GameTreeTwoPlayer : GameTree{
                GameTreeTwoPlayer(double sb, double bb, double eff)
                        :sb_(sb),
                        bb_(bb),
                        eff_(eff)
                {
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
                
                virtual double SmallBlind()const override{ return sb_; }
                virtual double BigBlind()const override{ return bb_; }
                virtual double EffectiveStack()const override{ return eff_; }
        private:
                double sb_;
                double bb_;
                double eff_;
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
                        :sb_(sb),
                        bb_(bb),
                        eff_(eff),
                        raise_{raise}
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
                virtual double SmallBlind()const override{ return sb_; }
                virtual double BigBlind()const override{ return bb_; }
                virtual double EffectiveStack()const override{ return eff_; }
        private:
                double sb_;
                double bb_;
                double eff_;
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
                GameTreeThreePlayer(double sb, double bb, double eff)
                        :sb_(sb),
                        bb_(bb),
                        eff_(eff)
                {
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
                virtual double SmallBlind()const override{ return sb_; }
                virtual double BigBlind()const override{ return bb_; }
                virtual double EffectiveStack()const override{ return eff_; }
        private:
                double sb_;
                double bb_;
                double eff_;
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

} // end namespace sim
} // end namespace ps

#endif // PS_SIM_GAME_TREE_IMPL_H
