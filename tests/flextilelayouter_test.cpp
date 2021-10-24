#include <gtest/gtest.h>
#include <vector>
#include "flextilelayouter.h"

namespace {
using Tile = FlexTileLayouter::Tile;

constexpr QRectF unitRect { 0.0, 0.0, 1.0, 1.0 };

std::vector<Tile> createTiles(size_t count)
{
    std::vector<Tile> tiles;
    tiles.reserve(count);
    for (size_t i = 0; i < count; ++i) {
        tiles.push_back({ { 0.0, 0.0, 0.0, 0.0 }, {}, {}, {}, {}, {}, {} });
    }
    return tiles;
}

void moveBorder(FlexTileLayouter &layouter, size_t index, Qt::Orientations orientations,
                const QPointF &normPos, const QSizeF &handleSize, const QSizeF &snapSize = {})
{
    layouter.startMoving(index, orientations, false, unitRect, handleSize);
    layouter.moveTo(normPos, snapSize);
    layouter.resetMovingState();
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

TEST(FlexTileLayouterTest, MoveH2)
{
    FlexTileLayouter layouter;
    layouter.split(0, Qt::Horizontal, createTiles(1), {});
    ASSERT_EQ(layouter.count(), 2);

    layouter.startMoving(1, Qt::Horizontal, false, unitRect, {});
    EXPECT_TRUE(layouter.isMoving());

    layouter.moveTo({ 0.2, 0.0 }, {});
    EXPECT_TRUE(layouter.isMoving());

    EXPECT_EQ(layouter.tileAt(0).normRect.x0, 0.0);
    EXPECT_EQ(layouter.tileAt(0).normRect.x1, 0.2);
    EXPECT_EQ(layouter.tileAt(1).normRect.x0, 0.2);
    EXPECT_EQ(layouter.tileAt(1).normRect.x1, 1.0);

    EXPECT_EQ(layouter.tileAt(0).normRect.y0, 0.0);
    EXPECT_EQ(layouter.tileAt(0).normRect.y1, 1.0);
    EXPECT_EQ(layouter.tileAt(0).normRect.y0, 0.0);
    EXPECT_EQ(layouter.tileAt(0).normRect.y1, 1.0);

    layouter.moveTo({ 0.7, 0.0 }, {});
    layouter.resetMovingState();
    EXPECT_FALSE(layouter.isMoving());

    EXPECT_EQ(layouter.tileAt(0).normRect.x0, 0.0);
    EXPECT_EQ(layouter.tileAt(0).normRect.x1, 0.7);
    EXPECT_EQ(layouter.tileAt(1).normRect.x0, 0.7);
    EXPECT_EQ(layouter.tileAt(1).normRect.x1, 1.0);

    EXPECT_EQ(layouter.tileAt(0).normRect.y0, 0.0);
    EXPECT_EQ(layouter.tileAt(0).normRect.y1, 1.0);
    EXPECT_EQ(layouter.tileAt(0).normRect.y0, 0.0);
    EXPECT_EQ(layouter.tileAt(0).normRect.y1, 1.0);
}

TEST(FlexTileLayouterTest, MoveV2)
{
    FlexTileLayouter layouter;
    layouter.split(0, Qt::Vertical, createTiles(1), {});
    ASSERT_EQ(layouter.count(), 2);

    layouter.startMoving(1, Qt::Vertical, false, unitRect, {});
    EXPECT_TRUE(layouter.isMoving());

    layouter.moveTo({ 0.0, 0.3 }, {});
    EXPECT_TRUE(layouter.isMoving());

    EXPECT_EQ(layouter.tileAt(0).normRect.x0, 0.0);
    EXPECT_EQ(layouter.tileAt(0).normRect.x1, 1.0);
    EXPECT_EQ(layouter.tileAt(0).normRect.x0, 0.0);
    EXPECT_EQ(layouter.tileAt(0).normRect.x1, 1.0);

    EXPECT_EQ(layouter.tileAt(0).normRect.y0, 0.0);
    EXPECT_EQ(layouter.tileAt(0).normRect.y1, 0.3);
    EXPECT_EQ(layouter.tileAt(1).normRect.y0, 0.3);
    EXPECT_EQ(layouter.tileAt(1).normRect.y1, 1.0);

    layouter.moveTo({ 0.0, 0.8 }, {});
    layouter.resetMovingState();
    EXPECT_FALSE(layouter.isMoving());

    EXPECT_EQ(layouter.tileAt(0).normRect.x0, 0.0);
    EXPECT_EQ(layouter.tileAt(0).normRect.x1, 1.0);
    EXPECT_EQ(layouter.tileAt(0).normRect.x0, 0.0);
    EXPECT_EQ(layouter.tileAt(0).normRect.x1, 1.0);

    EXPECT_EQ(layouter.tileAt(0).normRect.y0, 0.0);
    EXPECT_EQ(layouter.tileAt(0).normRect.y1, 0.8);
    EXPECT_EQ(layouter.tileAt(1).normRect.y0, 0.8);
    EXPECT_EQ(layouter.tileAt(1).normRect.y1, 1.0);
}

TEST(FlexTileLayouterTest, MoveH2Clamped)
{
    FlexTileLayouter layouter;
    layouter.split(0, Qt::Horizontal, createTiles(1), {});
    ASSERT_EQ(layouter.count(), 2);

    moveBorder(layouter, 1, Qt::Horizontal, { 0.0, 0.0 }, { 0.1, 0.1 });
    EXPECT_EQ(layouter.tileAt(0).normRect.x1, layouter.tileAt(1).normRect.x0);
    EXPECT_NEAR(layouter.tileAt(0).normRect.x1, 0.1, 0.00001);
    EXPECT_NEAR(layouter.tileAt(1).normRect.x0, 0.1, 0.00001);

    moveBorder(layouter, 1, Qt::Horizontal, { 1.0, 1.0 }, { 0.1, 0.1 });
    EXPECT_EQ(layouter.tileAt(0).normRect.x1, layouter.tileAt(1).normRect.x0);
    EXPECT_NEAR(layouter.tileAt(0).normRect.x1, 0.9, 0.00001);
    EXPECT_NEAR(layouter.tileAt(1).normRect.x0, 0.9, 0.00001);
}

TEST(FlexTileLayouterTest, MoveV2Clamped)
{
    FlexTileLayouter layouter;
    layouter.split(0, Qt::Vertical, createTiles(1), {});
    ASSERT_EQ(layouter.count(), 2);

    moveBorder(layouter, 1, Qt::Vertical, { 0.0, 0.0 }, { 0.1, 0.1 });
    EXPECT_EQ(layouter.tileAt(0).normRect.y1, layouter.tileAt(1).normRect.y0);
    EXPECT_NEAR(layouter.tileAt(0).normRect.y1, 0.1, 0.00001);
    EXPECT_NEAR(layouter.tileAt(1).normRect.y0, 0.1, 0.00001);

    moveBorder(layouter, 1, Qt::Vertical, { 1.0, 1.0 }, { 0.1, 0.1 });
    EXPECT_EQ(layouter.tileAt(0).normRect.y1, layouter.tileAt(1).normRect.y0);
    EXPECT_NEAR(layouter.tileAt(0).normRect.y1, 0.9, 0.00001);
    EXPECT_NEAR(layouter.tileAt(1).normRect.y0, 0.9, 0.00001);
}

TEST(FlexTileLayouterTest, MoveH2Epsilon)
{
    FlexTileLayouter layouter;
    layouter.split(0, Qt::Horizontal, createTiles(1), {});
    ASSERT_EQ(layouter.count(), 2);

    moveBorder(layouter, 1, Qt::Horizontal, { 0.0, 0.0 }, { 0.0, 0.0 });
    EXPECT_EQ(layouter.tileAt(0).normRect.x1, layouter.tileAt(1).normRect.x0);
    EXPECT_LT(layouter.tileAt(0).normRect.x0, layouter.tileAt(0).normRect.x1);
    EXPECT_LT(layouter.tileAt(1).normRect.x0, layouter.tileAt(1).normRect.x1);

    moveBorder(layouter, 1, Qt::Horizontal, { 1.0, 1.0 }, { 0.0, 0.0 });
    EXPECT_EQ(layouter.tileAt(0).normRect.x1, layouter.tileAt(1).normRect.x0);
    EXPECT_LT(layouter.tileAt(0).normRect.x0, layouter.tileAt(0).normRect.x1);
    EXPECT_LT(layouter.tileAt(1).normRect.x0, layouter.tileAt(1).normRect.x1);
}

TEST(FlexTileLayouterTest, MoveV2Epsilon)
{
    FlexTileLayouter layouter;
    layouter.split(0, Qt::Vertical, createTiles(1), {});
    ASSERT_EQ(layouter.count(), 2);

    moveBorder(layouter, 1, Qt::Vertical, { 0.0, 0.0 }, { 0.0, 0.0 });
    EXPECT_EQ(layouter.tileAt(0).normRect.y1, layouter.tileAt(1).normRect.y0);
    EXPECT_LT(layouter.tileAt(0).normRect.y0, layouter.tileAt(0).normRect.y1);
    EXPECT_LT(layouter.tileAt(1).normRect.y0, layouter.tileAt(1).normRect.y1);

    moveBorder(layouter, 1, Qt::Vertical, { 1.0, 1.0 }, { 0.0, 0.0 });
    EXPECT_EQ(layouter.tileAt(0).normRect.y1, layouter.tileAt(1).normRect.y0);
    EXPECT_LT(layouter.tileAt(0).normRect.y0, layouter.tileAt(0).normRect.y1);
    EXPECT_LT(layouter.tileAt(1).normRect.y0, layouter.tileAt(1).normRect.y1);
}

TEST(FlexTileLayouterTest, MoveHV2x2)
{
    FlexTileLayouter layouter;
    layouter.split(0, Qt::Horizontal, createTiles(1), { 0.1, 0.1 });
    layouter.split(0, Qt::Vertical, createTiles(1), { 0.1, 0.1 });
    layouter.split(2, Qt::Vertical, createTiles(1), { 0.1, 0.1 });
    ASSERT_EQ(layouter.count(), 4);

    // All borders are isolated at this moment.
    moveBorder(layouter, 3, Qt::Horizontal, { 0.4, 0.0 }, { 0.1, 0.1 });
    EXPECT_EQ(layouter.tileAt(0).normRect.x1, layouter.tileAt(2).normRect.x0);
    EXPECT_EQ(layouter.tileAt(1).normRect.x1, layouter.tileAt(3).normRect.x0);
    EXPECT_DOUBLE_EQ(layouter.tileAt(0).normRect.x1, 0.5);
    EXPECT_DOUBLE_EQ(layouter.tileAt(1).normRect.x1, 0.4);
    EXPECT_DOUBLE_EQ(layouter.tileAt(2).normRect.x0, 0.5);
    EXPECT_DOUBLE_EQ(layouter.tileAt(3).normRect.x0, 0.4);

    // Vertical border should get unisolated.
    moveBorder(layouter, 1, Qt::Vertical, { 0.0, 0.6 }, { 0.1, 0.1 });
    EXPECT_EQ(layouter.tileAt(0).normRect.y1, layouter.tileAt(1).normRect.y0);
    EXPECT_EQ(layouter.tileAt(2).normRect.y1, layouter.tileAt(3).normRect.y0);
    EXPECT_EQ(layouter.tileAt(0).normRect.y1, layouter.tileAt(2).normRect.y1);
    EXPECT_EQ(layouter.tileAt(1).normRect.y0, layouter.tileAt(3).normRect.y0);
    EXPECT_DOUBLE_EQ(layouter.tileAt(0).normRect.y1, 0.6);
    EXPECT_DOUBLE_EQ(layouter.tileAt(1).normRect.y0, 0.6);
    EXPECT_DOUBLE_EQ(layouter.tileAt(2).normRect.y1, 0.6);
    EXPECT_DOUBLE_EQ(layouter.tileAt(3).normRect.y0, 0.6);
}

TEST(FlexTileLayouterTest, MoveVH2x2)
{
    FlexTileLayouter layouter;
    layouter.split(0, Qt::Horizontal, createTiles(1), { 0.1, 0.1 });
    layouter.split(0, Qt::Vertical, createTiles(1), { 0.1, 0.1 });
    layouter.split(2, Qt::Vertical, createTiles(1), { 0.1, 0.1 });
    ASSERT_EQ(layouter.count(), 4);

    // All borders are isolated at this moment.
    moveBorder(layouter, 1, Qt::Vertical, { 0.0, 0.3 }, { 0.1, 0.1 });
    EXPECT_EQ(layouter.tileAt(0).normRect.y1, layouter.tileAt(1).normRect.y0);
    EXPECT_EQ(layouter.tileAt(2).normRect.y1, layouter.tileAt(3).normRect.y0);
    EXPECT_DOUBLE_EQ(layouter.tileAt(0).normRect.y1, 0.3);
    EXPECT_DOUBLE_EQ(layouter.tileAt(1).normRect.y0, 0.3);
    EXPECT_DOUBLE_EQ(layouter.tileAt(2).normRect.y1, 0.5);
    EXPECT_DOUBLE_EQ(layouter.tileAt(3).normRect.y0, 0.5);

    // Horizontal border should get unisolated.
    moveBorder(layouter, 2, Qt::Horizontal, { 0.8, 0.0 }, { 0.1, 0.1 });
    EXPECT_EQ(layouter.tileAt(0).normRect.x1, layouter.tileAt(2).normRect.x0);
    EXPECT_EQ(layouter.tileAt(1).normRect.x1, layouter.tileAt(3).normRect.x0);
    EXPECT_EQ(layouter.tileAt(0).normRect.x1, layouter.tileAt(1).normRect.x1);
    EXPECT_EQ(layouter.tileAt(2).normRect.x0, layouter.tileAt(3).normRect.x0);
    EXPECT_DOUBLE_EQ(layouter.tileAt(0).normRect.x1, 0.8);
    EXPECT_DOUBLE_EQ(layouter.tileAt(1).normRect.x1, 0.8);
    EXPECT_DOUBLE_EQ(layouter.tileAt(2).normRect.x0, 0.8);
    EXPECT_DOUBLE_EQ(layouter.tileAt(3).normRect.x0, 0.8);
}
