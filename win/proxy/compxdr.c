/* $Id$ */
/* Copyright (c) Slash'EM Development Team 2001-2002 */
/* NetHack may be freely redistributed.  See license for details. */

#include <stdlib.h>
#include "nhxdr.h"
#include "proxycom.h"

/*
 * This module implements the XDR routines for non-trivial compound types that
 * NhExt needs.
 */

nhext_xdr_bool_t proxy_xdr_init_nhwindow_req(xdr, datum)
NhExtXdr *xdr;
struct proxy_init_nhwindow_req *datum;
{
    return nhext_xdr_array(xdr, (char **)&datum->argv, 
      &datum->argc, (unsigned int)-1, sizeof(char *), nhext_xdr_wrapstring);
}

nhext_xdr_bool_t proxy_xdr_init_nhwindow_res(xdr, datum)
NhExtXdr *xdr;
struct proxy_init_nhwindow_res *datum;
{
    int retval = nhext_xdr_bool(xdr, &datum->inited);
    retval &= nhext_xdr_array(xdr, (char **)&datum->argv, 
      &datum->argc, (unsigned int)-1, sizeof(char *), nhext_xdr_wrapstring);
    return retval;
}

nhext_xdr_bool_t proxy_xdr_proxy_mi(xdrs, datum)
NhExtXdr *xdrs;
struct proxy_mi *datum;
{
    return nhext_xdr_int(xdrs, &datum->item) && nhext_xdr_long(xdrs, &datum->count);
}

nhext_xdr_bool_t proxy_xdr_select_menu_res(xdr, datum)
NhExtXdr *xdr;
struct proxy_select_menu_res *datum;
{
    int retval;
    retval = nhext_xdr_int(xdr, &datum->retval);
    retval &= nhext_xdr_array(xdr, (char **)&datum->selected, 
      &datum->n, 0xffffffff, sizeof(struct proxy_mi), proxy_xdr_proxy_mi);
    return retval;
}

nhext_xdr_bool_t proxy_xdr_status_req(xdr, datum)
NhExtXdr *xdr;
struct proxy_status_req *datum;
{
    return nhext_xdr_int(xdr, &datum->reconfig) &
      nhext_xdr_array(xdr, (char **)&datum->values, 
      &datum->nv, (unsigned int)-1, sizeof(char *), nhext_xdr_wrapstring);
}

nhext_xdr_bool_t proxy_xdr_glyph_row(xdr, datum)
NhExtXdr *xdr;
struct proxy_glyph_row *datum;
{
    return nhext_xdr_int(xdr, &datum->start) &
      nhext_xdr_array(xdr, (char **)&datum->glyphs, 
      &datum->ng, (unsigned int)-1, sizeof(int), nhext_xdr_int);
}

nhext_xdr_bool_t proxy_xdr_glyph_layer(xdr, datum)
NhExtXdr *xdr;
struct proxy_glyph_layer *datum;
{
    return nhext_xdr_int(xdr, &datum->start) &
      nhext_xdr_array(xdr, (char **)&datum->rows, &datum->nr,
      (unsigned int)-1, sizeof(struct proxy_glyph_row), proxy_xdr_glyph_row);
}

nhext_xdr_bool_t proxy_xdr_print_glyph_layered_req(xdr, datum)
NhExtXdr *xdr;
struct proxy_print_glyph_layered_req *datum;
{
    return nhext_xdr_int(xdr, &datum->window) &
      nhext_xdr_array(xdr, (char **)&datum->layers, &datum->nl,
      (unsigned int)-1, sizeof(struct proxy_glyph_layer),
      proxy_xdr_glyph_layer);
}

nhext_xdr_bool_t proxycb_xdr_get_player_choices_res_role(xdr, datum)
NhExtXdr *xdr;
struct proxycb_get_player_choices_res_role *datum;
{
    return nhext_xdr_wrapstring(xdr, (char **)&datum->male) &
      nhext_xdr_wrapstring(xdr, (char **)&datum->female);
}

nhext_xdr_bool_t proxycb_xdr_get_player_choices_res(xdr, datum)
NhExtXdr *xdr;
struct proxycb_get_player_choices_res *datum;
{
    return nhext_xdr_array(xdr, (char **)&datum->aligns, &datum->n_aligns,
        (unsigned int)-1, sizeof(char *), nhext_xdr_wrapstring) &
      nhext_xdr_array(xdr, (char **)&datum->genders, &datum->n_genders,
        (unsigned int)-1, sizeof(char *), nhext_xdr_wrapstring) &
      nhext_xdr_array(xdr, (char **)&datum->races, &datum->n_races,
        (unsigned int)-1, sizeof(char *), nhext_xdr_wrapstring) &
      nhext_xdr_array(xdr, (char **)&datum->roles, &datum->n_roles,
        (unsigned int)-1, sizeof(struct proxycb_get_player_choices_res_role),
	proxycb_xdr_get_player_choices_res_role);
}

nhext_xdr_bool_t proxycb_xdr_get_extended_commands_res(xdr, datum)
NhExtXdr *xdr;
struct proxycb_get_extended_commands_res *datum;
{
    return nhext_xdr_array(xdr, (char **)&datum->commands, &datum->n_commands,
        (unsigned int)-1, sizeof(char *), nhext_xdr_wrapstring);
}

nhext_xdr_bool_t proxycb_xdr_get_tilesets_res_tileset(xdr, datum)
NhExtXdr *xdr;
struct proxycb_get_tilesets_res_tileset *datum;
{
    return nhext_xdr_wrapstring(xdr, (char **)&datum->name) &
      nhext_xdr_wrapstring(xdr, (char **)&datum->file) &
      nhext_xdr_wrapstring(xdr, (char **)&datum->mapfile) &
      nhext_xdr_long(xdr, &datum->flags);
}

nhext_xdr_bool_t proxycb_xdr_get_tilesets_res(xdr, datum)
NhExtXdr *xdr;
struct proxycb_get_tilesets_res *datum;
{
    return nhext_xdr_array(xdr, (char **)&datum->tilesets, &datum->n_tilesets,
        (unsigned int)-1, sizeof(struct proxycb_get_tilesets_res_tileset),
	proxycb_xdr_get_tilesets_res_tileset);
}

nhext_xdr_bool_t proxycb_xdr_get_glyph_mapping_res_symdef(xdr, datum)
NhExtXdr *xdr;
struct proxycb_get_glyph_mapping_res_symdef *datum;
{
    return nhext_xdr_long(xdr, &datum->rgbsym) &
      nhext_xdr_wrapstring(xdr, (char **)&datum->description);
}

nhext_xdr_bool_t proxycb_xdr_get_glyph_mapping_res_submapping(xdr, datum)
NhExtXdr *xdr;
struct proxycb_get_glyph_mapping_res_submapping *datum;
{
    int start, retval;
    start = xdr->x_pos;
    retval = proxycb_xdr_get_glyph_mapping_res_symdef(xdr, &datum->symdef) &
      nhext_xdr_array(xdr, (char **)&datum->glyphs, &datum->n_glyphs,
        (unsigned int)-1, sizeof(struct proxycb_get_glyph_mapping_res_symdef),
	proxycb_xdr_get_glyph_mapping_res_symdef);
    return retval;
}

nhext_xdr_bool_t proxycb_xdr_get_glyph_mapping_res_mapping(xdr, datum)
NhExtXdr *xdr;
struct proxycb_get_glyph_mapping_res_mapping *datum;
{
    return nhext_xdr_wrapstring(xdr, (char **)&datum->flags) &
      nhext_xdr_int(xdr, &datum->base_mapping) &
      nhext_xdr_int(xdr, &datum->alt_glyph) &
      proxycb_xdr_get_glyph_mapping_res_symdef(xdr, &datum->symdef) &
      nhext_xdr_array(xdr, (char **)&datum->submappings, &datum->n_submappings,
        (unsigned int)-1,
	sizeof(struct proxycb_get_glyph_mapping_res_submapping),
	proxycb_xdr_get_glyph_mapping_res_submapping);
}

nhext_xdr_bool_t proxycb_xdr_get_glyph_mapping_res(xdr, datum)
NhExtXdr *xdr;
struct proxycb_get_glyph_mapping_res *datum;
{
    return nhext_xdr_int(xdr, &datum->no_glyph) &
      nhext_xdr_long(xdr, &datum->transparent) &
      nhext_xdr_array(xdr, (char **)&datum->mappings, &datum->n_mappings,
        (unsigned int)-1, sizeof(struct proxycb_get_glyph_mapping_res_mapping),
	proxycb_xdr_get_glyph_mapping_res_mapping);
}
