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
#ifndef PS_SIM_COMPUTER_H
#define PS_SIM_COMPUTER_H

/*
        to do three player all in evaluation, I found that creating a 
        static vector was the most computationally tangibble. The alternative
        is to do a lookup each evaluation, but that is much slower it 
        it fits in memory.
                For the taxonomy, we have the below, which means that this
        code build on on pre-flop all in equity evaluator. Basically solving
        preflop all in situations is figuring out the the strategy performs
        on average against the other opponent


                                HandEval
                                   ^
                              PreFlopAllInEval
                                   ^
                         StrategyVsStrategyEval


 */

namespace ps{
namespace sim{
        
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
                Eigen::VectorXd ExpectedValue(StateType const& S)const noexcept {
                        Eigen::VectorXd V;
                        switch(S.size()){
                        case 2:
                        default:
                                // TODO
                                V.resize(2);
                                break;
                        case 6:
                                V.resize(3);
                                break;
                        }
                        V.fill(0);
                        EvaluateFiltered(V, S, [](auto&&){return true; });
                        return V;
                }

        };

} // end namespace sim
} // end namespace ps

#endif // PS_SIM_COMPUTER_H
