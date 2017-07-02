#ifndef PS_HU_SOLVER_H
#define PS_HU_SOLVER_H

#include "ps/heads_up.h"

namespace ps{


struct calc_context{
        class_equity_cacher* cec;
        hu_strategy sb_push_strat;
        hu_strategy bb_call_strat;
        double eff_stack;
        double sb;
        double bb;
        holdem_class_id sb_id;
        holdem_class_id bb_id;
};

double calc_detail(calc_context& ctx);
double calc(calc_context& ctx);


struct class_equity_cacher;

double calc( class_equity_cacher& cec,
                   hu_strategy const& sb_push_strat,
                   hu_strategy const& bb_call_strat,
                   double eff_stack, double sb, double bb);
hu_strategy solve_hu_push_fold_bb_maximal_exploitable(ps::class_equity_cacher& cec,
                                               hu_strategy const& sb_push_strat,
                                               double eff_stack, double sb, double bb); 
hu_strategy solve_hu_push_fold_sb_maximal_exploitable(ps::class_equity_cacher& cec,
                                               hu_strategy const& bb_call_strat,
                                               double eff_stack, double sb, double bb);
hu_strategy solve_hu_push_fold_bb_maximal_exploitable__ev(ps::class_equity_cacher& cec,
                                               hu_strategy const& sb_push_strat,
                                               double eff_stack, double sb, double bb);
hu_strategy solve_hu_push_fold_sb_maximal_exploitable__ev(ps::class_equity_cacher& cec,
                                               hu_strategy const& bb_call_strat,
                                               double eff_stack, double sb, double bb);
hu_strategy solve_hu_push_fold_sb(ps::class_equity_cacher& cec, double eff_stack, double sb, double bb);
void make_heads_up_table();

} // ps

#endif // PS_HU_SOLVER_H
