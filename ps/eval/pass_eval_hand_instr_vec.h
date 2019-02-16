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
#ifndef PS_EVAL_COMPUTER_MASK_H
#define PS_EVAL_COMPUTER_MASK_H

#include <future>

#include <boost/timer/timer.hpp>
#include "ps/eval/pass.h"
#include "ps/detail/dispatch.h"
#include "ps/base/rank_hasher.h"
#include "ps/base/suit_hasher.h"
#include "ps/base/holdem_board_decl.h"

#include "ps/eval/evaluator_6_card_map.h"
#include "ps/eval/pass_eval_hand_instr.h"
#include "ps/eval/rank_hash_eval.h"

#include <unordered_set>
#include <unordered_map>

#include <boost/iterator/counting_iterator.hpp>

#include <emmintrin.h>
#include <immintrin.h>

namespace ps{

struct pass_eval_hand_instr_vec_impl;

struct pass_eval_hand_instr_vec : computation_pass{


        pass_eval_hand_instr_vec();

        virtual void transform(computation_context* ctx, instruction_list* instr_list, computation_result* result)override;
private:
        std::shared_ptr<pass_eval_hand_instr_vec_impl> impl_;
};

} // end namespace ps

#endif // PS_EVAL_COMPUTER_MASK_H
