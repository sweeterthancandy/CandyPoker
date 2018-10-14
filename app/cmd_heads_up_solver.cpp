#include <thread>
#include <numeric>
#include <atomic>
#include <boost/format.hpp>
#include "ps/support/config.h"
#include "ps/base/frontend.h"
#include "ps/base/cards.h"
#include "ps/base/tree.h"
#include "ps/base/board_combination_iterator.h"
#include "ps/base/cards.h"
#include "ps/support/index_sequence.h"
#include "app/pretty_printer.h"
#include "ps/base/algorithm.h"
#include "ps/eval/instruction.h"
#include "ps/base/rank_hasher.h"
#include "ps/base/suit_hasher.h"
#include "ps/base/holdem_board_decl.h"
#include "ps/eval/computer_mask.h"
#include "ps/eval/computer_eval.h"
#include "ps/eval/class_cache.h"

#include <boost/timer/timer.hpp>

#include <boost/log/trivial.hpp>

#include <Eigen/Dense>
#include <fstream>

#include "ps/support/command.h"

/*
 * 2 player
        
        player ev
        =========
 
        ---+---------------
        f_ | 0
        pf | sb+bb
        pp | 2*S*Ev - (S-sb)

        ---+---------------
        pf | 0
        pp | 2*S*Ev - (S-bb)
        

        total value
        ===========


        ---+---------------
        f_ | -sb
        pf | bb
        pp | 2*S*Ev - S
        
        ---+---------------
        pf | -sb
        pp | 2*S*Ev - S





                2 players

                             P1         P2
                        -- (fold) -->                 f
                        -- (push) --> (fold) -->      pf
                        -- (push) --> (call) -->      pp
                             
                3 players
                             P1         P2         P3
                        -- (fold) --> (fold)              ff
                        -- (fold) --> (push) --> (fold)   fpf
                        -- (fold) --> (push) --> (call)   fpp
                        -- (push) --> (fold) --> (fold)   pff
                        -- (push) --> (fold) --> (call)   pfp
                        -- (push) --> (call) --> (fold)   ppf
                        -- (push) --> (call) --> (call)   ppp
 */

namespace ps{
namespace gt{


        struct eval_tree_node;

        struct gt_context{
                gt_context(size_t num_players, double eff, double sb, double bb)
                        :num_players_{num_players},
                        eff_(eff),
                        sb_(sb),
                        bb_(bb)
                {}
                size_t num_players()const{ return num_players_; }
                double eff()const{ return eff_; }
                double sb()const{ return sb_; }
                double bb()const{ return bb_; }

                gt_context& use_game_tree(std::shared_ptr<eval_tree_node> gt){
                        gt_ = gt;
                        return *this;
                }
                gt_context& use_cache(class_cache const& cc){
                        cc_ = &cc;
                        return *this;
                }
                
                std::shared_ptr<eval_tree_node> root()const{
                        return gt_;
                }
                class_cache const* cc()const{
                        return cc_;
                }
                
                friend std::ostream& operator<<(std::ostream& ostr, gt_context const& self){
                        ostr << "eff_ = " << self.eff_;
                        ostr << ", sb_ = " << self.sb_;
                        ostr << ", bb_ = " << self.bb_;
                        return ostr;
                }
        private:
                size_t num_players_;
                double eff_;
                double sb_;
                double bb_;

                std::shared_ptr<eval_tree_node> gt_;
                class_cache const* cc_;
        };


        struct eval_tree_node{
                enum{ MinusZero = 1024 };
                virtual void evaluate(Eigen::VectorXd& out,
                                      double p,
                                      gt_context const& ctx,
                                      holdem_class_vector const& vec,
                                      Eigen::VectorXd const& s)=0;
                eval_tree_node& times(size_t idx){
                        p_ticker_.push_back(static_cast<int>(idx));
                        return *this;
                }
                eval_tree_node& not_times(size_t idx){
                        if( idx == 0 ){
                                p_ticker_.push_back(MinusZero);
                        } else {
                                p_ticker_.push_back(-static_cast<int>(idx));
                        }
                        return *this;
                }
                double factor(Eigen::VectorXd const& s){
                        double result = 1.0;
                        for(auto op : p_ticker_){
                                if( op == MinusZero ){
                                        result *= ( 1. - s[0] );
                                } else if( op >= 0 ){
                                        result *= s[static_cast<size_t>(op)];
                                } else{
                                        result *= ( 1. - s[static_cast<size_t>(-op)] );
                                }
                        }
                        //std::cout << "factor(" << vector_to_string(s) << ", [" << detail::to_string(p_ticker_) << "]) => " << result << "\n";
                        return result;
                }
        private:
                // we encode -0 here
                std::vector<int> p_ticker_;
        };
        struct eval_tree_node_static : eval_tree_node{
                explicit eval_tree_node_static(Eigen::VectorXd vec):
                        vec_{vec}
                {}

                virtual void evaluate(Eigen::VectorXd& out,
                                      double p,
                                      gt_context const& ctx,
                                      holdem_class_vector const& vec,
                                      Eigen::VectorXd const& s)override
                {
                        auto q = factor(s);
                        p *= q;
                        for(size_t idx=0;idx!=vec_.size();++idx){
                                out[idx] += vec_[idx] * p;
                        }
                        //out += vec_ * p;
                        out[vec.size()] += p;
                }
        private:
                Eigen::VectorXd vec_;
        };

        struct eval_tree_node_eval : eval_tree_node{
                explicit
                eval_tree_node_eval(std::vector<size_t> mask)
                        :mask_{mask}
                {
                        v_mask_.resize(mask_.size());
                        v_mask_.fill(0);
                        for(size_t idx=0;idx!=mask.size();++idx){
                                v_mask_(mask[idx]) = 1.0;
                        }
                }
                virtual void evaluate(Eigen::VectorXd& out,
                                      double p,
                                      gt_context const& ctx,
                                      holdem_class_vector const& vec,
                                      Eigen::VectorXd const& s)override
                {
                        auto q = factor(s);
                        p *= q;
                        if( std::fabs(p) < 0.001 )
                                return;

                        auto ev = ctx.cc()->LookupVector(vec);

                        auto equity_vec = ( mask_.size() * ev - v_mask_ ) * ctx.eff() * p;

                        out += equity_vec;
                        out[vec.size()] += p;
                }
        private:
                std::vector<size_t> mask_;
                Eigen::VectorXd v_mask_;
        };

        struct eval_tree_non_terminal
                : public eval_tree_node
                , public std::vector<std::shared_ptr<eval_tree_node> >
        {
                virtual void evaluate(Eigen::VectorXd& out,
                                      double p,
                                      gt_context const& ctx,
                                      holdem_class_vector const& vec,
                                      Eigen::VectorXd const& s)override
                {
                        auto q = factor(s);
                        p *= q;
                        for(auto& ptr : *this){
                                ptr->evaluate(out, p, ctx, vec, s);
                        }
                }
        };

        struct hu_eval_tree : eval_tree_non_terminal{
                explicit
                hu_eval_tree( gt_context const& ctx){

                        #if 0
                        Eigen::VectorXd pot{2};
                        pot(0) = ctx.sb();
                        pot(1) = ctx.bb();

                        Eigen::VectorXd v_f_{2};
                        v_f_(0) = -ctx.sb();
                        v_f_(1) =  ctx.sb();
                        auto n_f_ = std::make_shared<eval_tree_node_static>(v_f_);
                        n_f_->not_times(0);
                        push_back(n_f_);

                        auto n_p_ = std::make_shared<eval_tree_non_terminal>();
                        n_p_->times(0);


                        Eigen::VectorXd v_pf{2};
                        v_pf(0) =  ctx.bb();
                        v_pf(1) = -ctx.bb();
                        auto n_pf = std::make_shared<eval_tree_node_static>(v_pf);
                        n_pf->not_times(1);
                        n_p_->push_back(n_pf);

                        std::vector<size_t> m_pp;
                        m_pp.push_back(0);
                        m_pp.push_back(1);
                        auto n_pp = std::make_shared<eval_tree_node_eval>(m_pp);
                        n_pp->times(0);
                        n_pp->times(1);
                        n_p_->push_back(n_pp);
                        #endif

                        #if 1
                        do{
                                Eigen::VectorXd v_f_{2};
                                v_f_(0) = -ctx.sb();
                                v_f_(1) =  ctx.sb();
                                auto n_f_ = std::make_shared<eval_tree_node_static>(v_f_);
                                n_f_->not_times(0);
                                push_back(n_f_);
                        }while(0);

                        do{
                                Eigen::VectorXd v_pf{2};
                                v_pf(0) =  ctx.bb();
                                v_pf(1) = -ctx.bb();
                                auto n_pf = std::make_shared<eval_tree_node_static>(v_pf);
                                n_pf->times(0);
                                n_pf->not_times(1);
                                push_back(n_pf);
                        }while(0);

                        do{
                                std::vector<size_t> m_pp;
                                m_pp.push_back(0);
                                m_pp.push_back(1);
                                auto n_pp = std::make_shared<eval_tree_node_eval>(m_pp);
                                n_pp->times(0);
                                n_pp->times(1);
                                push_back(n_pp);
                        }while(0);
                        #endif

                }
        };
#if 0
        struct hu_pot_game_tree : eval_tree_non_terminal{
                explicit
                hu_pot_game_tree( gt_context const& ctx){


                        do{
                                Eigen::VectorXd v_f_{2};
                                v_f_(0) = -ctx.sb();
                                v_f_(1) =  ctx.sb();
                                auto n_f_ = std::make_shared<eval_tree_node_static>(v_f_);
                                n_f_->not_times(0);
                                push_back(n_f_);
                        }while(0);

                        do{
                                Eigen::VectorXd v_pf{2};
                                v_pf(0) =  ctx.bb();
                                v_pf(1) = -ctx.bb();
                                auto n_pf = std::make_shared<eval_tree_node_static>(v_pf);
                                n_pf->times(0);
                                n_pf->not_times(1);
                                push_back(n_pf);
                        }while(0);

                        do{
                                std::vector<size_t> m_pp;
                                m_pp.push_back(0);
                                m_pp.push_back(1);
                                auto n_pp = std::make_shared<eval_tree_node_eval>(m_pp);
                                n_pp->times(0);
                                n_pp->times(1);
                                push_back(n_pp);
                        }while(0);

                }
        };
#endif

        struct decl_none{};
        struct decl_post_sb{};
        struct decl_post_bb{};
        struct decl_push{};
        struct decl_fold{};
        struct decl_pot{};

        using action_type = boost::variant<decl_none, decl_post_sb, decl_post_bb, decl_push, decl_fold, decl_pot>;

        using action_vec = std::vector<action_type>;

        struct branch{

                explicit branch(gt_context const& gctx){
                        active_.resize(gctx.num_players());
                        active_.fill(0.);
                        pot_.resize(gctx.num_players());
                        pot_.fill(0.);
                        stack_.resize(gctx.num_players());
                        stack_.fill(gctx.eff());
                }
                branch(branch const& parent){
                        pot_    = parent.pot_;
                        stack_  = parent.stack_;
                        active_ = parent.active_;
                        head_   = parent.head_;
                        end_    = parent.end_;
                        increment_ptr();
                }

                // modifiers
                void put_in_pot(double val){
                        auto capped_val = std::max(val, stack_(head_));
                        pot_(head_) += capped_val;
                        stack_(head_) -= capped_val;
                }
                void increment_ptr(){
                        size_t last = head_;
                        for(;;){
                                increment_ptr_impl();
                                if( active_[head_] )
                                        break;
                                if( head_ == last ){
                                        end_ = true;
                                        break;
                                }
                        }
                }
                void increment_ptr_impl(){
                        ++head_;
                        head_ = head_ % active_.size();
                }


                static std::shared_ptr<branch> generate(gt_context const& gctx,
                                                        action_vec const& forced,
                                                        action_vec const& choices);
        private:
                friend class game_tree_builder;

                Eigen::VectorXd pot_;
                Eigen::VectorXd stack_;
                Eigen::VectorXi active_;
                size_t head_{0};
                bool end_{false};
                std::vector<branch> next_;
                action_type action_;
        };
        
        std::shared_ptr<branch> branch::generate(gt_context const& gctx,
                                                 action_vec const& forced,
                                                 action_vec const& choices)
        {
                std::shared_ptr<branch> root(new branch{gctx});

                struct forced_accept : boost::static_visitor<void>{
                        explicit
                        forced_accept(std::shared_ptr<branch> head_)
                                :head(head_)
                        {}
                        void operator()(decl_post_sb const& a){
                        }
                        void operator()(decl_post_bb const& a){
                        }
                        void operator()(decl_push const& a){
                        }
                        void operator()(decl_fold const& a){}
                        void operator()(decl_pot const& a){}
                        void operator()(decl_none const& a){}
                        std::shared_ptr<branch> head;
                };

                forced_accept fa{root};
                for(auto const& a : forced ){
                        boost::apply_visitor(fa, a);
                }
                return root;
        }
        struct Scratch : Command{
                explicit
                Scratch(std::vector<std::string> const& args):players_s_{args}{}
                virtual int Execute()override{
                        return EXIT_SUCCESS;
                }
        private:
                std::vector<std::string> const& players_s_;
        };
        static TrivialCommandDecl<Scratch> ScratchDecl{"scratch"};

        #if 0
        struct game_tree_builder{


                explicit
                game_tree_builder(gt_context const& ctx)
                        : gtctx_{&ctx}
                {
                        actions_.emplace_back();
                }
                void next(){
                        actions_.emplace_back();
                }
                game_tree_builder& post_sb(){
                        head_->
                        return *this;
                }
                game_tree_builder& post_bb(){
                        actions_.back().emplace_back(decl_post_bb());
                        return *this;
                }
                game_tree_builder& push(){
                        actions_.back().emplace_back(decl_push());
                        return *this;
                }
                game_tree_builder& fold(){
                        actions_.back().emplace_back(decl_fold());
                        return *this;
                }
                game_tree_builder& pot(){
                        actions_.back().emplace_back(decl_pot());
                        return *this;
                }
                std::shared_ptr<eval_tree_node> make()const{

                        struct branch{
                                Eigen::VectorXd pot;
                                Eigen::VectorXd stack;
                                std::vector<size_t> active;
                                size_t ptr_{0};

                        };
                        struct context{
                                context(gt_context const* gtctx_)
                                        : gtctx{gtctx_}
                                {
                                        graph.emplace_back();
                                        graph.back().active.resize(num_players_);
                                        graph.back().active.fill(0.);
                                        graph.back().pot.resize(num_players_);
                                        graph.back().pot.fill(0.);
                                        graph.back().stack.resize(num_players_);
                                        graph.back().stack.fill(gtctx->eff())
                                }
                                gt_context const& gtctx;
                                std::vector<branch> graph;
                                std::vector<branch> out;
                        };

                        struct accept_type : boost::static_visitor<void>{
                                void operator()(decl_post_sb const& a){
                                        BOOST_ASSERT( ctx->graph.size() == 1 );

                                        auto new_branch = graph;
                                        for(auto& _ : copy){
                                                _.
                                        }
                                }
                                void operator()(decl_post_bb const& a){
                                        BOOST_ASSERT( ctx->graph.size() == 1 );
                                        
                                        auto& g = ctx->graph.back().put_in_pot( ctx->gtctx->bb() );
                                }
                                void operator()(decl_push const& a){
                                }
                                void operator()(decl_fold const& a){}
                                void operator()(decl_pot const& a){}
                                context* ctx;
                        };
                        context ctx(num_players_);
                        accept_type accept{&ctx};

                        for(auto const& a : actions_){
                                boost::apply_visitor( accept, a);
                        }
                }
        private:
                gt_context const& gtctx_;
                std::shared_ptr<branch> head_;
                std::vector<std::shared_ptr<branch> > head_;
        };
        #endif
        
        


        // returns a vector each players hand value
        Eigen::VectorXd combination_value(gt_context const& ctx,
                                          holdem_class_vector const& vec,
                                          Eigen::VectorXd const& s){
                Eigen::VectorXd result{vec.size()+1};
                result.fill(.0);
                ctx.root()->evaluate(result, 1., ctx, vec,s);

                return result;
        }

        Eigen::VectorXd unilateral_detail(gt_context const& ctx,
                                          size_t idx,
                                          std::vector<Eigen::VectorXd> const& S,
                                          double weighted = true)
        {
                Eigen::VectorXd result(169);
                result.fill(.0);
                        
                Eigen::VectorXd s(ctx.num_players());

                for(holdem_class_perm_iterator iter(ctx.num_players()),end;iter!=end;++iter){

                        auto const& cv = *iter;
                        auto p = cv.prob();
                        for(size_t idx=0;idx!=cv.size();++idx){
                                s[idx] = S[idx][cv[idx]];
                        }
                        auto meta_result = combination_value(ctx, cv, s);
                        result(cv[idx]) += p * meta_result[idx];
                }

                return result;
        }
        Eigen::VectorXd choose_push_fold(Eigen::VectorXd const& push, Eigen::VectorXd const& fold){
                Eigen::VectorXd result(169);
                result.fill(.0);
                for(holdem_class_id id=0;id!=169;++id){
                        if( push(id) >= fold(id) ){
                                result(id) = 1.0;
                        }
                }
                return result;
        }
        Eigen::VectorXd clamp(Eigen::VectorXd s){
                for(holdem_class_id idx=0;idx!=169;++idx){
                        s(idx) = ( s(idx) < .5 ? .0 : 1. );
                }
                return s;
        }
        
        static Eigen::VectorXd fold_s = Eigen::VectorXd::Zero(169);
        static Eigen::VectorXd push_s = Eigen::VectorXd::Ones(169);
        Eigen::VectorXd unilateral_maximal_exploitable(gt_context const& ctx, size_t idx, std::vector<Eigen::VectorXd> const& S)
        {

                auto copy = S;
                copy[idx] = fold_s;
                auto fold = unilateral_detail(ctx, idx, copy);
                copy[idx] = push_s;
                auto push = unilateral_detail(ctx, idx, copy);
                return choose_push_fold(push, fold);
        }

        std::vector<Eigen::VectorXd>
        unilateral_maximal_explitable_step(gt_context const& ctx,
                                           std::vector<Eigen::VectorXd> const& state)
        {
                double factor = 0.05;
                auto bb_counter = unilateral_maximal_exploitable(ctx,1, state);
                auto tmp = state;
                tmp[1] = bb_counter;
                auto sb_counter = unilateral_maximal_exploitable(ctx,0, tmp);
                auto copy = state;
                copy[0] *= ( 1 - factor );
                copy[0] +=  factor * sb_counter;
                copy[1]  = bb_counter;
                return copy;
        }




        

        struct solver{
                virtual std::vector<Eigen::VectorXd> step(gt_context const& ctx,
                                                          std::vector<Eigen::VectorXd> const& state)=0;
        };
        struct maximal_exploitable_solver : solver{
                explicit maximal_exploitable_solver(double factor = 0.05):factor_{factor}{}
                virtual std::vector<Eigen::VectorXd> step(gt_context const& ctx,
                                                          std::vector<Eigen::VectorXd> const& state)override
                {
                        auto bb_counter = unilateral_maximal_exploitable(ctx, 1, state);
                        auto tmp = state;
                        tmp[1] = bb_counter;
                        auto sb_counter = unilateral_maximal_exploitable(ctx, 0, tmp);
                        auto copy = state;
                        copy[0] *= ( 1 - factor_ );
                        copy[0] +=  factor_ * sb_counter;
                        copy[1]  = bb_counter;
                        return copy;
                }
        private:
                double factor_;
        };

        struct cond_single_strategy_lp1{
                using state_t = std::vector<Eigen::VectorXd>;
                cond_single_strategy_lp1(size_t idx, double epsilon)
                        :idx_(idx),
                        epsilon_(epsilon)
                {}
                bool operator()(state_t const& from, state_t const& to)const{
                        auto d = from[idx_] - to[idx_];
                        auto norm = d.lpNorm<1>();
                        auto cond = ( norm < epsilon_ );
                        return cond;
                }
        private:
                size_t idx_;
                double epsilon_;
        };

        struct make_solver{
                
                enum{ DefaultMaxIter = 400 };

                using state_t         = std::vector<Eigen::VectorXd>;
                using step_observer_t = std::function<void(state_t const&)>;
                using stoppage_condition_t = std::function<bool(state_t const&, state_t const&)>;

                explicit make_solver(gt_context const& ctx){
                        ctx_ = &ctx;
                }
                make_solver& use_solver(std::shared_ptr<solver> s){
                        solver_ = s;
                        return *this;
                }
                make_solver& max_steps(size_t n){
                        max_steps_ = n;
                        return *this;
                }
                make_solver& init_state(std::vector<Eigen::VectorXd> const& s0){
                        state0_ = s0;
                        return *this;
                }
                make_solver& observer(step_observer_t obs){
                        obs_.push_back(obs);
                        return *this;
                }
                make_solver& stoppage_condition(stoppage_condition_t cond){
                        stop_cond_ = cond;
                        return *this;
                }
                std::vector<Eigen::VectorXd> run(){

                        BOOST_ASSERT(ctx_ );
                        BOOST_ASSERT(solver_ );
                        BOOST_ASSERT(state0_.size() );
                        BOOST_ASSERT( stop_cond_ );

                        std::vector<Eigen::VectorXd> state = state0_;
                        for(auto& _ : obs_){
                                _(state);
                        }

                        for(size_t idx=0;idx<max_steps_;++idx){

                                auto next = solver_->step(*ctx_, state);

                                if( stop_cond_(state, next) ){
                                        state[0] = clamp(state[0]);
                                        state[1] = clamp(state[1]);
                                        return state;
                                }
                                state = next;
                                for(auto& _ : obs_){
                                        _(state);
                                }

                        }

                        std::vector<Eigen::VectorXd> result;
                        result.push_back(Eigen::VectorXd::Zero(169));
                        result.push_back(Eigen::VectorXd::Zero(169));

                        BOOST_LOG_TRIVIAL(warning) << "Failed to converge solve ctx = " << *ctx_;
                        return result;
                }
        private:
                gt_context const* ctx_;
                std::shared_ptr<solver> solver_;
                std::vector<Eigen::VectorXd> state0_;
                std::vector<step_observer_t > obs_;
                stoppage_condition_t stop_cond_;
                size_t max_steps_{DefaultMaxIter};
        };




} // end namespace gt

struct HeadUpSolverCmd : Command{
        explicit
        HeadUpSolverCmd(std::vector<std::string> const& args):args_{args}{}
        virtual int Execute()override{
                class_cache cc;
	
                std::string cache_name{".cc.bin"};
                try{
                        cc.load(cache_name);
                }catch(std::exception const& e){
                        std::cerr << "Failed to load (" << e.what() << ")\n";
                        throw;
                }

                using namespace gt;

                std::vector<Eigen::VectorXd> state0;
                state0.push_back(Eigen::VectorXd::Zero(169));
                state0.push_back(Eigen::VectorXd::Zero(169));

                using result_t = std::future<std::tuple<double, std::vector<Eigen::VectorXd> > >;
                std::vector<result_t> tmp;


                auto enque = [&](double eff){
                        tmp.push_back(std::async([eff,&cc,&state0](){
                                gt_context gtctx(2, eff, .5, 1.);
                                auto root = std::make_shared<hu_eval_tree>(gtctx);
                                gtctx.use_game_tree(root);
                                gtctx.use_cache(cc);
                                auto result = make_solver(gtctx)
                                        .use_solver(std::make_shared<maximal_exploitable_solver>())
                                        .stoppage_condition(cond_single_strategy_lp1(0, 0.1))
                                        .init_state(state0)
                                        .run();
                                return std::make_tuple(eff, result);
                        }));
                };
                #if 0
                for(double eff = 10.0;eff <= 20.0;eff+=1){
                        enque(eff);
                }
                #else
                enque(20);
                #endif
                Eigen::VectorXd s0(169);
                s0.fill(.0);
                Eigen::VectorXd s1(169);
                s1.fill(.0);
                for(auto& _ : tmp){
                        auto aux = _.get();
                        auto eff = std::get<0>(aux);
                        auto const& vec = std::get<1>(aux);
                        for(size_t idx=0;idx!=169;++idx){
                                s0(idx) = std::max(s0(idx), eff*vec[0](idx));
                                s1(idx) = std::max(s1(idx), eff*vec[1](idx));
                        }
                }
                
                pretty_print_strat(s0, 1);
                pretty_print_strat(s1, 1);




                return EXIT_SUCCESS;
        }
private:
        std::vector<std::string> const& args_;
};
static TrivialCommandDecl<HeadUpSolverCmd> HeadsUpSolverCmdDecl{"heads-up-solver"};
        












#if NOT_DEFINED
        std::vector<Eigen::VectorXd>
        unilateral_chooser_step(gt_context const& ctx,
                                std::vector<Eigen::VectorXd> const& state)
        {
                auto bb_counter = unilateral_bb_maximal_exploitable(ctx,
                                                                    cache,
                                                                    state);

                auto head = unilateral_detail(ctx, cache, 0, state[0], bb_counter, false);
                auto fold = unilateral_detail(ctx, cache, 0, fold_s, bb_counter, false);
                auto push = unilateral_detail(ctx, cache, 0, push_s, bb_counter, false);

                using Operator = std::tuple<size_t, double, double, double>;

                std::vector<Operator> ops;

                auto add_cand = [&](size_t idx, double cand, double cand_val){
                        auto d = cand - head(idx);
                        if( d > 0.0 ){
                                ops.emplace_back(idx, cand, cand_val, d);
                        }
                };

                for(size_t idx=0;idx!=169;++idx){
                        add_cand(idx, fold(idx), 0.0);
                        add_cand(idx, push(idx), 1.0);
                }

                std::sort(ops.begin(), ops.end(), [](auto const& l, auto const& r){
                        return std::get<3>(l) > std::get<3>(r);
                });

                #if 0
                enum{ CandidateSetSz = 3 };
                if( ops.size() > CandidateSetSz ){
                        ops.resize(CandidateSetSz);
                }
                std::vector<std::tuple<Eigen::VectorXd, double> > candidates;
                for(auto const& t : ops){
                        auto cand_strat = state[0];
                        cand_strat[std::get<0>(t)] = std::get<2>(t);
                        auto counter = unilateral_bb_maximal_exploitable(ctx,
                                                                         cache,
                                                                         cand_strat);
                        candidates.emplace_back(cand_strat, unilateral_detail(ctx, cache, 0, cand_strat, counter).sum());

                        std::cout << "holdem_class_decl::get(std::get<0>(t)) => " << holdem_class_decl::get(std::get<0>(t)) << "\n"; // __CandyPrint__(cxx-print-scalar,holdem_class_decl::get(std::get<0>(t)))
                        std::cout << "std::get<1>(candidates.back()) => " << std::get<1>(candidates.back()) << "\n"; // __CandyPrint__(cxx-print-scalar,std::get<1>(candidates.back()))
                }
                std::sort(candidates.begin(), candidates.end(), [](auto const& r, auto const& l){
                        return std::get<1>(r) < std::get<1>(l);
                });
                
                auto copy = state;
                if( candidates.size()){
                        copy[0] = std::get<0>(candidates.back());
                }
                copy[1] = bb_counter;
                #endif

                auto copy = state;
                ops.resize(ops.size()/2);
                copy[1] = bb_counter;
                for(auto const& t : ops){
                        auto cand_strat = state[0];
                        copy[0][std::get<0>(t)] = std::get<2>(t);
                }
                return copy;

        }
        #endif // NOT_DEFINED

} // end namespace ps
