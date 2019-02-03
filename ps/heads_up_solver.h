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
