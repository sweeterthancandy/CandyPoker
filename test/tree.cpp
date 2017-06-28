#include <gtest/gtest.h>

#include "ps/tree.h"
#include "ps/frontend.h"
#include "ps/frontend_decl.h"

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

        auto const& child{root.children.front()};

        // 6 * 4 = 24
        EXPECT_EQ( 24, child.children.size());

        root.display();



}
