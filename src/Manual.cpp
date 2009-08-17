extern "C"
{
#include "MazeIO.h"
#include "Analysis.h"
}

#include <FL/Fl.H>
#include <FL/Fl_Window.H>
#include <FL/fl_draw.H>

const int SZ_SQ = 25;
const int SZ_WA =  5;
const int SZ_PL = 20;
const int SZ_CE = SZ_SQ + SZ_WA;

Fl_Color bg_color   = fl_rgb_color(0x60, 0x60, 0x60);
Fl_Color sq_u_color = fl_rgb_color(0xF0, 0xF0, 0xF0);
Fl_Color sq_p_color = fl_rgb_color(0x00, 0xFF, 0x00);
Fl_Color wa_a_color = fl_rgb_color(0x00, 0xF0, 0x00);
Fl_Color wa_u_color = fl_rgb_color(0xE0, 0xE0, 0xE0);
Fl_Color wa_p_color = fl_rgb_color(0x00, 0x00, 0x00);
Fl_Color pl_color   = fl_rgb_color(0xB0, 0x00, 0xB0);
Fl_Color op_color   = fl_rgb_color(0x00, 0x00, 0xFF);

class MazeWindow : public Fl_Window
{
public:
    MazeWindow(MazeMap *mm, int distsq);
    int square(int r, int c);
    int wall(int r, int c, Dir dir);
    void draw();
    int handle(int event);
    const char *turn() { return m_turn; }

private:
    const MazeMap   * const mm;
    const int       distsq;
    int             m_dist[HEIGHT][WIDTH];
    const char      *m_turn;
};

MazeWindow::MazeWindow(MazeMap *mm, int distsq)
    : Fl_Window(SZ_CE*mm_width(mm) + SZ_WA, SZ_CE*mm_height(mm) + SZ_WA,
                "MazeMap"),
      mm(mm), distsq(distsq)
{
    find_distance(mm, m_dist, mm->loc.r, mm->loc.c);
    m_turn = "T";
}

int MazeWindow::square(int r, int c)
{
    return SQUARE(mm, (r + mm->border.top)%HEIGHT, (c + mm->border.left)%WIDTH);
}

int MazeWindow::wall(int r, int c, Dir dir)
{
    return WALL(mm, (r + mm->border.top)%HEIGHT,
                    (c + mm->border.left)%WIDTH, dir);
}

void MazeWindow::draw()
{
    int W = mm_width(mm), H = mm_height(mm);

    fl_color(bg_color);
    fl_rectf(0, 0, SZ_CE*W + SZ_WA, SZ_CE*H + SZ_WA);

    // Draw cells
    for (int r = 0; r < H; ++r)
    {
        for (int c = 0; c < W; ++c)
        {
            fl_color(square(r, c) == PRESENT ? sq_p_color : sq_u_color);
            fl_rectf(c*SZ_CE + SZ_WA, r*SZ_CE + SZ_WA, SZ_SQ, SZ_SQ);
        }
    }

    // Draw horizontal walls
    for (int r = 0; r <= H; ++r)
    {
        for (int c = 0; c < W; ++c)
        {
            fl_color(wall(r, c, NORTH) == ABSENT  ? wa_a_color :
                     wall(r, c, NORTH) == PRESENT ? wa_p_color : wa_u_color);
            fl_rectf(c*SZ_CE + SZ_WA, r*SZ_CE, SZ_SQ, SZ_WA);
        }
    }

    // Draw vertical walls
    for (int r = 0; r < H; ++r)
    {
        for (int c = 0; c <= W; ++c)
        {
            fl_color(wall(r, c, WEST) == ABSENT  ? wa_a_color :
                     wall(r, c, WEST) == PRESENT ? wa_p_color : wa_u_color);
            fl_rectf(c*SZ_CE, r*SZ_CE + SZ_WA, SZ_WA, SZ_SQ);
        }
    }

    // Relative player row/column
    int pr = (mm->loc.r - mm->border.top + HEIGHT)%HEIGHT;
    int pc = (mm->loc.c - mm->border.left + WIDTH)%WIDTH;

    // Draw opponent's potential locaiton
    for (int r = 0; r < H; ++r)
    {
        for (int c = 0; c < W; ++c)
        {
            int dr = r - pr, dc = c - pc;
            if (dr*dr + dc*dc == distsq)
            {
                fl_begin_complex_polygon();
                fl_color(op_color);
                fl_circle(c*SZ_CE + SZ_WA + 0.5*SZ_SQ,
                          r*SZ_CE + SZ_WA + 0.5*SZ_SQ, 0.5*SZ_PL);
                fl_end_complex_polygon();
            }
        }
    }

    // Draw player
    fl_push_matrix();
    fl_translate(pc*SZ_CE + SZ_WA + SZ_SQ/2, pr*SZ_CE + SZ_WA + SZ_SQ/2);
    fl_scale(SZ_PL);
    fl_rotate(-90*mm->dir);
    fl_color(pl_color);
    fl_begin_complex_polygon();
    fl_vertex( 0.00, -0.40);
    fl_vertex(+0.50, +0.10);
    fl_vertex(+0.20, +0.10);
    fl_vertex(+0.20, +0.40);
    fl_vertex(-0.20, +0.40);
    fl_vertex(-0.20, +0.10);
    fl_vertex(-0.50, +0.10);
    fl_end_complex_polygon();
    fl_pop_matrix();

}

int MazeWindow::handle(int event)
{
    switch (event)
    {
    case FL_PUSH:
        return 1;

    case FL_RELEASE:
        {
            int x = Fl::event_x(), y = Fl::event_y();
            if (x%SZ_CE < SZ_WA || y%SZ_CE < SZ_WA) return 1;
            int r = (y/SZ_CE + mm->border.top)%HEIGHT,
                c = (x/SZ_CE + mm->border.left)%WIDTH;
            if (m_dist[r][c] > 0)
            {
                m_turn = construct_turn(mm, m_dist,
                                        mm->loc.r, mm->loc.c, mm->dir, r, c);
                hide();
            }
        }
        return 1;

    default:
        return 0;
    }
}


extern "C"
const char *pick_move(MazeMap *mm, int distsq)
{
    MazeWindow win(mm, distsq);
    win.end();
    win.show();
    Fl::run();
    return win.turn();
}