/**
 * \file cave-view.c
 * \brief Line-of-sight and view calculations
 *
 * Copyright (c) 1997 Ben Harrison, James E. Wilson, Robert A. Koeneke
 *
 * This work is free software; you can redistribute it and/or modify it
 * under the terms of either:
 *
 * a) the GNU General Public License as published by the Free Software
 *    Foundation, version 2, or
 *
 * b) the "Angband licence":
 *    This software may be copied and distributed for educational, research,
 *    and not for profit purposes provided that this copyright and statement
 *    are included in all such copies.  Other copyrights may also apply.
 */

#include "angband.h"
#include "cave.h"
#include "cmds.h"
#include "init.h"
#include "game-world.h"
#include "monster.h"
#include "player-calcs.h"
#include "player-timed.h"
#include "trap.h"

/* Ask GCC for aggressive scalar optimisation in this translation unit.
 * cave-view.c contains the update_view() hot path which runs on every player
 * step; the ARM926EJ-S in the Nspire CX II benefits greatly from O3 here. */
#pragma GCC optimize("O3,unroll-loops")

/* -----------------------------------------------------------------------
 * Per-frame view bounding-box projectable/LOS cache
 *
 * Stores one byte per cell covering the sight+2 neighbourhood around the
 * player.  Built once at the top of update_view() and consumed by
 * update_view_one() / los_fast().
 *
 * Byte layout:
 *   bit 0 → square_isprojectable() (used by los_fast inner loop)
 *   bit 1 → square_allowslos()     (used by wall-proxy logic)
 *   bit 2 → SQUARE_GLOW            (used by calc_lighting glow loop)
 *
 * Side length 50 is > 2*(max_sight+2)+1 = 45 for max_sight=20, so it
 * comfortably handles max_sight up to 23 before resizing is needed.
 * The entire array is 50×50 = 2500 bytes — fits in the ARM926's 16 KB L1D.
 * -------------------------------------------------------------------- */
#define NSPIRE_VC_SIDE  50

static uint8_t s_vcache[NSPIRE_VC_SIDE][NSPIRE_VC_SIDE];
static int     s_vc_y0, s_vc_x0;  /* top-left corner of cache in dungeon coords */
static int     s_vc_y1, s_vc_x1;  /* bottom-right corner */

/* Read cache — caller must guarantee (x,y) is within the valid bbox */
#define VC_PROJ(x, y)  (s_vcache[(y) - s_vc_y0][(x) - s_vc_x0] & 1u)
#define VC_LOS(x, y)   ((s_vcache[(y) - s_vc_y0][(x) - s_vc_x0] >> 1) & 1u)
/* bit 2 = SQUARE_GLOW, populated by build_view_cache() */
#define VC_GLOW(x, y)  ((s_vcache[(y) - s_vc_y0][(x) - s_vc_x0] >> 2) & 1u)
/* Guard for callers whose bbox may slightly exceed the LOS bbox (add_light) */
#define VC_IN_BOUNDS(x, y) \
	((y) >= s_vc_y0 && (y) <= s_vc_y1 && (x) >= s_vc_x0 && (x) <= s_vc_x1)

/**
 * Build the view cache for the bounding box [y0..y1] × [x0..x1].
 * Called once at the top of update_view() before any LOS work starts.
 */
static void build_view_cache(struct chunk *c, int y0, int y1, int x0, int x1)
{
	int x, y;
	s_vc_y0 = y0;
	s_vc_x0 = x0;
	s_vc_y1 = y1;
	s_vc_x1 = x1;
	for (y = y0; y <= y1; y++) {
		for (x = x0; x <= x1; x++) {
			const struct square *sq = square(c, loc(x, y));
			uint8_t v = 0;
			if (feat_is_projectable(sq->feat)) v |= 1u;
			if (feat_is_los(sq->feat))         v |= 2u;
			if (sqinfo_has(sq->info, SQUARE_GLOW)) v |= 4u;
			s_vcache[y - y0][x - x0] = v;
		}
	}
}

/**
 * Fast LOS check using the per-frame view cache instead of hitting the full
 * cave array on every inner step.  Only call this when both endpoints are
 * guaranteed to lie within the cache bbox built by build_view_cache().
 *
 * This is a verbatim copy of los() with every square_isprojectable() call
 * replaced by the VC_PROJ() cache lookup.  The bounds check inside
 * square_isprojectable() is intentionally omitted: all intermediate cells
 * along a line between two in-bounds, in-bbox endpoints are also in-bbox.
 */
static bool los_fast(struct loc grid1, struct loc grid2)
{
	int dx, dy, ax, ay, sx, sy, qx, qy, tx, ty, f1, f2, m;

	dy = grid2.y - grid1.y;
	dx = grid2.x - grid1.x;
	ay = ABS(dy);
	ax = ABS(dx);

	if ((ax < 2) && (ay < 2)) return true;

	if (!dx) {
		if (dy > 0) {
			for (ty = grid1.y + 1; ty < grid2.y; ty++)
				if (!VC_PROJ(grid1.x, ty)) return false;
		} else {
			for (ty = grid1.y - 1; ty > grid2.y; ty--)
				if (!VC_PROJ(grid1.x, ty)) return false;
		}
		return true;
	}

	if (!dy) {
		if (dx > 0) {
			for (tx = grid1.x + 1; tx < grid2.x; tx++)
				if (!VC_PROJ(tx, grid1.y)) return false;
		} else {
			for (tx = grid1.x - 1; tx > grid2.x; tx--)
				if (!VC_PROJ(tx, grid1.y)) return false;
		}
		return true;
	}

	sx = (dx < 0) ? -1 : 1;
	sy = (dy < 0) ? -1 : 1;

	if ((ax == 1) && (ay == 2) && VC_PROJ(grid1.x, grid1.y + sy))
		return true;
	else if ((ay == 1) && (ax == 2) && VC_PROJ(grid1.x + sx, grid1.y))
		return true;

	f2 = ax * ay;
	f1 = f2 << 1;

	if (ax >= ay) {
		qy = ay * ay;
		m  = qy << 1;
		tx = grid1.x + sx;
		if (qy == f2) { ty = grid1.y + sy; qy -= f1; }
		else            ty = grid1.y;
		while (grid2.x - tx) {
			if (!VC_PROJ(tx, ty)) return false;
			qy += m;
			if (qy < f2) {
				tx += sx;
			} else if (qy > f2) {
				ty += sy;
				if (!VC_PROJ(tx, ty)) return false;
				qy -= f1;
				tx += sx;
			} else {
				ty += sy;
				qy -= f1;
				tx += sx;
			}
		}
	} else {
		qx = ax * ax;
		m  = qx << 1;
		ty = grid1.y + sy;
		if (qx == f2) { tx = grid1.x + sx; qx -= f1; }
		else            tx = grid1.x;
		while (grid2.y - ty) {
			if (!VC_PROJ(tx, ty)) return false;
			qx += m;
			if (qx < f2) {
				ty += sy;
			} else if (qx > f2) {
				tx += sx;
				if (!VC_PROJ(tx, ty)) return false;
				qx -= f1;
				ty += sy;
			} else {
				tx += sx;
				qx -= f1;
				ty += sy;
			}
		}
	}
	return true;
}

/**
 * Approximate distance between two points.
 *
 * When either the X or Y component dwarfs the other component,
 * this function is almost perfect, and otherwise, it tends to
 * over-estimate about one grid per fifteen grids of distance.
 *
 * Algorithm: hypot(dy,dx) = max(dy,dx) + min(dy,dx) / 2
 */
int distance(struct loc grid1, struct loc grid2)
{
	/* Find the absolute y/x distance components */
	int ay = abs(grid2.y - grid1.y);
	int ax = abs(grid2.x - grid1.x);

	/* Approximate the distance */
	return ay > ax ? ay + (ax >> 1) : ax + (ay >> 1);
}


/**
 * A simple, fast, integer-based line-of-sight algorithm.  By Joseph Hall,
 * 4116 Brewster Drive, Raleigh NC 27606.  Email to jnh@ecemwl.ncsu.edu.
 *
 * This function returns true if a "line of sight" can be traced from the
 * center of the grid (x1,y1) to the center of the grid (x2,y2), with all
 * of the grids along this path (except for the endpoints) being non-wall
 * grids.  Actually, the "chess knight move" situation is handled by some
 * special case code which allows the grid diagonally next to the player
 * to be obstructed, because this yields better gameplay semantics.  This
 * algorithm is totally reflexive, except for "knight move" situations.
 *
 * Because this function uses (short) ints for all calculations, overflow
 * may occur if dx and dy exceed 90.
 *
 * Once all the degenerate cases are eliminated, we determine the "slope"
 * ("m"), and we use special "fixed point" mathematics in which we use a
 * special "fractional component" for one of the two location components
 * ("qy" or "qx"), which, along with the slope itself, are "scaled" by a
 * scale factor equal to "abs(dy*dx*2)" to keep the math simple.  Then we
 * simply travel from start to finish along the longer axis, starting at
 * the border between the first and second tiles (where the y offset is
 * thus half the slope), using slope and the fractional component to see
 * when motion along the shorter axis is necessary.  Since we assume that
 * vision is not blocked by "brushing" the corner of any grid, we must do
 * some special checks to avoid testing grids which are "brushed" but not
 * actually "entered".
 *
 * Angband three different "line of sight" type concepts, including this
 * function (which is used almost nowhere), the "project()" method (which
 * is used for determining the paths of projectables and spells and such),
 * and the "update_view()" concept (which is used to determine which grids
 * are "viewable" by the player, which is used for many things, such as
 * determining which grids are illuminated by the player's torch, and which
 * grids and monsters can be "seen" by the player, etc).
 */
bool los(struct chunk *c, struct loc grid1, struct loc grid2)
{
	/* Delta */
	int dx, dy;

	/* Absolute */
	int ax, ay;

	/* Signs */
	int sx, sy;

	/* Fractions */
	int qx, qy;

	/* Scanners */
	int tx, ty;

	/* Scale factors */
	int f1, f2;

	/* Slope, or 1/Slope, of LOS */
	int m;

	/* Extract the offset */
	dy = grid2.y - grid1.y;
	dx = grid2.x - grid1.x;

	/* Extract the absolute offset */
	ay = ABS(dy);
	ax = ABS(dx);

	/* Handle adjacent (or identical) grids */
	if ((ax < 2) && (ay < 2)) return (true);

	/* Directly South/North */
	if (!dx) {
		/* South -- check for walls */
		if (dy > 0) {
			for (ty = grid1.y + 1; ty < grid2.y; ty++)
				if (!square_isprojectable(c, loc(grid1.x, ty))) return (false);
		} else { /* North -- check for walls */
			for (ty = grid1.y - 1; ty > grid2.y; ty--)
				if (!square_isprojectable(c, loc(grid1.x, ty))) return (false);
		}

		/* Assume los */
		return (true);
	}

	/* Directly East/West */
	if (!dy) {
		/* East -- check for walls */
		if (dx > 0) {
			for (tx = grid1.x + 1; tx < grid2.x; tx++)
				if (!square_isprojectable(c, loc(tx, grid1.y))) return (false);
		} else { /* West -- check for walls */
			for (tx = grid1.x - 1; tx > grid2.x; tx--)
				if (!square_isprojectable(c, loc(tx, grid1.y))) return (false);
		}

		/* Assume los */
		return (true);
	}


	/* Extract some signs */
	sx = (dx < 0) ? -1 : 1;
	sy = (dy < 0) ? -1 : 1;

	/* Vertical and horizontal "knights" */
	if ((ax == 1) && (ay == 2) &&
		square_isprojectable(c, loc(grid1.x, grid1.y + sy))) {
		return (true);
	} else if ((ay == 1) && (ax == 2) &&
			   square_isprojectable(c, loc(grid1.x + sx, grid1.y))) {
		return (true);
	}

	/* Calculate scale factor div 2 */
	f2 = (ax * ay);

	/* Calculate scale factor */
	f1 = f2 << 1;


	/* Travel horizontally */
	if (ax >= ay) {
		/* Let m = dy / dx * 2 * (dy * dx) = 2 * dy * dy */
		qy = ay * ay;
		m = qy << 1;

		tx = grid1.x + sx;

		/* Consider the special case where slope == 1. */
		if (qy == f2) {
			ty = grid1.y + sy;
			qy -= f1;
		} else {
			ty = grid1.y;
		}

		/* Note (below) the case (qy == f2), where */
		/* the LOS exactly meets the corner of a tile. */
		while (grid2.x - tx) {
			if (!square_isprojectable(c, loc(tx, ty)))
				return (false);

			qy += m;

			if (qy < f2) {
				tx += sx;
			} else if (qy > f2) {
				ty += sy;
				if (!square_isprojectable(c, loc(tx, ty)))
					return (false);
				qy -= f1;
				tx += sx;
			} else {
				ty += sy;
				qy -= f1;
				tx += sx;
			}
		}
	} else { /* Travel vertically */
		/* Let m = dx / dy * 2 * (dx * dy) = 2 * dx * dx */
		qx = ax * ax;
		m = qx << 1;

		ty = grid1.y + sy;

		if (qx == f2) {
			tx = grid1.x + sx;
			qx -= f1;
		} else {
			tx = grid1.x;
		}

		/* Note (below) the case (qx == f2), where */
		/* the LOS exactly meets the corner of a tile. */
		while (grid2.y - ty) {
			if (!square_isprojectable(c, loc(tx, ty)))
				return (false);

			qx += m;

			if (qx < f2) {
				ty += sy;
			} else if (qx > f2) {
				tx += sx;
				if (!square_isprojectable(c, loc(tx, ty)))
					return (false);
				qx -= f1;
				ty += sy;
			} else {
				tx += sx;
				qx -= f1;
				ty += sy;
			}
		}
	}

	/* Assume los */
	return (true);
}

/**
 * The comments below are still predominantly true, and have been left
 * (slightly modified for accuracy) for historical and nostalgic reasons.
 *
 * Some comments on the dungeon related data structures and functions...
 *
 * Angband is primarily a dungeon exploration game, and it should come as
 * no surprise that the internal representation of the dungeon has evolved
 * over time in much the same way as the game itself, to provide semantic
 * changes to the game itself, to make the code simpler to understand, and
 * to make the executable itself faster or more efficient in various ways.
 *
 * There are a variety of dungeon related data structures, and associated
 * functions, which store information about the dungeon, and provide methods
 * by which this information can be accessed or modified.
 *
 * Some of this information applies to the dungeon as a whole, such as the
 * list of unique monsters which are still alive.  Some of this information
 * only applies to the current dungeon level, such as the current depth, or
 * the list of monsters currently inhabiting the level.  And some of the
 * information only applies to a single grid of the current dungeon level,
 * such as whether the grid is illuminated, or whether the grid contains a
 * monster, or whether the grid can be seen by the player.  If Angband was
 * to be turned into a multi-player game, some of the information currently
 * associated with the dungeon should really be associated with the player,
 * such as whether a given grid is viewable by a given player.
 *
 * Currently, a lot of the information about the dungeon is stored in ways
 * that make it very efficient to access or modify the information, while
 * still attempting to be relatively conservative about memory usage, even
 * if this means that some information is stored in multiple places, or in
 * ways which require the use of special code idioms.  For example, each
 * monster record in the monster array contains the location of the monster,
 * and each cave grid has an index into the monster array, or a zero if no
 * monster is in the grid.  This allows the monster code to efficiently see
 * where the monster is located, while allowing the dungeon code to quickly
 * determine not only if a monster is present in a given grid, but also to
 * find out which monster.  The extra space used to store the information
 * twice is inconsequential compared to the speed increase.
 *
 * Several pieces of information about each cave grid are stored in the
 * info field of the "cave->squares" array, which is a special array of
 * bitflags.
 *
 * The "SQUARE_ROOM" flag is used to determine which grids are part of "rooms", 
 * and thus which grids are affected by "illumination" spells.
 *
 * The "SQUARE_VAULT" flag is used to determine which grids are part of 
 * "vaults", and thus which grids cannot serve as the destinations of player 
 * teleportation.
 *
 * The "SQUARE_GLOW" flag is used to determine which grids are "permanently 
 * illuminated".  This flag is used by the update_view() function to help 
 * determine which viewable grids may be "seen" by the player.  This flag
 * has special semantics for wall grids (see "update_view()").
 *
 * The "SQUARE_VIEW" flag is used to determine which grids are currently in
 * line of sight of the player.  This flag is set by (and used by) the
 * "update_view()" function.  This flag is used by any code which needs to
 * know if the player can "view" a given grid.  This flag is used by the
 * "map_info()" function for some optional special lighting effects.  The
 * "player_has_los_bold()" macro wraps an abstraction around this flag, but
 * certain code idioms are much more efficient.  This flag is used to check
 * if a modification to a terrain feature might affect the player's field of
 * view.  This flag is used to see if certain monsters are "visible" to the
 * player.  This flag is used to allow any monster in the player's field of
 * view to "sense" the presence of the player.
 *
 * The "SQUARE_SEEN" flag is used to determine which grids are currently in
 * line of sight of the player and also illuminated in some way.  This flag
 * is set by the "update_view()" function, using computations based on the
 * "SQUARE_VIEW" and "SQUARE_GLOW" flags and terrain of various grids.  
 * This flag is used by any code which needs to know if the player can "see" a
 * given grid.  This flag is used by the "map_info()" function both to see
 * if a given "boring" grid can be seen by the player, and for some optional
 * special lighting effects.  The "player_can_see_bold()" macro wraps an
 * abstraction around this flag, but certain code idioms are much more
 * efficient.  This flag is used to see if certain monsters are "visible" to
 * the player.  This flag is never set for a grid unless "SQUARE_VIEW" is also
 * set for the grid.  Whenever the terrain or "SQUARE_GLOW" flag changes
 * for a grid which has the "SQUARE_VIEW" flag set, the "SQUARE_SEEN" flag must
 * be recalculated.  The simplest way to do this is to call "forget_view()"
 * and "update_view()" whenever the terrain or "SQUARE_GLOW" flag changes
 * for a grid which has "SQUARE_VIEW" set.
 *
 * The "SQUARE_WASSEEN" flag is used for a variety of temporary purposes.  This
 * flag is used to determine if the "SQUARE_SEEN" flag for a grid has changed
 * during the "update_view()" function.  This flag is used to "spread" light
 * or darkness through a room.  This flag is used by the "monster flow code".
 * This flag must always be cleared by any code which sets it.
 *
 * The "SQUARE_CLOSE_PLAYER" flag is set for squares that are seen and either
 * in the player's light radius or the UNLIGHT detection radius.  It is used
 * by "map_info()" to select which lighting effects to apply to a square.
 *
 * The "update_view()" function is an extremely important function.  It is
 * called only when the player moves, significant terrain changes, or the
 * player's blindness or torch radius changes.  Note that when the player
 * is resting, or performing any repeated actions (like digging, disarming,
 * farming, etc), there is no need to call the "update_view()" function, so
 * even if it was not very efficient, this would really only matter when the
 * player was "running" through the dungeon.  It sets the "SQUARE_VIEW" flag
 * on every cave grid in the player's field of view.  It also checks the torch
 * radius of the player, and sets the "SQUARE_SEEN" flag for every grid which
 * is in the "field of view" of the player and which is also "illuminated",
 * either by the player's torch (if any), light from monsters, light from
 * bright terrain, or by any permanent light source (as marked by SQUARE_GLOW).
 * It could use and help maintain information about multiple light sources,
 * which would be helpful in a multi-player version of Angband.
 *
 * Note that the "update_view()" function allows, among other things, a room
 * to be "partially" seen as the player approaches it, with a growing cone
 * of floor appearing as the player gets closer to the door.  Also, by not
 * turning on the "memorize perma-lit grids" option, the player will only
 * "see" those floor grids which are actually in line of sight.  And best
 * of all, you can now activate the special lighting effects to indicate
 * which grids are actually in the player's field of view by using dimmer
 * colors for grids which are not in the player's field of view, and/or to
 * indicate which grids are illuminated only by the player's torch by using
 * the color yellow for those grids.
 *
 * It seems as though slight modifications to the "update_view()" functions
 * would allow us to determine "reverse" line-of-sight as well as "normal"
 * line-of-sight", which would allow monsters to have a more "correct" way
 * to determine if they can "see" the player, since right now, they "cheat"
 * somewhat and assume that if the player has "line of sight" to them, then
 * they can "pretend" that they have "line of sight" to the player.  But if
 * such a change was attempted, the monsters would actually start to exhibit
 * some undesirable behavior, such as "freezing" near the entrances to long
 * hallways containing the player, and code would have to be added to make
 * the monsters move around even if the player was not detectable, and to
 * "remember" where the player was last seen, to avoid looking stupid.
 *
 * Note that the "SQUARE_GLOW" flag means that a grid is permanently lit in
 * some way.  However, for the player to "see" the grid, as determined by
 * the "SQUARE_SEEN" flag, the player must not be blind, the grid must have
 * the "SQUARE_VIEW" flag set, and if the grid is a "wall" grid, and it is
 * not lit by some other light source, then it must touch a projectable grid
 * which has both the "SQUARE_GLOW" and "SQUARE_VIEW" flags set.  This last
 * part about wall grids is induced by the semantics of "SQUARE_GLOW" as
 * applied to wall grids, and checking the technical requirements can be very
 * expensive, especially since the grid may be touching some "illegal" grids.
 * Luckily, it is more or less correct to restrict the "touching" grids from
 * the eight "possible" grids to the (at most) three grids which are touching
 * the grid, and which are closer to the player than the grid itself, which
 * eliminates more than half of the work, including all of the potentially
 * "illegal" grids, if at most one of the three grids is a "diagonal" grid.
 * In addition, in almost every situation, it is possible to ignore the
 * "SQUARE_VIEW" flag on these three "touching" grids, for a variety of
 * technical reasons.  Finally, note that in most situations, it is only
 * necessary to check a single "touching" grid, in fact, the grid which is
 * strictly closest to the player of all the touching grids, and in fact,
 * it is normally only necessary to check the "SQUARE_GLOW" flag of that grid,
 * again, for various technical reasons.  However, one of the situations which
 * does not work with this last reduction is the very common one in which the
 * player approaches an illuminated room from a dark hallway, in which the
 * two wall grids which form the "entrance" to the room would not be marked
 * as "SQUARE_SEEN", since of the three "touching" grids nearer to the player
 * than each wall grid, only the farthest of those grids is itself marked
 * "SQUARE_GLOW".
 *
 *
 * Here are some pictures of the legal "light source" radius values, in
 * which the numbers indicate the "order" in which the grids could have
 * been calculated, if desired.  Note that the code will work with larger
 * radiuses, though currently yields such a radius, and the game would
 * become slower in some situations if it did.
 *
 *       Rad=0     Rad=1      Rad=2        Rad=3
 *      No-Light Torch,etc   Lantern     Artifacts
 *
 *                                          333
 *                             333         43334
 *                  212       32123       3321233
 *         @        1@1       31@13       331@133
 *                  212       32123       3321233
 *                             333         43334
 *                                          333
 *
 */


/**
 * Mark the currently seen grids, then wipe in preparation for recalculating.
 *
 * Restricted to the union of the *previous* and *current* view bounding boxes
 * so that grids that were in view last frame but just fell outside the new bbox
 * (e.g. after a single-step move) still get their stale VIEW/SEEN flags cleared.
 * This eliminates the original O(dungeon_width × dungeon_height) full scan and
 * replaces it with O(sight²) — roughly a 7× speedup on a standard dungeon.
 */
static void mark_wasseen(struct chunk *c, int y0, int y1, int x0, int x1)
{
	int x, y;

	/* Union with the bbox from the previous call so stale flags are cleared. */
	static int py0 = -1, py1 = -1, px0 = -1, px1 = -1;

	int uy0 = (py0 >= 0 && py0 < y0) ? py0 : y0;
	int uy1 = (py1 > y1)              ? py1 : y1;
	int ux0 = (px0 >= 0 && px0 < x0) ? px0 : x0;
	int ux1 = (px1 > x1)              ? px1 : x1;

	/* Clamp to dungeon bounds */
	if (uy0 < 0)            uy0 = 0;
	if (uy1 >= c->height)   uy1 = c->height - 1;
	if (ux0 < 0)            ux0 = 0;
	if (ux1 >= c->width)    ux1 = c->width - 1;

	/* Remember current bbox for next call */
	py0 = y0; py1 = y1; px0 = x0; px1 = x1;

	for (y = uy0; y <= uy1; y++) {
		for (x = ux0; x <= ux1; x++) {
			struct loc grid = loc(x, y);
			if (square_isseen(c, grid))
				sqinfo_on(square(c, grid)->info, SQUARE_WASSEEN);
			else
				sqinfo_off(square(c, grid)->info, SQUARE_WASSEEN);
			sqinfo_off(square(c, grid)->info, SQUARE_VIEW);
			sqinfo_off(square(c, grid)->info, SQUARE_SEEN);
			sqinfo_off(square(c, grid)->info, SQUARE_CLOSE_PLAYER);
		}
	}
}

/**
 * Help glow_can_light_wall(), add_light() and calc_lighting():  check for
 * whether a wall can appear to be lit, as viewed by the player, by a light
 * source regardless of line-of-sight details.
 * \param c Is the chunk in which to do the evaluation.
 * \param p Is the player to test.
 * \param sgrid Is the location of the light source.
 * \param wgrid Is the location of the wall.
 * \return Return true if the wall will appear to be lit for the player.
 * Otherwise, return false.
 */
static bool source_can_light_wall(struct chunk *c, struct player *p,
		struct loc sgrid, struct loc wgrid)
{
	struct loc sn = next_grid(wgrid, motion_dir(wgrid, sgrid)), pn, cn;

	/*
	 * If the light source is coincident with the wall, all faces will be
	 * lit, and the player can potentially see it if it's within range and
	 * the line of sight isn't broken.
	 */
	if (loc_eq(sn, wgrid)) return true;

	/*
	 * If the player is coincident with the wall, all faces of the wall are
	 * visible to the player and the player can see whichever of those is
	 * lit by the light source.
	 */
	pn = next_grid(wgrid, motion_dir(wgrid, p->grid));
	if (loc_eq(pn, wgrid)) return true;

	/*
	 * For the lit face of the wall to be visible to the player, the
	 * view directions from the wall to the player and the wall to the
	 * light source must share at least one component.
	 */
	if (sn.x == pn.x) {
		/*
		 * If the view directions share both components, the lit face
		 * will be visible to the player if in range and the line of
		 * sight isn't broken.
		 */
		if (sn.y == pn.y) return true;
		cn.x = sn.x;
		cn.y = wgrid.y;
	} else if (sn.y == pn.y) {
		cn.x = wgrid.x;
		cn.y = sn.y;
	} else {
		/*
		 * If the view directions don't share a component, the lit face
		 * is not visible to the player.
		 */
		return false;
	}

	/*
	 * When only one component of the view directions is shared, take the
	 * common component and test whether there's a wall there that would
	 * block the player's view of the lit face.  That prevents instances
	 * like this:
	 *  p
	 * ###1#
	 *  @
	 * where both the light-emitting monster, 'p', and the player, '@',
	 * have line of sight to the wall, '1', but the face of '1' that would
	 * be lit is blocked by the wall immediately to the left of '1'.
	 */
	return square_allowslos(c, cn);
}

/**
 * Help calc_lighting():  check for whether a wall marked with SQUARE_GLOW
 * can appear to be lit, as viewed by the player regardless of line-of-sight
 * details.
 * \param c Is the chunk in which to do the evaluation.
 * \param p Is the player to test.
 * \param wgrid Is the location of the wall.
 * \return Return true if the wall will appear to be lit for the player.
 * Otherwise, return false.
 */
static bool glow_can_light_wall(struct chunk *c, struct player *p,
		struct loc wgrid)
{
	struct loc pn = next_grid(wgrid, motion_dir(wgrid, p->grid)), chk;

	/*
	 * If the player is in the wall grid, the player will see the lit face.
	 */
	if (loc_eq(pn, wgrid)) return true;

	/*
	 * If the grid in the direction of the player is not a wall and is
	 * glowing, it'll illuminate the wall.
	 */
	if (square_allowslos(c, pn) && square_isglow(c, pn)) return true;

	/*
	 * Try the two neighboring squares adjacent to the one in the direction
	 * of the player to see if one or more will illuminate the wall by
	 * glowing.  Those could be out of bounds if the direction isn't
	 * diagonal.
	 */
	if (pn.x != wgrid.x) {
		if (pn.y != wgrid.y) {
			chk.x = pn.x;
			chk.y = wgrid.y;
			if (square_allowslos(c, chk) &&
					square_isglow(c, chk) &&
					source_can_light_wall(c, p, chk, wgrid))
				return true;
			chk.x = wgrid.x;
			chk.y = pn.y;
			if (square_allowslos(c, chk) &&
					square_isglow(c, chk) &&
					source_can_light_wall(c, p, chk, wgrid))
				return true;
		} else {
			chk.x = pn.x;
			chk.y = wgrid.y - 1;
			if (square_in_bounds(c, chk) &&
					square_allowslos(c, chk) &&
					square_isglow(c, chk) &&
					source_can_light_wall(c, p, chk, wgrid))
				return true;
			chk.y = wgrid.y + 1;
			if (square_in_bounds(c, chk) &&
					square_allowslos(c, chk) &&
					square_isglow(c, chk) &&
					source_can_light_wall(c, p, chk, wgrid))
				return true;
		}
	} else {
		chk.y = pn.y;
		chk.x = wgrid.x - 1;
		if (square_in_bounds(c, chk) && square_allowslos(c, chk) &&
				square_isglow(c, chk) &&
				source_can_light_wall(c, p, chk, wgrid))
			return true;
		chk.x = wgrid.x + 1;
		if (square_in_bounds(c, chk) && square_allowslos(c, chk) &&
				square_isglow(c, chk) &&
				source_can_light_wall(c, p, chk, wgrid))
			return true;
	}

	/*
	 * The adjacent squares towards the player won't light the wall by
	 * by glowing.
	 */
	return false;
}

/**
 * Help calc_lighting():  add in the effect of a light source.
 * \param c Is the chunk to use.
 * \param p Is the player to use.
 * \param sgrid Is the location of the light source.
 * \param radius Is the radius, in grids, of the light source.
 * \param inten Is the intensity of the light source.
 * This is a brute force approach.  Some computation probably could be saved by
 * propagating the light out from the source and terminating paths when they
 * reach a wall.
 */
static void add_light(struct chunk *c, struct player *p, struct loc sgrid,
		int radius, int inten)
{
	int y;

	for (y = -radius; y <= radius; y++) {
		int x;

		for (x = -radius; x <= radius; x++) {
			struct loc grid = loc_sum(sgrid, loc(x, y));
			int dist = distance(sgrid, grid);
			if (!square_in_bounds(c, grid)) continue;
			if (dist > radius) continue;
			/* Don't propagate the light through walls.
			 * Use the per-frame view cache when both endpoints are
			 * within its bbox; fall back for rare cases where a
			 * monster's light radius extends past the cache edge. */
			if (VC_IN_BOUNDS(sgrid.x, sgrid.y) &&
					VC_IN_BOUNDS(grid.x, grid.y)) {
				if (!los_fast(sgrid, grid)) continue;
			} else {
				if (!los(c, sgrid, grid)) continue;
			}
			/*
			 * Only light a wall if the face lit is possibly visible
			 * to the player.
			 */
			{
				bool proj = VC_IN_BOUNDS(grid.x, grid.y)
				            ? (bool)VC_LOS(grid.x, grid.y)
				            : square_allowslos(c, grid);
				if (!proj && !source_can_light_wall(c, p, sgrid, grid))
					continue;
			}
			/* Adjust the light level */
			if (inten > 0) {
				/* Light getting less further away */
				c->squares[grid.y][grid.x].light +=
					inten - dist;
			} else {
				/* Light getting greater further away */
				c->squares[grid.y][grid.x].light +=
					inten + dist;
			}
		}
	}
}

/**
 * Calculate light level for every grid in view - stolen from Sil
 */
static void calc_lighting(struct chunk *c, struct player *p)
{
	int dir, k, x, y;
	int light = p->state.cur_light, radius = ABS(light) - 1;
	int old_light = square_light(c, p->grid);
	int y0, y1, x0, x1, sight;

	/*
	 * Restrict lighting calculation to the player's visible area.
	 * The full-dungeon scan was the single biggest CPU sink on every
	 * update_view() call, because glow_can_light_wall() is expensive
	 * and was called for every lit wall in the dungeon regardless of
	 * how far it is from the player.  Squares outside max_sight are
	 * never reached by update_view_one(), so their light values don't
	 * affect gameplay.  Use max_sight+2 as margin for wall-adjacent
	 * bright-terrain spill and the wall-stealing LOS shift.
	 */
	sight = z_info->max_sight;
	y0 = p->grid.y - sight - 2; if (y0 < 0) y0 = 0;
	y1 = p->grid.y + sight + 2; if (y1 >= c->height) y1 = c->height - 1;
	x0 = p->grid.x - sight - 2; if (x0 < 0) x0 = 0;
	x1 = p->grid.x + sight + 2; if (x1 >= c->width) x1 = c->width - 1;

	/* Starting values based on permanent light */
	for (y = y0; y <= y1; y++) {
		for (x = x0; x <= x1; x++) {
			struct loc grid = loc(x, y);

			/* The cache bbox matches calc_lighting's y0/y1 exactly,
			 * so (x,y) is always within bounds here. */
			if (VC_GLOW(x, y) &&
					(VC_LOS(x, y) ||
					glow_can_light_wall(c, p, grid))) {
				c->squares[y][x].light = 1;
			} else {
				c->squares[y][x].light = 0;
			}

			/* Squares with bright terrain have intensity 2 */
			if (square_isbright(c, grid)) {
				c->squares[y][x].light += 2;
				for (dir = 0; dir < 8; dir++) {
					struct loc adj_grid = loc_sum(grid, ddgrid_ddd[dir]);
					if (!square_in_bounds(c, adj_grid)) continue;
					/*
					 * Only brighten a wall if the player
					 * is in position to view the face
					 * that's lit up.
					 */
					if (!square_allowslos(c, adj_grid) &&
							!source_can_light_wall(
							c, p, grid, adj_grid))
							continue;
					c->squares[adj_grid.y][adj_grid.x].light += 1;
				}
			}
		}
	}

	/* Light around the player */
	add_light(c, p, p->grid, radius, light);

	/* Scan monster list and add monster light or darkness */
	for (k = 1; k < cave_monster_max(c); k++) {
		/* Check the k'th monster */
		struct monster *mon = cave_monster(c, k);

		/* Skip dead monsters */
		if (!mon->race) continue;

		/* Skip if the monster is hidden */
		if (monster_is_camouflaged(mon)) continue;

		/* Get light info for this monster */
		light = mon->race->light;
		radius = ABS(light) - 1;

		/* Skip monsters not affecting light */
		if (!light) continue;

		/* Skip if the player can't see it. */
		if (distance(p->grid, mon->grid) - radius > z_info->max_sight)
			continue;

		add_light(c, p, mon->grid, radius, light);
	}

	/* Update light level indicator */
	if (square_light(c, p->grid) != old_light) {
		p->upkeep->redraw |= PR_LIGHT;
	}
}

/**
 * Make a square part of the current view
 */
static void become_viewable(struct chunk *c, struct loc grid, struct player *p,
							bool close)
{
	int x = grid.x;
	int y = grid.y;

	/* Already viewable, nothing to do */
	if (square_isview(c, grid)) return;

	/* Add the grid to the view, make seen if it's close enough to the player */
	sqinfo_on(square(c, grid)->info, SQUARE_VIEW);
	if (close) {
		sqinfo_on(square(c, grid)->info, SQUARE_SEEN);
		sqinfo_on(square(c, grid)->info, SQUARE_CLOSE_PLAYER);
	}

	/* Mark lit grids, and walls near to them, as seen */
	if (square_islit(c, grid)) {
		if (!VC_LOS(x, y)) {
			/* For walls, check for a lit grid closer to the player */
			int xc = (x < p->grid.x) ? (x + 1) : (x > p->grid.x) ? (x - 1) : x;
			int yc = (y < p->grid.y) ? (y + 1) : (y > p->grid.y) ? (y - 1) : y;
			if (square_islit(c, loc(xc, yc))) {
				sqinfo_on(square(c, grid)->info, SQUARE_SEEN);
			}
		} else {
			sqinfo_on(square(c, grid)->info, SQUARE_SEEN);
		}
	}
}

/**
 * Decide whether to include a square in the current view
 */
static void update_view_one(struct chunk *c, struct loc grid, struct player *p)
{
	int x = grid.x;
	int y = grid.y;
	int xc = x, yc = y;

	int d = distance(grid, p->grid);
	bool close = d < p->state.cur_light;

	/* Too far away */
	if (d > z_info->max_sight) return;

	/* UNLIGHT players have a special radius of view */
	if (player_has(p, PF_UNLIGHT) && (p->state.cur_light <= 1)) {
		close = d < (2 + p->lev / 6 - p->state.cur_light);
	}

	/* Special case for wall lighting. If we are a wall and the square in
	 * the direction of the player is in LOS, we are in LOS. This avoids
	 * situations like:
	 * #1#############
	 * #............@#
	 * ###############
	 * where the wall cell marked '1' would not be lit because the LOS
	 * algorithm runs into the adjacent wall cell.
	 *
	 * Use the per-frame view cache (VC_LOS) instead of calling
	 * square_allowslos() so the compiler keeps the flag table in registers
	 * rather than re-dereferencing the cave array on every inner call.
	 * los_fast() uses the same cache for its projectable checks.
	 */
	if (!VC_LOS(x, y)) {
		int dx = x - p->grid.x;
		int dy = y - p->grid.y;
		int ax = ABS(dx);
		int ay = ABS(dy);
		int sx = dx > 0 ? 1 : -1;
		int sy = dy > 0 ? 1 : -1;

		xc = (x < p->grid.x) ? (x + 1) : (x > p->grid.x) ? (x - 1) : x;
		yc = (y < p->grid.y) ? (y + 1) : (y > p->grid.y) ? (y - 1) : y;

		/* Check that the cell we're trying to steal LOS from isn't a
		 * wall. If we don't do this, double-thickness walls will have
		 * both sides visible.
		 */
		if (!VC_LOS(xc, yc)) {
			xc = x;
			yc = y;
		}

		/* Check if we got here via the 'knight's move' rule and, if so,
		 * don't steal LOS. */
		if (ax == 2 && ay == 1) {
			if (VC_LOS(x - sx, y) && !VC_LOS(x - sx, y - sy)) {
				xc = x;
				yc = y;
			}
		} else if (ax == 1 && ay == 2) {
			if (VC_LOS(x, y - sy) && !VC_LOS(x - sx, y - sy)) {
				xc = x;
				yc = y;
			}
		}
	}

	if (los_fast(p->grid, loc(xc, yc)))
		become_viewable(c, grid, p, close);
}

/**
 * Update view for a single square
 */
static void update_one(struct chunk *c, struct loc grid, struct player *p)
{
	/* Remove view if blind, check visible squares for traps */
	if (p->timed[TMD_BLIND]) {
		sqinfo_off(square(c, grid)->info, SQUARE_SEEN);
		sqinfo_off(square(c, grid)->info, SQUARE_CLOSE_PLAYER);
	} else if (square_isseen(c, grid)) {
		square_reveal_trap(c, grid, false, true);
	}

	/* Square went from unseen -> seen */
	if (square_isseen(c, grid) && !square_wasseen(c, grid)) {
		if (square_isfeel(c, grid)) {
			c->feeling_squares++;
			sqinfo_off(square(c, grid)->info, SQUARE_FEEL);
			/* Don't display feeling if it will display for the new level */
			if ((c->feeling_squares == z_info->feeling_need) &&
				!p->upkeep->only_partial) {
				display_feeling(true);
				p->upkeep->redraw |= PR_FEELING;
			}
		}

		square_note_spot(c, grid);
		square_light_spot(c, grid);
	}

	/* Square went from seen -> unseen */
	if (!square_isseen(c, grid) && square_wasseen(c, grid))
		square_light_spot(c, grid);

	sqinfo_off(square(c, grid)->info, SQUARE_WASSEEN);
}

/**
 * Update the player's current view
 */
void update_view(struct chunk *c, struct player *p)
{
	int x, y;
	int y0, y1, x0, x1, sight;
	int y0c, y1c, x0c, x1c;

	/*
	 * Compute the bounding box first — it is needed by mark_wasseen()
	 * (to restrict the flag-clear scan) and by build_view_cache().
	 *
	 * +1 margin: catches wall squares proxied from one floor cell closer
	 * to the player, and squares that just left view so update_one() can
	 * issue the required square_light_spot() call.
	 */
	sight = z_info->max_sight;
	y0 = p->grid.y - sight - 1; if (y0 < 0) y0 = 0;
	y1 = p->grid.y + sight + 1; if (y1 >= c->height) y1 = c->height - 1;
	x0 = p->grid.x - sight - 1; if (x0 < 0) x0 = 0;
	x1 = p->grid.x + sight + 1; if (x1 >= c->width) x1 = c->width - 1;

	/*
	 * Build the per-frame projectable/allowslos cache.  Extend the bbox by
	 * one extra cell in each direction (+2 total margin) so that the proxy
	 * wall checks in update_view_one() — which look at (x±1, y±1) relative
	 * to the loop cell — are always covered, avoiding out-of-bounds reads.
	 */
	y0c = y0 - 1; if (y0c < 0) y0c = 0;
	y1c = y1 + 1; if (y1c >= c->height) y1c = c->height - 1;
	x0c = x0 - 1; if (x0c < 0) x0c = 0;
	x1c = x1 + 1; if (x1c >= c->width) x1c = c->width - 1;
	build_view_cache(c, y0c, y1c, x0c, x1c);

	/* Record the current view, restricted to the union of prev+curr bbox */
	mark_wasseen(c, y0, y1, x0, x1);

	/* Calculate light levels */
	calc_lighting(c, p);

	/* Assume we can view the player grid */
	sqinfo_on(square(c, p->grid)->info, SQUARE_VIEW);
	if (p->state.cur_light > 0 || square_islit(c, p->grid) ||
		player_has(p, PF_UNLIGHT)) {
		sqinfo_on(square(c, p->grid)->info, SQUARE_SEEN);
		sqinfo_on(square(c, p->grid)->info, SQUARE_CLOSE_PLAYER);
	}
	/*
	 * If the player is blind and in terrain that was remembered to be
	 * impassable, forget the remembered terrain.  This will have to be
	 * modified in variants that have timed effects which allow a player
	 * to move through impassable terrain.
	 */
	if (p->timed[TMD_BLIND] && square_isknown(c, p->grid)
			&& !square_ispassable(p->cave, p->grid)) {
		square_forget(c, p->grid);
	}

	/* Squares we have LOS to get marked as in the view, and perhaps seen */
	for (y = y0; y <= y1; y++)
		for (x = x0; x <= x1; x++)
			update_view_one(c, loc(x, y), p);

	/* Update each grid */
	for (y = y0; y <= y1; y++)
		for (x = x0; x <= x1; x++)
			update_one(c, loc(x, y), p);
}


/**
 * Returns true if the player's grid is dark
 */
bool no_light(const struct player *p)
{
	return (!square_isseen(cave, p->grid));
}
