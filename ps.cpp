#include "ps/holdem/simulation.h"
#include "ps/core/cards.h"
#include "ps/holdem/frontend.h"

        /*
        
        The idea here is what when computing range
        vs range evaluation, there are a number of
        results which are linearly related, because
        the way the suits are distribution evently
        when can essentially creat a bijective
        mapping (or permutation)
                f: {h,d,s,c} -> {h',d',s',c'},
        such such that there are dupliucate 
        computations. 
                Consider the computeration

                        AKs vs 98s
                =>
                        AhKh  9h8h 
                        AcKc  9c8c
                        AsKs  9s8s
                        AdKd  9d8d

                =>
                        AhKh   9h8h
                        AhKh   9c8c
                        AhKh   9s8s
                        AhKh   9d8d
                        AcKc   9h8h
                        AcKc   9c8c
                        AcKc   9s8s
                        AcKc   9d8d
                        AsKs   9h8h
                        AsKs   9c8c
                        AsKs   9s8s
                        AsKs   9d8d
                        AdKd   9h8h
                        AdKd   9c8c
                        AdKd   9s8s
                        AdKd   9d8d

        For there 16 computations there exists a
        bijective mapping that the above results in
                
                +--+------------+
                |N |Computation |
                +--+------------+
                |12|AhKh vs 9c8c|
                |4 |AhKh vs 9h8h|
                +--+------------+
                   figure 1

                Another optimization to consider is
        the computation set
               
                  +----------------------+-------------+
                  |     Computation      | Permutation |
                  +----------------------+-------------+
                  | AhKh vs TcTh vs 2s2c |    0 1 2    |
                  | AhKh vs 2s2c vs TcTh |    0 2 1    |
                  | TcTh vs AhKh vs 2s2c |    1 0 2    |
                  | 2s2c vs AhKh vs TcTh |    1 2 0    |
                  | TcTh vs 2s2c vs AhKh |    2 0 1    |
                  | 2s2c vs TcTh vs AhKh |    2 1 0    |
                  +----------------------+-------------+
                            figure 2

                This again can be represented by creating an
        auxillary computation set, and then the result will
        be a linear combintation of this.
                To compute fig 2, we have the below
                
                Let Comp = AhKh vs TcTh vs 2s2c 

                        |S_AhKh|   | w_0 d_0 s_0 e_0 |
                Let S = |S_TcTh| = | w_1 d_1 s_1 e_1 |
                        |S_2s2C|   | w_2 d_2 s_2 e_2 |

                where
                    = Eval(AhKh vs TcTh vs 2s2c)
                      Eval(Comp) 
                    = S

                This implies that
                                                     | 1 0 0 |
                        S_{AhKh vs 2s2c vs TcTh} = S | 0 0 1 |
                                                     | 0 1 0 |

                And thus a computeration set can be reduced
        to a linear combintations of permutation matrixis.
                        
                
                Another optimization that will be applied is
        cached values. For this, consider the computation
                        AA vs TT,
        if I allow this to be represented in the computation
        symbolically, rather than expand to suits, and using
        the fact that
                        log(52^4) + 15.804975 + \epsilon,
        i can precalculate all the heads up probilities
        for heads up calculations

                                                     



        there will be lots of redundant computtaion 


                For this I need to abstract the way computation
        results are stored, to allow the freedom of how the 
        result is calculated

         */

namespace ps{

        namespace detail{
                struct print_context{
                        std::ostream* stream;
                        std::vector<size_t> indent;

                        void push(){
                                indent.push_back( indent.back()+1);
                        }
                        void pop(){
                                indent.pop_back();
                        }

                        std::ostream& put(){ return *stream << std::string(indent.back()*2, ' '); }
                };
        }

        struct symbolic_computation{
                using handle = std::shared_ptr<symbolic_computation>;

                virtual ~symbolic_computation()=default;

                void print(std::ostream& ostr = std::cout)const{
                        detail::print_context ctx;
                        ctx.indent.emplace_back(0);
                        ctx.stream = &ostr;
                        print_impl(ctx);
                }
                virtual void print_impl(detail::print_context& ctx)const=0;
        };
        
        struct symbolic_primitive : symbolic_computation{
                symbolic_primitive(std::vector<frontend::hand> const& hands):hands_{hands}{}
                
                void print_impl(detail::print_context& ctx)const override{
                        std::stringstream sstr;
                        for(size_t i{0};i!=hands_.size();++i){
                                if( i != 0 ) sstr << " vs ";
                                sstr << hands_[i];
                        }
                        ctx.put() << sstr.str() << "\n";
                }
        private:
                std::vector<frontend::hand> hands_;
        };

        struct symbolic_primitive_range : symbolic_computation{
                symbolic_primitive_range(std::vector<frontend::primitive_t> const& prims)
                        :prims_{prims}
                {
                        std::vector<size_t> size_vec;
                        std::vector<std::vector<holdem_id> > aux;

                        for( auto const& p : prims_){
                                aux.emplace_back( to_hand_vector(p));
                                size_vec.push_back( aux.back().size()-1);
                        }
                        
                        switch(prims_.size()){
                        case 2:
                                detail::visit_exclusive_combinations<2>(
                                        [&](auto a, auto b){
                                        children_.emplace_back(
                                               std::make_shared<symbolic_primitive>( std::vector<frontend::hand>{frontend::hand{aux[0][a]},
                                                                                                                {frontend::hand{aux[1][b]}}}));
                                }, detail::true_, size_vec);
                                break;
                        default:
                                assert( 0 && " not implemented");
                        }


                }
                void print_impl(detail::print_context& ctx)const override{
                        ctx.put() << "children.size() = " << children_.size() << "\n";
                        for(size_t i{0};i!=prims_.size();++i){
                                ctx.put() << "prim " << i << " : " << prims_[i] << "\n";
                        }
                        ctx.push();
                        for( auto const& c : children_ ){
                                c->print_impl(ctx);
                        }
                        ctx.pop();
                }
        private:
                std::vector<frontend::primitive_t> prims_;
                std::vector<handle> children_;
        };

        

        struct symbolic_range : symbolic_computation{
                symbolic_range(std::vector<frontend::range> const& players):
                        players_{players}
                {
                        std::vector<size_t> size_vec;
                        std::vector<frontend::primitive_range> prims;
                        for(auto const& rng : players_){
                                prims.emplace_back( expand(rng).to_primitive_range());
                                assert( prims.back().size() != 0 && "precondition failed");
                                size_vec.emplace_back(prims.back().size()-1);
                        }

                        switch(players.size()){
                        case 2:
                                detail::visit_exclusive_combinations<2>(
                                        [&](auto a, auto b){
                                        children_.emplace_back(
                                               std::make_shared<symbolic_primitive_range>( std::vector<frontend::primitive_t>{prims[0][a], prims[1][b] }));
                                }, detail::true_, size_vec);
                                break;
                        default:
                                assert( 0 && " not implemented");
                        }
                }
                void print_impl(detail::print_context& ctx)const override{
                        ctx.put() << "children.size() = " << children_.size() << "\n";
                        for(size_t i{0};i!=players_.size();++i){
                                ctx.put() << "player " << i << " : " << players_[i] << "\n";
                        }
                        ctx.push();
                        for( auto const& c : children_ ){
                                c->print_impl(ctx);
                        }
                        ctx.pop();
                }
        private:
                std::vector<frontend::range> players_;

                std::vector<handle> children_;
        };



        #if 0
        struct computation_result{
        };
        
        struct computation_result_view{
        };

        struct computation_item{
                virtual ~computation_item()=default;
        };

        struct primitive_computation{
                explicit primitive_computation(std::vector<primitive_t> const& prim){
                }
        };

        struct computation{
        };

        struct computation_builder{
                computation make(std::vector<frontend::range> const& players){
                        switch(players.size()){
                        case 2: return do_make<2>(players);
                        case 3: return do_make<3>(players);
                        default: assert( 0 && "not implemented");
                        }
                }
        private:
                template<size_t N>
                computation do_make(std::vector<frontend::range> const& players){
                        std::vector<frontend::primitive_range> players_aux;
                        for( auto const& rng : players){
                                players_aux.emplace_back( expand( rng ).to_primitive_range() );
                        }
                }

        };
        #endif


}

int main(){

        using namespace ps;

        using namespace ps::frontend;

        range hero;
        hero += _AKo++;

        range villian;
        villian += _55-_77;

        auto star = std::make_shared<symbolic_range>( std::vector<frontend::range>{hero, villian} );

        star->print();
        

        
}
