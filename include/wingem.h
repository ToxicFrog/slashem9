/*	SCCS Id: @(#)wingem.h	3.4	1999/12/10	*/
/* Copyright (c) Christian Bressler, 1999				  */
/* NetHack may be freely redistributed.  See license for details. */

#ifndef WINGEM_H
#define WINGEM_H

/* menu structure */
typedef struct Gmi{
	struct Gmi *Gmi_next;
	int Gmi_glyph;
	long Gmi_identifier;
	char Gmi_accelerator, Gmi_groupacc;
	int Gmi_attr;
	char *Gmi_str;
	long Gmi_count;
	int Gmi_selected;
} Gem_menu_item;

#define MAXWIN 20		/* maximum number of windows, cop-out */

extern struct window_procs Gem_procs;

/* ### wingem1.c ### */
#ifdef CLIPPING
extern void setclipped(void);
#endif
extern void FDECL(docorner, (int, int));
extern void end_glyphout(void);
extern void FDECL(g_putch, (int));
extern void win_Gem_init(void);
extern int mar_gem_init(void);
extern char mar_ask_class(void);
extern char * mar_ask_name(void);
extern int FDECL(mar_create_window, (int));
extern void FDECL(mar_destroy_nhwindow, (int));
extern void FDECL(mar_print_glyph, (int, int, int, int));
extern void FDECL(mar_print_line, (int, int, int, char *));
extern void FDECL(mar_set_message, (char *, char *, char *));
extern Gem_menu_item *mar_hol_inv(void);
extern void FDECL(mar_set_menu_type,(int));
extern void mar_reverse_menu(void);
extern void FDECL(mar_set_menu_title, (const char *));
extern void mar_set_accelerators(void);
extern void FDECL(mar_add_menu, (winid, Gem_menu_item *));
extern void FDECL(mar_change_menu_2_text, (winid));
extern void FDECL(mar_add_message, (const char *));
extern void mar_status_dirty(void);
extern int FDECL(mar_hol_win_type, (int));
extern void mar_clear_messagewin(void);
extern void FDECL(mar_set_no_glyph, (int));
extern void mar_map_curs_weiter(void);

/* external declarations */
extern void FDECL(Gem_init_nhwindows, (int *, char **));
extern void Gem_player_selection(void);
extern void Gem_askname(void);
extern void Gem_get_nh_event(void);
extern void FDECL(Gem_exit_nhwindows, (const char *));
extern void FDECL(Gem_suspend_nhwindows, (const char *));
extern void Gem_resume_nhwindows(void);
extern winid FDECL(Gem_create_nhwindow, (int));
extern void FDECL(Gem_clear_nhwindow, (winid));
extern void FDECL(Gem_display_nhwindow, (winid, BOOLEAN_P));
extern void FDECL(Gem_dismiss_nhwindow, (winid));
extern void FDECL(Gem_destroy_nhwindow, (winid));
extern void FDECL(Gem_curs, (winid,int,int));
extern void FDECL(Gem_putstr, (winid, int, const char *));
extern void FDECL(Gem_display_file, (const char *, BOOLEAN_P));
extern void FDECL(Gem_start_menu, (winid));
extern void FDECL(Gem_add_menu, (winid,int,const ANY_P *,
			CHAR_P,CHAR_P,int,const char *, BOOLEAN_P));
extern void FDECL(Gem_end_menu, (winid, const char *));
extern int FDECL(Gem_select_menu, (winid, int, MENU_ITEM_P **));
extern char FDECL(Gem_message_menu, (CHAR_P,int,const char *));
extern void Gem_update_inventory(void);
extern void Gem_mark_synch(void);
extern void Gem_wait_synch(void);
#ifdef CLIPPING
extern void FDECL(Gem_cliparound, (int, int));
#endif
#ifdef POSITIONBAR
extern void FDECL(Gem_update_positionbar, (char *));
#endif
extern void FDECL(Gem_print_glyph, (winid,XCHAR_P,XCHAR_P,int));
extern void FDECL(Gem_raw_print, (const char *));
extern void FDECL(Gem_raw_print_bold, (const char *));
extern int Gem_nhgetch(void);
extern int FDECL(Gem_nh_poskey, (int *, int *, int *));
extern void Gem_nhbell(void);
extern int Gem_doprev_message(void);
extern char FDECL(Gem_yn_function, (const char *, const char *, CHAR_P));
extern void FDECL(Gem_getlin, (const char *,char *));
extern int Gem_get_ext_cmd(void);
extern void FDECL(Gem_number_pad, (int));
extern void Gem_delay_output(void);
#ifdef CHANGE_COLOR
extern void FDECL(Gem_change_color,(int color,long rgb,int reverse));
extern char * Gem_get_color_string(void);
#endif

/* other defs that really should go away (they're tty specific) */
extern void Gem_start_screen(void);
extern void Gem_end_screen(void);

extern void FDECL(genl_outrip, (winid,int));

#endif /* WINGEM_H */
