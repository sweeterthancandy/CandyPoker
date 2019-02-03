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
#ifndef PS_DETAIL_GRAPH_H
#define PS_DETAIL_GRAPH_H


namespace ps{
namespace detail{

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

        inline
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

} // end namespace detail
} // end namespace ps

#endif // PS_DETAIL_GRAPH_H
