#include <gtest/gtest.h>
#include <vector>
#include "flextilelayouter.h"

namespace {
using Tile = FlexTileLayouter::Tile;

std::vector<Tile> createTiles(size_t count)
{
    std::vector<Tile> tiles;
    tiles.reserve(count);
    for (size_t i = 0; i < count; ++i) {
        tiles.push_back({ { 0.0, 0.0, 0.0, 0.0 }, {}, {}, {}, {}, {}, {} });
    }
    return tiles;
}
}

TEST(FlexTileLayouterTest, Initial)
{
    FlexTileLayouter layouter;
    ASSERT_EQ(layouter.count(), 1);

    EXPECT_EQ(layouter.tileAt(0).normRect.x0, 0.0);
    EXPECT_EQ(layouter.tileAt(0).normRect.x1, 1.0);

    EXPECT_EQ(layouter.tileAt(0).normRect.y0, 0.0);
    EXPECT_EQ(layouter.tileAt(0).normRect.y1, 1.0);
}

TEST(FlexTileLayouterTest, SplitH2)
{
    FlexTileLayouter layouter;
    layouter.split(0, Qt::Horizontal, createTiles(1), {});
    ASSERT_EQ(layouter.count(), 2);

    EXPECT_EQ(layouter.tileAt(0).normRect.x0, 0.0);
    EXPECT_EQ(layouter.tileAt(0).normRect.x1, layouter.tileAt(1).normRect.x0);
    EXPECT_EQ(layouter.tileAt(1).normRect.x1, 1.0);
    EXPECT_DOUBLE_EQ(layouter.tileAt(1).normRect.x0, 0.5);

    EXPECT_EQ(layouter.tileAt(0).normRect.y0, 0.0);
    EXPECT_EQ(layouter.tileAt(0).normRect.y1, 1.0);
    EXPECT_EQ(layouter.tileAt(1).normRect.y0, 0.0);
    EXPECT_EQ(layouter.tileAt(1).normRect.y1, 1.0);
}

TEST(FlexTileLayouterTest, SplitV2)
{
    FlexTileLayouter layouter;
    layouter.split(0, Qt::Vertical, createTiles(1), {});
    ASSERT_EQ(layouter.count(), 2);

    EXPECT_EQ(layouter.tileAt(0).normRect.x0, 0.0);
    EXPECT_EQ(layouter.tileAt(0).normRect.x1, 1.0);
    EXPECT_EQ(layouter.tileAt(1).normRect.x0, 0.0);
    EXPECT_EQ(layouter.tileAt(1).normRect.x1, 1.0);

    EXPECT_EQ(layouter.tileAt(0).normRect.y0, 0.0);
    EXPECT_EQ(layouter.tileAt(0).normRect.y1, layouter.tileAt(1).normRect.y0);
    EXPECT_EQ(layouter.tileAt(1).normRect.y1, 1.0);
    EXPECT_DOUBLE_EQ(layouter.tileAt(1).normRect.y0, 0.5);
}

TEST(FlexTileLayouterTest, Split3x3)
{
    FlexTileLayouter layouter;
    layouter.split(0, Qt::Horizontal, createTiles(2), { 0.1, 0.1 });
    layouter.split(0, Qt::Vertical, createTiles(2), { 0.1, 0.1 });
    layouter.split(3, Qt::Vertical, createTiles(2), { 0.1, 0.1 });
    layouter.split(6, Qt::Vertical, createTiles(2), { 0.1, 0.1 });
    ASSERT_EQ(layouter.count(), 9);

    EXPECT_EQ(layouter.tileAt(0).normRect.x0, 0.0);
    EXPECT_EQ(layouter.tileAt(1).normRect.x0, 0.0);
    EXPECT_EQ(layouter.tileAt(2).normRect.x0, 0.0);
    EXPECT_EQ(layouter.tileAt(0).normRect.x1, layouter.tileAt(3).normRect.x0);
    EXPECT_EQ(layouter.tileAt(1).normRect.x1, layouter.tileAt(4).normRect.x0);
    EXPECT_EQ(layouter.tileAt(2).normRect.x1, layouter.tileAt(5).normRect.x0);
    EXPECT_EQ(layouter.tileAt(3).normRect.x1, layouter.tileAt(6).normRect.x0);
    EXPECT_EQ(layouter.tileAt(4).normRect.x1, layouter.tileAt(7).normRect.x0);
    EXPECT_EQ(layouter.tileAt(5).normRect.x1, layouter.tileAt(8).normRect.x0);
    EXPECT_EQ(layouter.tileAt(6).normRect.x1, 1.0);
    EXPECT_EQ(layouter.tileAt(7).normRect.x1, 1.0);
    EXPECT_EQ(layouter.tileAt(8).normRect.x1, 1.0);
    EXPECT_DOUBLE_EQ(layouter.tileAt(3).normRect.x0, 1.0 / 3.0);
    EXPECT_DOUBLE_EQ(layouter.tileAt(4).normRect.x0, 1.0 / 3.0);
    EXPECT_DOUBLE_EQ(layouter.tileAt(5).normRect.x0, 1.0 / 3.0);
    EXPECT_DOUBLE_EQ(layouter.tileAt(6).normRect.x0, 2.0 / 3.0);
    EXPECT_DOUBLE_EQ(layouter.tileAt(7).normRect.x0, 2.0 / 3.0);
    EXPECT_DOUBLE_EQ(layouter.tileAt(8).normRect.x0, 2.0 / 3.0);

    EXPECT_EQ(layouter.tileAt(0).normRect.y0, 0.0);
    EXPECT_EQ(layouter.tileAt(3).normRect.y0, 0.0);
    EXPECT_EQ(layouter.tileAt(6).normRect.y0, 0.0);
    EXPECT_EQ(layouter.tileAt(0).normRect.y1, layouter.tileAt(1).normRect.y0);
    EXPECT_EQ(layouter.tileAt(3).normRect.y1, layouter.tileAt(4).normRect.y0);
    EXPECT_EQ(layouter.tileAt(6).normRect.y1, layouter.tileAt(7).normRect.y0);
    EXPECT_EQ(layouter.tileAt(1).normRect.y1, layouter.tileAt(2).normRect.y0);
    EXPECT_EQ(layouter.tileAt(4).normRect.y1, layouter.tileAt(5).normRect.y0);
    EXPECT_EQ(layouter.tileAt(7).normRect.y1, layouter.tileAt(8).normRect.y0);
    EXPECT_EQ(layouter.tileAt(2).normRect.y1, 1.0);
    EXPECT_EQ(layouter.tileAt(5).normRect.y1, 1.0);
    EXPECT_EQ(layouter.tileAt(8).normRect.y1, 1.0);
    EXPECT_DOUBLE_EQ(layouter.tileAt(1).normRect.y0, 1.0 / 3.0);
    EXPECT_DOUBLE_EQ(layouter.tileAt(4).normRect.y0, 1.0 / 3.0);
    EXPECT_DOUBLE_EQ(layouter.tileAt(7).normRect.y0, 1.0 / 3.0);
    EXPECT_DOUBLE_EQ(layouter.tileAt(2).normRect.y0, 2.0 / 3.0);
    EXPECT_DOUBLE_EQ(layouter.tileAt(5).normRect.y0, 2.0 / 3.0);
    EXPECT_DOUBLE_EQ(layouter.tileAt(8).normRect.y0, 2.0 / 3.0);
}

TEST(FlexTileLayouterTest, Close1)
{
    FlexTileLayouter layouter;
    ASSERT_EQ(layouter.count(), 1);

    const int collapsedToIndex = layouter.close(0);
    ASSERT_EQ(layouter.count(), 1);
    EXPECT_EQ(collapsedToIndex, -1);
}

TEST(FlexTileLayouterTest, Close2ToLeft)
{
    FlexTileLayouter layouter;
    layouter.split(0, Qt::Horizontal, createTiles(1), {});
    ASSERT_EQ(layouter.count(), 2);

    const int collapsedToIndex = layouter.close(1);
    ASSERT_EQ(layouter.count(), 1);
    EXPECT_EQ(collapsedToIndex, 0);

    EXPECT_EQ(layouter.tileAt(0).normRect.x0, 0.0);
    EXPECT_EQ(layouter.tileAt(0).normRect.x1, 1.0);

    EXPECT_EQ(layouter.tileAt(0).normRect.y0, 0.0);
    EXPECT_EQ(layouter.tileAt(0).normRect.y1, 1.0);
}

TEST(FlexTileLayouterTest, Close2ToRight)
{
    FlexTileLayouter layouter;
    layouter.split(0, Qt::Horizontal, createTiles(1), {});
    ASSERT_EQ(layouter.count(), 2);

    const int collapsedToIndex = layouter.close(0);
    ASSERT_EQ(layouter.count(), 1);
    EXPECT_EQ(collapsedToIndex, 0);

    EXPECT_EQ(layouter.tileAt(0).normRect.x0, 0.0);
    EXPECT_EQ(layouter.tileAt(0).normRect.x1, 1.0);

    EXPECT_EQ(layouter.tileAt(0).normRect.y0, 0.0);
    EXPECT_EQ(layouter.tileAt(0).normRect.y1, 1.0);
}

TEST(FlexTileLayouterTest, Close2ToTop)
{
    FlexTileLayouter layouter;
    layouter.split(0, Qt::Vertical, createTiles(1), {});
    ASSERT_EQ(layouter.count(), 2);

    const int collapsedToIndex = layouter.close(1);
    ASSERT_EQ(layouter.count(), 1);
    EXPECT_EQ(collapsedToIndex, 0);

    EXPECT_EQ(layouter.tileAt(0).normRect.x0, 0.0);
    EXPECT_EQ(layouter.tileAt(0).normRect.x1, 1.0);

    EXPECT_EQ(layouter.tileAt(0).normRect.y0, 0.0);
    EXPECT_EQ(layouter.tileAt(0).normRect.y1, 1.0);
}

TEST(FlexTileLayouterTest, Close2ToBottom)
{
    FlexTileLayouter layouter;
    layouter.split(0, Qt::Vertical, createTiles(1), {});
    ASSERT_EQ(layouter.count(), 2);

    const int collapsedToIndex = layouter.close(0);
    ASSERT_EQ(layouter.count(), 1);
    EXPECT_EQ(collapsedToIndex, 0);

    EXPECT_EQ(layouter.tileAt(0).normRect.x0, 0.0);
    EXPECT_EQ(layouter.tileAt(0).normRect.x1, 1.0);

    EXPECT_EQ(layouter.tileAt(0).normRect.y0, 0.0);
    EXPECT_EQ(layouter.tileAt(0).normRect.y1, 1.0);
}
