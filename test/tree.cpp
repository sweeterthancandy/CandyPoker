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
#include <gtest/gtest.h>

#include "ps/base/tree.h"
#include "ps/base/frontend.h"
#include "ps/base/frontend_decl.h"

using namespace ps;
using namespace ps::frontend;

TEST(tree, _){
        frontend::range p0;
        frontend::range p1;

        p0 += _AKs;
        p1 += _22;

        tree_range root{ std::vector<frontend::range>{p0, p1} };

        EXPECT_EQ( 2, root.players.size());
        ASSERT_EQ( 1, root.children.size());

        auto const& child = root.children.front();

        // 6 * 4 = 24
        EXPECT_EQ( 24, child.children.size());

        root.display();



}
