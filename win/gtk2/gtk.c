/*
  $Id$
 */
/*
  GTK+ NetHack Copyright (c) Issei Numata 1999-2000
  GTK+ NetHack may be freely redistributed.  See license for details. 
*/

/* #define DEBUG */			/* Uncomment for debugging */

#include <sys/types.h>
#include <signal.h>
#include "winGTK.h"
#include "wintype.h"
#include "func_tab.h"
#include "dlb.h"
#ifdef SHORT_FILENAMES
#include "patchlev.h"
#else
#include "patchlevel.h"
#endif

static int	initialized;
static int	initialized2;
static int	in_topten;

static void	select_player(GtkWidget *w, guint data);
static void	key_command(GtkWidget *w, gpointer data);
static void	move_command(GtkWidget *w, gpointer data);
static void	fight_command(GtkWidget *w, gpointer data);
static void	ext_command(GtkWidget *w, gpointer data);
static void	game_option(GtkWidget *w, gpointer data);
static void	game_quit(GtkWidget *w, gpointer data);
static void	game_topten(GtkWidget *w, gpointer data);

static void	help_help(GtkWidget *w, gpointer data);
static void	help_shelp(GtkWidget *w, gpointer data);
static void	help_option(GtkWidget *w, gpointer data);
static void	help_je(GtkWidget *w, gpointer data);
static void	help_history(GtkWidget *w, gpointer data);
static void	help_license(GtkWidget *w, gpointer data);

#define GTK_NORTH	0
#define GTK_EAST	1
#define GTK_SOUTH	2
#define GTK_WEST	3
#define GTK_NORTHEAST	4
#define GTK_NORTHWEST	5
#define GTK_SOUTHEAST	6
#define GTK_SOUTHWEST	7

NHWindow gtkWindows[MAXWIN];

static winid		rawprint_win = WIN_ERR;

GtkAccelGroup *accel_group=NULL;

GtkWidget		*main_window;
#ifdef RADAR
static GtkWidget	*main_radar;
#endif

static GtkItemFactory	*main_item_factory;

int			root_width;
int			root_height;

GdkColor	  nh_color[N_NH_COLORS] = {
    {0, 0*257, 0*257, 0*257,},		/* black */
    {0, 255*257, 0*257, 0*257,},	/* red */
    {0, 152*257, 251*257, 152*257,},	/* pale green */
    {0, 165*257, 42*257, 42*257,},	/* brown */
    {0, 0*257, 0*257, 255*257,},	/* blue */
    {0, 255*257, 0*257, 255*257,}, 	/* magenta */
    {0, 224*257, 255*257, 255*257,}, 	/* light cyan */
    {0, 190*257, 190*257, 190*257,},	/* gray */
    {1, 0*257, 0*257, 0*257,},		/* default  */
    {0, 255*257, 165*257, 0*257,},	/* orange */
    {0, 0*257, 255*257, 0*257,},	/* green */
    {0, 255*257, 255*257, 0*257,}, 	/* yellow */
    {0, 65*257, 105*257, 225*257,},	/* royal blue */
    {0, 238*257, 130*257, 238*257,},	/* violet */
    {0, 0*257, 255*257, 255*257,},	/* cyan */
    {0, 255*257, 255*257, 255*257,},	/* white */
    {0, 0*257, 100*257, 0*257,},	/* dark green */
    {0, 20*257, 60*257, 20*257,},	/* map background */
};

void
hook()
{
    ;
}

#ifdef GTK_ASKNAME
void
gtk_askname() {
    int tryct = 0;
    static char who_are_you[] = "Who are you? ";
    
    do {
	if (tryct > 10) panic("Giving up after 10 tries.\n");
	else tryct++;
    	
	GTK_getlin(who_are_you, plname);
    } while (*plname == '\0');
}
#endif

struct window_procs GTK_procs = {
    "gtk",
    GTK_init_nhwindows,
    GTK_player_selection,
#ifdef WIN32
    gtk_askname, /* tty_askname,*/
#else
    hook, /* tty_askname,*/
#endif
    GTK_get_nh_event,
    GTK_exit_nhwindows,
    hook, /*tty_suspend_nhwindows,*/
    hook, /*tty_resume_nhwindows,*/
    GTK_create_nhwindow,
    GTK_clear_nhwindow,
    GTK_display_nhwindow,
    GTK_destroy_nhwindow,
    GTK_curs,
    GTK_putstr,
    GTK_display_file,
    GTK_start_menu,
    GTK_add_menu,
    GTK_end_menu,
    GTK_select_menu,
    genl_message_menu,
    GTK_update_inventory,
    GTK_mark_synch,
    GTK_wait_synch,
#ifdef CLIPPING
    GTK_cliparound,
#endif
#ifdef POSITIONBAR
    hook,
#endif
    GTK_print_glyph,
    GTK_raw_print,
    GTK_raw_print_bold,
    GTK_nhgetch,
    GTK_nh_poskey,
    hook, /* tty_nhbell,*/
    GTK_doprev_message,
    GTK_yn_function,
    GTK_getlin,
    GTK_get_ext_cmd,
    hook, /*tty_number_pad,*/
    GTK_delay_output,
#ifdef CHANGE_COLOR/* only a Mac option currently */
    hook,
    hook,
#endif
    /* other defs that really should go away (they're tty specific) */
    hook, /* tty_start_screen,*/
    hook, /* tty_end_screen,*/
#ifdef GRAPHIC_TOMBSTONE
    GTK_outrip,
#else
    genl_outrip,
#endif
};

static GtkItemFactoryEntry menu_template[] = {
    {"/Game",			NULL,		NULL,		0,	"<Branch>"},
    {"/Game/Gtear1",		NULL,		NULL,		0,	"<Tearoff>"},
    {"/Game/Play",		NULL,		NULL,		0,	"<Branch>"},
    /* Roles are inserted in the place of this NULL element */
    {NULL,			NULL,		NULL,		0,	NULL},
    {"/Game/Gsep1",		NULL,		NULL,		0,	"<Separator>"},
    {"/Game/Save",		"<shift>S",	key_command,	'S',	NULL},
    {"/Game/Option",		"<shift>O",	game_option,	'O',    NULL},
    {"/Game/Score",		NULL,		game_topten,	0,	NULL},
    {"/Game/Gsep2",		NULL,		NULL,		0,	"<Separator>"},
    {"/Game/Quit",		NULL,		game_quit,	0,	NULL},
};

static GtkItemFactoryEntry *menu_items;
static int nmenu_items;

static GtkItemFactoryEntry helpmenu_items[] = {
    {"/Help",			NULL,		NULL,		0,	"<LastBranch>"},
    {"/Help/Htear1",		NULL,		NULL,		0,	"<Tearoff>"},
    {"/Help/Command Help",	NULL,		help_help,	0,	NULL},
    {"/Help/Key Help",		NULL,		help_shelp,	0,	NULL},
    {"/Help/Option Help",	NULL,		help_option,	0,	NULL},
    {"/Help/sep3",		NULL,		NULL,		0,	"<Separator>"},
    {"/Help/History",		NULL,		help_history,	0,	NULL},
    {"/Help/License",		NULL,		help_license,	0,	NULL},
};

static GtkItemFactoryEntry playmenu_items[] = {
    {"/Move",			NULL,		NULL,		0,	"<Branch>"},
    {"/Move/Mtear1",		NULL,		NULL,		0,	"<Tearoff>"},
    {"/Move/North",		NULL,		move_command,	GTK_NORTH,	NULL},
    {"/Move/East",		NULL,		move_command,	GTK_EAST,	NULL},
    {"/Move/South",		NULL,		move_command,	GTK_SOUTH,	NULL},
    {"/Move/West",		NULL,		move_command,	GTK_WEST,	NULL},
    {"/Move/Northeast",		NULL,		move_command,	GTK_NORTHEAST,	NULL},
    {"/Move/Northwest",		NULL,		move_command,	GTK_NORTHWEST,	NULL},
    {"/Move/Southeast",		NULL,		move_command,	GTK_SOUTHEAST,	NULL},
    {"/Move/Southwest",		NULL,		move_command,	GTK_SOUTHWEST,	NULL},
    {"/Move/Down",		"greater",	key_command,	'>',	NULL},
    {"/Move/Up",		"less",		key_command,	'<',	NULL},
    {"/Fight",			NULL,		NULL,		0,	"<Branch>"},
    {"/Fight/Ftear1",		NULL,		NULL,		0,	"<Tearoff>"},
    {"/Fight/North",		NULL,		fight_command,	GTK_NORTH,	NULL},
    {"/Fight/East",		NULL,		fight_command,	GTK_EAST,	NULL},
    {"/Fight/South",		NULL,		fight_command,	GTK_SOUTH,	NULL},
    {"/Fight/West",		NULL,		fight_command,	GTK_WEST,	NULL},
    {"/Fight/Northeast",	NULL,		fight_command,	GTK_NORTHEAST,	NULL},
    {"/Fight/Northwest",	NULL,		fight_command,	GTK_NORTHWEST,	NULL},
    {"/Fight/Southeast",	NULL,		fight_command,	GTK_SOUTHEAST,	NULL},
    {"/Fight/Southwest",	NULL,		fight_command,	GTK_SOUTHWEST,	NULL},
    {"/Check",			NULL,		NULL,		0,	"<Branch>"},
    {"/Check/Ctear1",		NULL,		NULL,		0,	"<Tearoff>"},
    {"/Check/Here",		"colon",	key_command,	':',	NULL},
    {"/Check/There",		"semicolon",	key_command,	';',	NULL},
    {"/Check/Trap",		"asciicircum",	key_command,	'^',	NULL},
    {"/Equip",			NULL,		NULL,		0,	"<Branch>"},
    {"/Equip/Etear1",		NULL,		NULL,		0,	"<Tearoff>"},
    {"/Equip/Wield",		"w",		key_command,	'w',	NULL},	
    {"/Equip/Exchange weapons",	"x",		key_command,	'x',	NULL},	
    {"/Equip/Two-handed",	NULL,		ext_command,	EXT_CMD_TWOWEAPON,	NULL},	
    {"/Equip/Quiver",		"<shift>Q",	key_command,	'Q',	NULL},
    {"/Equip/Wear",		"<shift>w",	key_command,	'W',	NULL},	
    {"/Equip/Take off",		"<shift>T",	key_command,	'T',	NULL},	
    {"/Equip/Puton",		"<shift>p",	key_command,	'P',	NULL},	
    {"/Equip/Remove",		"<shift>r",	key_command,	'R',	NULL},	
    {"/You",			NULL,		NULL,		0,	"<Branch>"},
    {"/You/Ytear1",		NULL,		NULL,		0,	"<Tearoff>"},
    {"/You/Inventory",		"i",		key_command,	'i',	NULL},
    {"/You/Weapon",		"parenright",	key_command,	')',	NULL},	
    {"/You/Armor",		"bracketleft",	key_command,	'[',	NULL},	
    {"/You/Ring",		"equal",	key_command,	'=',	NULL},	
    {"/You/Amulet",		"quotedbl",	key_command,	'"',	NULL},	
    {"/You/Tool",		"parenleft",	key_command,	'(',	NULL},	
    {"/You/Spells",		"plus",		key_command,	'+',	NULL},	
    {"/You/Known Item",		"backslash",	key_command,	'\\',	NULL},	
    {"/You/Conduct",		NULL,		ext_command,	EXT_CMD_CONDUCT,	NULL},	
    {"/Adventure",		NULL,		NULL,		0,	"<Branch>"},
    {"/Adventure/ADtear1",	NULL,		NULL,		0,	"<Tearoff>"},
    {"/Adventure/Name",		"<alt>n",	key_command,	'n' | 0x80,	NULL},
    {"/Adventure/Call",		"<shift>C",	key_command,	'C',	NULL},
    {"/Adventure/Adjust",	"<alt>a",	key_command,	'a' | 0x80,	NULL},
    {"/Action",			NULL,		NULL,		0,	"<Branch>"},
    {"/Action/ACtear1",		NULL,		NULL,		0,	"<Tearoff>"},
    {"/Action/Rest",		"period",	key_command,	'.',	NULL},
    {"/Action/Search",		"s",		key_command,	's',	NULL},
    {"/Action/Eat",		"e",		key_command,	'e',	NULL},
    {"/Action/Asep1",		NULL,		NULL,		0,	"<Separator>"},
    {"/Action/Open",		"o",		key_command,	'o',	NULL},
    {"/Action/Close",		"c",		key_command,	'c',	NULL},
    {"/Action/Asep2",		NULL,		NULL,		0,	"<Separator>"},
    {"/Action/Pickup",		"comma",	key_command,	',',	NULL},
    {"/Action/Drop",		"d",		key_command,	'd',	NULL},
    {"/Action/Loot",		"<alt>l",	key_command,	'l' | 0x80, NULL},
    {"/Action/Apply",		"a",		key_command,	'a',	NULL},
    {"/Action/Kick",		"<control>D",	key_command,	'\04',	NULL},
    {"/Action/Throw",		"t",		key_command,	't',	NULL},
    {"/Action/Fire",		"f",		key_command,	'f',	NULL},
    {"/Action/Asep3",		NULL,		NULL,		0,	"<Separator>"},
    {"/Action/Drink",		"q",		key_command,	'q',	NULL},
    {"/Action/Read",		"r",		key_command,	'r',	NULL},
    {"/Action/Cast Spell",	"<shift>Z",	key_command,	'Z',	NULL},
    {"/Action/Zap",		"z",		key_command,	'z',	NULL},
    {"/Action/Dip",		"<alt>d",	key_command,	'd' | 0x80,	NULL},
    {"/Action/Sit",		"<alt>s",	key_command,	's' | 0x80,	NULL},
    {"/Religion",		NULL,		NULL,		0, 	"<Branch>"},
    {"/Religion/Rtear1",		NULL,		NULL,		0,	"<Tearoff>"},
    {"/Religion/Pray",		"<alt>p",	key_command,	'p' | 0x80,	NULL},
    {"/Religion/Offer",		"<alt>o",	key_command,	'o' | 0x80,	NULL},
    {"/Special",		NULL,		NULL,		0, 	"<Branch>"},
    {"/Special/Stear1",		NULL,		NULL,		0,	"<Tearoff>"},
    {"/Special/Engrave",	"<shift>E",	key_command,	'E', 		NULL},
    {"/Special/Pay",		"p",		key_command,	'p', 		NULL},
    {"/Special/Borrow",		"<alt>b",	key_command,	'b' | 0x80,	NULL},
    {"/Special/Chat",		"<alt>c",	key_command,	'c' | 0x80,	NULL},
    {"/Special/Enhance",	"<alt>e",	key_command,	'e' | 0x80,	NULL},
    {"/Special/Force",		"<alt>f",	key_command,	'f' | 0x80,	NULL},
    {"/Special/Invoke",		"<alt>i",	key_command,	'i' | 0x80,	NULL},
    {"/Special/Monster",	"<alt>m",	key_command,	'm' | 0x80,	NULL},
    {"/Special/Ride",		NULL,		ext_command,	EXT_CMD_RIDE,	NULL},
    {"/Special/Rub",		"<alt>r",	key_command,	'r' | 0x80,	NULL},
    {"/Special/Wipe",		"<alt>w",	key_command,	'w' | 0x80,	NULL},
    {"/Special/Technique",	"<alt>t",	key_command,	't' | 0x80,	NULL},
    {"/Special/Teleport",	"<control>t",	key_command,	'',	NULL},
    {"/Special/Turn",		NULL,		ext_command,	EXT_CMD_TURN,	NULL},
    {"/Special/Untrap",		"<alt>u",	key_command,	'u' | 0x80,	NULL},
    {"/Special/Jump",		"<alt>j",	key_command,	'j' | 0x80,	NULL},
};

static int keysym, keysym1;
static int pl_selection;

void
win_GTK_init()
{
    ;
}

/*
 * GTK_WINDOW_DIALOG is depreciated, and removed entirely in GTK+ 1.3.3
 */

GtkWidget *nh_gtk_window_dialog()
{
#if GTK_CHECK_VERSION(1,3,3)
    GtkWidget *w;
    w = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    if (w) {
	gtk_window_set_transient_for(GTK_WINDOW(w), GTK_WINDOW(main_window));
	gtk_window_set_modal(GTK_WINDOW(w), TRUE);
	gtk_window_set_destroy_with_parent(GTK_WINDOW(w), TRUE);
    }
    return w;
#else
    return gtk_window_new(GTK_WINDOW_DIALOG);
#endif
}

GtkWidget *
nh_gtk_new(GtkWidget *w, GtkWidget *parent, gchar *lbl)
{
#if 0
    /* [ALI] Removed this code, it doesn't seem to achive anything */
    gtk_widget_ref(w);
    gtk_object_set_data_full(
	GTK_OBJECT(parent), lbl, w,
	(GtkDestroyNotify)gtk_widget_unref);
#endif
    gtk_widget_show(w);

    return w;
}

GtkWidget *
nh_gtk_new_and_add(GtkWidget *w, GtkWidget *parent, gchar *lbl)
{
#if 0
    gtk_widget_ref(w);
    gtk_object_set_data_full(
	GTK_OBJECT(parent), lbl, w,
	(GtkDestroyNotify)gtk_widget_unref);
#endif
    gtk_widget_show(w);

    gtk_container_add(GTK_CONTAINER(parent), w);

    return w;
}

GtkWidget *
nh_gtk_new_and_pack(GtkWidget *w, GtkWidget *parent, gchar *lbl, 
		    gboolean a1, gboolean a2, guint a3)
{
#if 0
    gtk_widget_ref(w);
    gtk_object_set_data_full(
	GTK_OBJECT(parent), lbl, w,
	(GtkDestroyNotify)gtk_widget_unref);
#endif
    gtk_widget_show(w);

    gtk_box_pack_start(GTK_BOX(parent), w, a1, a2, a3);

    return w;
}

GtkWidget *
nh_gtk_new_and_attach(GtkWidget *w, GtkWidget *parent, gchar *lbl, 
		      guint a1, guint a2, guint a3, guint a4)
{
#if 0
    gtk_widget_ref(w);
    gtk_object_set_data_full(
	GTK_OBJECT(parent), lbl, w,
	(GtkDestroyNotify)gtk_widget_unref);
#endif
    gtk_widget_show(w);

    gtk_table_attach_defaults(GTK_TABLE(parent), w, a1, a2, a3, a4);

    return w;
}

GtkWidget *
nh_gtk_new_and_attach2(GtkWidget *w, GtkWidget *parent, gchar *lbl, 
		      guint a1, guint a2, guint a3, guint a4,
		      GtkAttachOptions a5,
		      GtkAttachOptions a6,
		      guint a7, guint  a8)
{
#if 0
    gtk_widget_ref(w);
    gtk_object_set_data_full(
	GTK_OBJECT(parent), lbl, w,
	(GtkDestroyNotify)gtk_widget_unref);
#endif
    gtk_widget_show(w);

    gtk_table_attach(GTK_TABLE(parent), w, a1, a2, a3, a4, a5, a6, a7, a8);

    return w;
}

int
nh_keysym(GdkEventKey *ev)
{
    int ret;

    switch(ev->keyval)
    {
	case GDK_Return:
	case GDK_KP_Enter:
	    ret = '\n';
	    break;
	case GDK_Escape:
	    ret = '\033';
	    break;
	default:
	    if (ev->length)
		ret = ev->string[0];
	    else
		ret = 0;
    }

    return ret;
}

static void
nh_menu_sensitive(char *menu, boolean f)
{
    GtkWidget *p;
    
    p = gtk_item_factory_get_widget(
	main_item_factory, menu);
    gtk_widget_set_sensitive(p, f);
}

/* ALI
 * We need to know the border width and height so that we can correctly
 * clamp dialogs to the screen. Unfortunately, GDK allows us no way to
 * dermine the border sizes before we map the window. We get around this
 * by remembering what they were for the last dialog displayed and
 * checking that they haven't changed after the window is mapped. If
 * we find we're offscreen after mapping we have to re-position the
 * window. Clumsy, but effective.
 *
 * Note: there's no way that I know of to determine the width of the
 * right border or the height of the bottom border. For now, we just
 * assume that these are both equal to the left border. Typically,
 * only the top border is different and this is a valid assumption.
 *
 * This is all further complicated by the fact that, at least under
 * the Enlightenment window manager, gtk_widget_set_uposition() sets
 * the position of the top left pixel of the border before the window
 * is mapped, but the top left pixel of the window after it is mapped.
 *
 * Note: If the window manager is set up such that gtk_widget_set_upostion()
 * always sets the position of the left left pixel of the window, and if
 * gtk_window_get_origin() returns this same position, then this code
 * will fail (gracefully) and dialogs may be positioned with their borders
 * just off-screen.
 */

static gint popup_dialog_bw = 0, popup_dialog_bh = 0;

static void
popup_dialog_mapped(GtkWidget *w, gpointer data)
{
    gint screen_width, screen_height;
    GdkWindow *window = GTK_WIDGET(w)->window;
    GTimer *t;
    gulong ms;
    int abandon = 0;
    gint x, y, nx, ny;
    data = gtk_object_get_user_data(GTK_OBJECT(w));
    if (data) {
	x = GPOINTER_TO_UINT(data) >> 16;
	y = GPOINTER_TO_UINT(data) & 0xffff;
	t = g_timer_new();
	if (!t)
	    return;
	/* Wait for Window Manager to place window and add borders */
	g_timer_start(t);
	do
	{
	    if (abandon) {
		g_timer_destroy(t);
		return;
	    }
	    (void)g_timer_elapsed(t, &ms);
	    if (ms >= 500)
		abandon++;	/* But only after one final check */
	    gdk_window_get_origin(window, &nx, &ny);
	} while (nx == 0 && ny == 0 || nx == x && ny == y);
	g_timer_destroy(t);
	popup_dialog_bw = nx - x;
	popup_dialog_bh = ny - y;
	/* Do we need to re-position this window to stay on-screen? */
	screen_width = gdk_screen_width();
	screen_height = gdk_screen_height();
	if (nx + popup_dialog_bw + w->allocation.width > screen_width)
	    x = screen_width - popup_dialog_bw - w->allocation.width;
	else x = nx;
	if (ny + popup_dialog_bw + w->allocation.height > screen_height)
	    /* popup_dialog_bw because we assume bottom border same as left */
	    y = screen_height - popup_dialog_bw - w->allocation.height;
	else y = ny;
	if (x != nx || y != ny)
	    gtk_widget_set_uposition(w, x, y);
    }
}

static void
popup_dialog_realizing(GtkWidget *w, gpointer data)
{
    GdkWindow *window = GTK_WIDGET(main_window)->window;
    gint x, y, width, height, ox, oy;
    gint screen_width = gdk_screen_width(), screen_height = gdk_screen_height();

    if (w->allocation.width > screen_width ||
      w->allocation.height > screen_height) {
	/* This is going to look bad anyway, we can't centre it because
	 * a position less than 0 is not valid, so the best we can do
	 * is place the pop-up at the top left of the screen and accept
	 * that the right and/or bottom edge will be off-screen.
	 */
	x = y = 0;
    }
    else {
	gdk_window_get_geometry(window, &x, &y, &width, &height, NULL);
	gdk_window_get_origin(window, &ox, &oy);
	x += ox + (width - w->allocation.width) / 2;
	y += oy + (height - w->allocation.height) / 2;
	if (x + w->allocation.width + popup_dialog_bw > screen_width)
	    x = screen_width - popup_dialog_bw - w->allocation.width;
	if (y + w->allocation.height + popup_dialog_bw > screen_height)
	    y = screen_height - popup_dialog_bw - w->allocation.height;
	/* (x,y) is the desired position of the window,
	 * adjust to take into account the expected border. */
	x -= popup_dialog_bw;
	y -= popup_dialog_bh;
	if (x < 0) x = 0;
	if (y < 0) y = 0;
    }
    gtk_widget_set_uposition(w, x, y);
    gtk_object_set_user_data(GTK_OBJECT(w), GUINT_TO_POINTER(x << 16 | y));
}

void
nh_position_popup_dialog(GtkWidget *w)
{
    if (GTK_WIDGET_REALIZED(w))
	popup_dialog_realizing(w, 0);
    if (GTK_WIDGET_MAPPED(w))
	popup_dialog_mapped(w, 0);
    gtk_signal_connect(GTK_OBJECT(w), "realize",
	GTK_SIGNAL_FUNC(popup_dialog_realizing), 0);
    gtk_signal_connect_after(GTK_OBJECT(w), "map",
	GTK_SIGNAL_FUNC(popup_dialog_mapped), 0);
}

void
quit_hook()
{
    gtk_main_quit();
}

void
main_hook()
{
    nh_map_check_visibility();
#ifdef RADAR
    if (!in_topten)
	nh_radar_update();
#endif

    gtk_main();
}

static void
game_option(GtkWidget *widget, gpointer data)
{
    nh_option_new();
    keysym = '\0';
}

static void
game_topten(GtkWidget *widget, gpointer data)
{
    winid id;
    char *argv[] = {
	"nethack",
	"-sall",
    };
    
    in_topten++;			/* Prevent radar window appearing */
    id = rawprint_win = create_toptenwin();
    prscore(2, argv);
    GTK_display_nhwindow(id, TRUE);
    GTK_destroy_nhwindow(id);
    dlb_init();				/* Re-initialise DLB */
    rawprint_win = WIN_ERR;
    keysym = '\0';
    in_topten--;
}

static void
help_license(GtkWidget *widget, gpointer data)
{
#ifndef FILE_AREAS    
    GTK_display_file(NH_LICENSE, TRUE);
#else
    GTK_display_file(NH_LICENSE_AREA, NH_LICENSE, TRUE);
#endif
    keysym = '\0';
}

static void
help_history(GtkWidget *widget, gpointer data)
{
    dohistory();
    keysym = '\0';
}


static void
help_option(GtkWidget *widget, gpointer data)
{
#ifndef FILE_AREAS    
    GTK_display_file(NH_OPTIONFILE, TRUE);
#else
    GTK_display_file(NH_OPTIONAREA, NH_OPTIONFILE, TRUE);
#endif
    keysym = '\0';
}

static void
help_shelp(GtkWidget *widget, gpointer data)
{
#ifndef FILE_AREAS    
    GTK_display_file(NH_SHELP, TRUE);
#else
    GTK_display_file(NH_SHELP_AREA, NH_SHELP, TRUE);
#endif
    keysym = '\0';
}

static void
help_help(GtkWidget *widget, gpointer data)
{
#ifndef FILE_AREAS    
    GTK_display_file(NH_HELP, TRUE);
#else
    GTK_display_file(NH_HELP_AREA, NH_HELP, TRUE);
#endif
    keysym = '\0';
}

static void
key_command(GtkWidget *widget, gpointer data)
{
    keysym = (int)data;

    quit_hook();
}

static const int dir_keys[8][2] = {
    	{'k','8'},	/* North */
    	{'l','6'},	/* East */
    	{'j','2'},	/* South */
    	{'h','4'},	/* West */
    	{'u','9'},	/* Northeast */
    	{'y','7'},	/* Northwest */
    	{'n','3'},	/* Southeast */
    	{'b','1'},	/* Southwest */
};

static void
move_command(GtkWidget *widget, gpointer data)
{

    keysym = 'm';
    keysym1 = (int) (iflags.num_pad ? 
    				dir_keys[(int)data][1] : 
    				dir_keys[(int)data][0]);

    quit_hook();
}

static void
fight_command(GtkWidget *widget, gpointer data)
{
    keysym = 'F';
    keysym1 = (int) (iflags.num_pad ? 
    				dir_keys[(int)data][1] : 
    				dir_keys[(int)data][0]);

    quit_hook();
}

static void
ext_command(GtkWidget *widget, gpointer data)
{
    keysym = '#';
    GTK_extcmd_set((int)data);

    quit_hook();
}

static void
quit()
{
    if(initialized2)
	done2();
    else{
	clearlocks();
	GTK_exit_nhwindows(NULL);
	terminate(0);
    }
}

static gint
main_window_destroy(GtkWidget *widget, gpointer data)
{
    quit();

    return TRUE;
}

static void
game_quit(GtkWidget *widget, gpointer data)
{
    quit();
}

static gint
default_destroy(GtkWidget *widget, gpointer data)
{
    GtkWindow **w = (GtkWindow **)data;
    *w = NULL;
    keysym = '\033';
    
    quit_hook();
    return FALSE;
}

int
nh_dir_keysym(GdkEventKey *ev)
{
    int ret;

    switch(ev->keyval)
    {
	case GDK_End:
	case GDK_KP_End:
	    ret = dir_keys[GTK_SOUTHWEST][!!iflags.num_pad];
	    break;
	case GDK_Down:
	case GDK_KP_Down:
	    ret = dir_keys[GTK_SOUTH][!!iflags.num_pad];
	    break;
	case GDK_Page_Down:
	case GDK_KP_Page_Down:
	    ret = dir_keys[GTK_SOUTHEAST][!!iflags.num_pad];
	    break;
	case GDK_Left:
	case GDK_KP_Left:
	    ret = dir_keys[GTK_WEST][!!iflags.num_pad];
	    break;
	case GDK_Begin:
	case GDK_KP_Begin:
	    ret = '.';
	    break;
	case GDK_Right:
	case GDK_KP_Right:
	    ret = dir_keys[GTK_EAST][!!iflags.num_pad];
	    break;
	case GDK_Home:
	case GDK_KP_Home:
	    ret = dir_keys[GTK_NORTHWEST][!!iflags.num_pad];
	    break;
	case GDK_Up:
	case GDK_KP_Up:
	    ret = dir_keys[GTK_NORTH][!!iflags.num_pad];
	    break;
	case GDK_Page_Up:
	case GDK_KP_Page_Up:
	    ret = dir_keys[GTK_NORTHEAST][!!iflags.num_pad];
	    break;
	default:
	    ret = 0;
    }

    return ret;
}

gint
GTK_default_key_press(GtkWidget *widget, GdkEventKey *event, gpointer data)
{
    keysym = nh_dir_keysym(event);
    if (!keysym)
	switch(event->keyval)
	{
	    case GDK_Insert:
	    case GDK_KP_Insert:
		keysym = 'i';
		break;
	    default:
		keysym = nh_keysym(event);
	}

    if(keysym)
	quit_hook();
    
    return FALSE;
}

static gint
default_button_press(GtkWidget *widget, GdkEventButton *event, gpointer data)
{
    if(data)
	keysym = (int)data;
    else
	keysym = '\n';

    quit_hook();

    return FALSE;
}

static gint
credit_expose_event(GtkWidget *widget, GdkEventExpose *event)
{
    gtk_main_quit();

    return FALSE;
}

static void
nh_rc(void)
{
    int i;
    char *rc_file;
    gchar **files, **new_files;
#ifndef FILE_AREAS
    rc_file = strdup("gtkrc");
#else
    rc_file = make_file_name(FILE_AREA_SHARE, "gtkrc");
#endif
    files = gtk_rc_get_default_files();
    for(i = 0; files[i]; i++)
	;
    new_files = g_new(gchar *, i + 2);
    if (!new_files)
	return;
    new_files[0] = (gchar *)rc_file;
    for(i = 0; files[i]; i++)
    {
	new_files[i + 1] = g_strdup(files[i]);
	if (!new_files[i + 1]) {
	    for(; i > 0; i--)
		g_free(new_files[i]);
	    g_free(new_files);
	    return;
	}
    }
    new_files[i + 1] = NULL;
    gtk_rc_set_default_files(new_files);
    for(i = 1; new_files[i]; i++)
	g_free(new_files[i]);
    g_free(new_files);
    free(rc_file);
}

/*
 * ALI
 *
 * Player selection code for 3.3 codebase.
 *
 * This code tries to keep to the spirit of the GTK interface by having
 * all possible selections available as menus for the user (rather than
 * bringing up a dialog). It also keeps the keyboard accelerators (one
 * per role), which are numbered from F1..F12, Shift-F1..Shift-F11 in
 * sequence. Shift-F12 is reserved for a random role.
 *
 * The interaction of the selection code and the flags.initfoo fields
 * (set for example with an OPTIONS=race:gnome line) is complex. These
 * values could either be treated as requirements (so that specifying
 * race:gnome would rule out any possibility of playing a Knight, for
 * example), or as cached answers to questions (so that the answer is
 * ignored until the question is asked and since role is queried before
 * race, the problem does not arise). The TTY interface follows the
 * latter interpretation, so we do too.
 *
 * For our purposes, displaying a menu is equivalent to asking a question
 * and the user choosing one the options (either by actually clicking
 * on an option or simply by moving the mouse) equates to giving an
 * answer. The joy of the GTK interface is that the user can un-answer
 * many questions by simply moving the mouse pointer back to the previous
 * menu.
 */

#define SELECT_KEY_ROLESHIFT	    20
#define SELECT_KEY_RACESHIFT	    8
#define SELECT_KEY_GENDSHIFT	    4
#define SELECT_KEY_ALIGNSHIFT	    0

#define SELECT_KEY_NUM(key, shift, mask) \
			((int)((key)>>(shift)&(mask)) - 1)

#define SELECT_KEY_ROLENUM(key)	    \
			SELECT_KEY_NUM(key, SELECT_KEY_ROLESHIFT, 0xfff)
#define SELECT_KEY_RACENUM(key)	    \
			SELECT_KEY_NUM(key, SELECT_KEY_RACESHIFT, 0xfff)
#define SELECT_KEY_GENDNUM(key)	    \
			SELECT_KEY_NUM(key, SELECT_KEY_GENDSHIFT, 0xf)
#define SELECT_KEY_ALIGNNUM(key)    \
			SELECT_KEY_NUM(key, SELECT_KEY_ALIGNSHIFT, 0xf)

/*
 * The call back routine. This extracts the relevant answers from the
 * menu option selected and arranges for a new game to be started.
 */

static void
select_player(GtkWidget *widget, guint data)
{
    flags.initrole = SELECT_KEY_ROLENUM(data);
    flags.initrace = SELECT_KEY_RACENUM(data);
    flags.initgend = SELECT_KEY_GENDNUM(data);
    flags.initalign = SELECT_KEY_ALIGNNUM(data);
    
    quit_hook();
}

/*
 * A node of the temporary tree which is used to generate the menu options.
 *
 * The key field contains the answers which the user would have given in
 * order to reach this point. Note that you need to know the level that the
 * node is in the tree to be able to distinguish an answer of 'random' from
 * `not yet asked'. This information is not stored in the tree itself.
 * Each son represents a possible answer to the next question. Again, you
 * need to know the level to know what the question is.
 *
 * Level	0	1	2		3
 * Question	Role?	Race?	Gender?		Alignment?
 *
 * Tree		root--->Arch--->human------+--->male---------+->lawful
 *                                         |                 \->neutral
 *                                         |                 \->random
 *                                         \--->female-------+->lawful
 *                                                           \...
 */

struct select_node {
    unsigned long key;
    int no_sons;
    struct select_node *sons;
};

/*
 * Return the <indx>th possible answer to the question at level <level>
 * given the answers already given in <key>. Note that indx==0 represents
 * the first possible answer.
 *
 * If indx is out of range, return _exactly_ -1.
 *
 * We use a cached answer if it is a valid answer. If not, we simply ignore it.
 *
 * Note that this function never lists random as a possible answer, the
 * caller must add this to the list if appropriate.
 */

static int
select_node_option(unsigned long key, int level, int indx)
{
    int rolenum, racenum, n, i, j;
    boolean (*valid)(int rolenum, int racenum, int alignnum);
    switch(level)
    {
	case 0:
	    /* Role */
	    if (flags.initrole >= 0)
		return indx ? -1 : flags.initrole;
	    for (i = 0; roles[i].name.m; i++)
		if (!indx--)
		    return i;
	    return -1;
	    break;
	case 1:
	    /* Race */
	    rolenum = SELECT_KEY_ROLENUM(key);
	    if (flags.initrace >= 0 &&
	      (rolenum < 0 || validrace(rolenum, flags.initrace)))
		return indx ? -1 : flags.initrace;
	    for (i = 0; races[i].noun; i++)
		if (rolenum < 0 || validrace(rolenum, i))
		    if (!indx--)
			return i;
	    return -1;
	    break;
	case 3:
	    /* Alignmnent */
	    /* FALL THROUGH */
	case 2:
	    /* Gender */
	    if (level == 2)
	    {
		n = ROLE_GENDERS;
		valid = validgend;
		i = flags.initgend;
	    }
	    else
	    {
		n = ROLE_ALIGNS;
		valid = validalign;
		i = flags.initalign;
	    }
	    rolenum = SELECT_KEY_ROLENUM(key);
	    racenum = SELECT_KEY_RACENUM(key);
	    if (i >= 0)
	    {
		if (rolenum < 0)
		    if (racenum < 0)
			return indx ? -1 : i;
		    else
		    {
			for (j = 0; roles[j].name.m; j++)
			    if (valid(j, racenum, i))
				return indx ? -1 : i;
		    }
		else if (racenum < 0)
		{
		    for (j = 0; races[j].noun; j++)
			if (valid(rolenum, j, i))
			    return indx ? -1 : i;
		}
		else if (valid(rolenum, racenum, i))
		    return indx ? -1 : i;
	    }
	    if (rolenum < 0)
		if (racenum < 0)
		{
		    if (indx >= 0 && indx < n)
			return indx;
		}
		else
		{
		    for (i = 0; i < n; i++)
			for (j = 0; roles[j].name.m; j++)
			    if (valid(j, racenum, i))
			    {
				if (!indx--)
				    return i;
				break;
			    }
		}
	    else if (racenum < 0)
	    {
		for (i = 0; i < n; i++)
		    for (j = 0; races[j].noun; j++)
			if (valid(rolenum, j, i))
			{
			    if (!indx--)
				return i;
			    break;
			}
	    }
	    else
		for (i = 0; i < n; i++)
		    if (valid(rolenum, racenum, i))
			if (!indx--)
			    return i;
	    return -1;
	    break;
    }
}

/*
 * Fill a tree starting at <node> which is in level <level>.
 * Return the number of menu items (including titles and seperators)
 * that this tree represents.
 */

static int
select_node_fill(struct select_node *node, int level)
{
    int shift, no_opts, count, option, i;
    for (no_opts = 0; ; no_opts++)
	if (select_node_option(node->key, level, no_opts) < 0)
	    break;
    if (level == 0)
	shift = SELECT_KEY_ROLESHIFT;
    else if (level == 1)
	shift = SELECT_KEY_RACESHIFT;
    else if (level == 2)
	shift = SELECT_KEY_GENDSHIFT;
    else
	shift = SELECT_KEY_ALIGNSHIFT;
    if (no_opts > 1)
    {
	node->no_sons = no_opts + 1;
	count = no_opts + 2;
    }
    else
    {
	node->no_sons = no_opts;
	count = no_opts;
    }
    if (!level)
	count++;
    node->sons = (struct select_node *)alloc(node->no_sons *
      sizeof(struct select_node));
    for (i = 0; i < node->no_sons; i++)
    {
	option = select_node_option(node->key, level, i);
	node->sons[i].key = node->key | (option + 1 << shift);
	if (level<3)
	    count += select_node_fill(node->sons + i, level + 1);
	else
	{
	    node->sons[i].no_sons = 0;
	    node->sons[i].sons = NULL;
	}
    }
    return count;
}

/*
 * Return a menu item path representing the answers given to <level>
 * questions stored in <key>. If <leaf> is not NULL, append this as
 * a leaf to the path.
 */

static char *
select_node_path(unsigned long key, int level, char *leaf)
{
    int rolenum, racenum, gendnum, alignnum, len, i;
    char *path;

    rolenum = SELECT_KEY_ROLENUM(key);
    racenum = SELECT_KEY_RACENUM(key);
    gendnum = SELECT_KEY_GENDNUM(key);
    alignnum = SELECT_KEY_ALIGNNUM(key);
    len = 11 + level;
    if (level > 0)
	if (rolenum >= 0)
	    len += strlen(roles[rolenum].name.m);
	else
	    len += 6;
    if (level > 1)
	if (racenum >= 0)
	    len += strlen(races[racenum].noun);
	else
	    len += 6;
    if (level > 2)
	if (gendnum >= 0)
	    len += strlen(genders[gendnum].adj);
	else
	    len += 6;
    if (level > 3)
	if (alignnum >= 0)
	    len += strlen(aligns[alignnum].adj);
	else
	    len += 6;
    if (leaf)
	len += 1 + strlen(leaf);
    path = (gchar *) alloc(len);
    strcpy(path, "/Game/Play");
#define SELECT_STR(num, str) (((num) >= 0) ? (str) : "Random")
    if (level > 0)
    {
	strcat(path, "/");
	strcat(path, SELECT_STR(rolenum, roles[rolenum].name.m));
    }
    if (level > 1)
    {
	strcat(path, "/");
	strcat(path, SELECT_STR(racenum, races[racenum].noun));
    }
    if (level > 2)
    {
	strcat(path, "/");
	strcat(path, SELECT_STR(gendnum, genders[gendnum].adj));
    }
    if (level > 3)
    {
	strcat(path, "/");
	strcat(path, SELECT_STR(alignnum, aligns[alignnum].adj));
    }
    if (leaf)
    {
	strcat(path, "/");
	strcat(path, leaf);
    }
#undef SELECT_STR
    return path;
}

/*
 * Return a suitable keyboard accelrator to go with this menu item.
 * For most menu items this will be NULL, but we allocate one menu
 * item from each role to a function key. This allows the user to
 * press a function key to start the game in that role. The race,
 * gender and alignment of the character will either be as required
 * by the role, as specified in the flags.initfoo fields or randomly
 * chosen.
 */

static char *
select_node_accel(unsigned long key)
{
    int rolenum;
    char *accel;

    rolenum = SELECT_KEY_ROLENUM(key);
    if (rolenum >= 23)
	return NULL;
    if (SELECT_KEY_RACENUM(key) >= 0 && select_node_option(key, 1, 1) >= 0)
	return NULL;
    if (SELECT_KEY_GENDNUM(key) >= 0 && select_node_option(key, 2, 1) >= 0)
	return NULL;
    if (SELECT_KEY_ALIGNNUM(key) >= 0 && select_node_option(key, 3, 1) >= 0)
	return NULL;
    if (rolenum < 0)
    {
	accel = (gchar *) alloc(11);
	sprintf(accel, "<shift>F12");
    }
    else if (rolenum < 12)
    {
	accel = (gchar *) alloc(rolenum > 9 ? 4 : 3);
	sprintf(accel, "F%d", rolenum + 1);
    }
    else
    {
	accel = (gchar *) alloc(rolenum > 20 ? 11 : 10);
	sprintf(accel, "<shift>F%d", rolenum - 11);
    }
    return accel;
}

/*
 * Walk the tree, generating menu items as we go.
 */

static int
select_node_traverse(struct select_node *node, int offset, int level)
{
    int i;
    char *titles[] = { "Role", "Race", "Gender", "Alignment" };
    if (node->no_sons)
    {
	menu_items[offset].path =
	  select_node_path(node->key, level, titles[level]);
	menu_items[offset].accelerator = NULL;
	menu_items[offset].callback = NULL;
	menu_items[offset].callback_action = 0;
	menu_items[offset++].item_type = "<Title>";
	for (i = 0; i < node->no_sons; i++)
	{
	    if (node->sons[i].key == node->key)
	    {
		menu_items[offset].path =
		  select_node_path(node->key, level, "GPSepR");
		menu_items[offset].accelerator = NULL;
		menu_items[offset].callback = NULL;
		menu_items[offset].callback_action = 0;
		menu_items[offset++].item_type = "<Separator>";
	    }
	    offset = select_node_traverse(node->sons + i, offset, level + 1);
	}
    }
    else
    {
	menu_items[offset].path = select_node_path(node->key, level, NULL);
	menu_items[offset].accelerator = select_node_accel(node->key);
	menu_items[offset].callback = select_player;
	menu_items[offset].callback_action = node->key;
	menu_items[offset++].item_type = NULL;
    }
    return offset;
}

static void
select_node_free(struct select_node *node)
{
    int i;
    for (i = 0; i < node->no_sons; i++)
	select_node_free(node->sons + i);
    free(node->sons);
}

#ifdef DEBUG
static void
select_node_dump(struct select_node *node, int level)
{
    int i;
    static int count;
    char buf[BUFSZ];
    char *titles[] = { "Role", "Race", "Gender", "Alignment" };

    if (level) {
	for(i = 0; i < level; i++)
	    fputs("    ", stderr);
	sprintf(buf, "[%d] 0x%lX: ", count++, node->key);
	switch (level)
	{
	case 1:
	    if (SELECT_KEY_ROLENUM(node->key) >= 0)
		fprintf(stderr, "%s%s", buf,
		  roles[SELECT_KEY_ROLENUM(node->key)].name.m);
	    else {
		fprintf(stderr, "[%d] ---\n", count - 1);
		for(i = 0; i < level; i++)
		    fputs("    ", stderr);
		fprintf(stderr, "[%d] 0x%lX: random", count++, node->key);
	    }
	    break;
	case 2:
	    if (SELECT_KEY_RACENUM(node->key) >= 0)
		fprintf(stderr, "%s%s", buf,
		  races[SELECT_KEY_RACENUM(node->key)].noun);
	    else {
		fprintf(stderr, "[%d] ---\n", count - 1);
		for(i = 0; i < level; i++)
		    fputs("    ", stderr);
		fprintf(stderr, "[%d] 0x%lX: random", count++, node->key);
	    }
	    break;
	case 3:
	    if (SELECT_KEY_GENDNUM(node->key) >= 0)
		fprintf(stderr, "%s%s", buf,
		  genders[SELECT_KEY_GENDNUM(node->key)].adj);
	    else {
		fprintf(stderr, "[%d] ---\n", count - 1);
		for(i = 0; i < level; i++)
		    fputs("    ", stderr);
		fprintf(stderr, "[%d] 0x%lX: random", count++, node->key);
	    }
	    break;
	case 4:
	    if (SELECT_KEY_ALIGNNUM(node->key) >= 0)
		fprintf(stderr, "%s%s", buf,
		  aligns[SELECT_KEY_ALIGNNUM(node->key)].adj);
	    else {
		fprintf(stderr, "[%d] ---\n", count - 1);
		for(i = 0; i < level; i++)
		    fputs("    ", stderr);
		fprintf(stderr, "[%d] 0x%lX: random", count++, node->key);
	    }
	    break;
	}
	fputc('\n', stderr);
    }
    else
	count = 0;
    if (node->no_sons) {
	for(i = 0; i < level; i++)
	    fputs("    ", stderr);
	if (level)
	    fprintf(stderr, "%s\n", titles[level]);
	else
	    fprintf(stderr, "[%d] %s\n", count++, titles[0]);
	for(i = 0; i < node->no_sons; i++)
	    select_node_dump(node->sons + i, level + 1);
    }
}
#endif

/*
 * Initialise the player selection code by creating a temporary tree
 * of all the possible options, using it to generate a set of menu
 * items that can be passed to GTK and finally freeing the tree.
 */

static void
init_select_player(boolean init)
{
    int num_opts, i;
    struct select_node *root;
    static int no_dynamic_opts, first_dynamic_opt;

    if (!init) {	/* Exit */
	for (i = 0; i < no_dynamic_opts; i++) {
	    free(menu_items[first_dynamic_opt + i].path);
	    free(menu_items[first_dynamic_opt + i].accelerator);
	}
	free(menu_items);
	return;
    }
    root = (struct select_node *)alloc(sizeof(struct select_node));
    root->key = 0;
    num_opts = select_node_fill(root, 0);
#ifdef DEBUG
    select_node_dump(root, 0);
#endif
    menu_items = (GtkItemFactoryEntry *)alloc(sizeof(GtkItemFactoryEntry) *
      (num_opts + SIZE(menu_template) - 1));
    nmenu_items = 0;
    for (i = 0; i < SIZE(menu_template); i++) {
	if (menu_template[i].path)
	    menu_items[nmenu_items++] = menu_template[i];
	else {
	    first_dynamic_opt = nmenu_items;
	    nmenu_items = select_node_traverse(root, nmenu_items, 0);
	    no_dynamic_opts = nmenu_items - first_dynamic_opt;
	}
    }
    if (nmenu_items > num_opts + SIZE(menu_template) - 1)
	panic("GTK: init_select_player: Too many options (%d instead of %d)",
	  nmenu_items, num_opts + SIZE(menu_template) - 1);
#ifdef DEBUG
    else if (nmenu_items < num_opts + SIZE(menu_template) - 1) {
	impossible("GTK: init_select_player: Too few options (%d instead of %d)",
	  nmenu_items, num_opts + SIZE(menu_template) - 1);
	for(i = 0; i < nmenu_items; i++) {
	    fprintf(stderr, "[%d] %s\n", i, menu_items[i].path);
	}
    }
#endif
    select_node_free(root);
    free(root);
}

void
GTK_init_nhwindows(int *argc, char **argv)
{
    char *credit_file;
    int i;
    GtkWidget 	*main_bar, 
		*credit_window, *credit_vbox, *credit_credit,
		*main_vbox, *main_hbox;
    GtkStyle	*credit_style;
    GdkPixmap	*credit_pixmap;
    GdkBitmap	*credit_mask;

#ifdef UNIX
    uid_t savuid;
#endif
    if(initialized2)
	goto selection;

    gtk_set_locale();
    nh_rc();

    /* Init windows to nothing. */
    for (i = 0; i < MAXWIN; i++)
	gtkWindows[i].type = NHW_NONE;

    /*
     * setuid hack: make sure that if nethack is setuid, to use real uid
     * when opening X11 connections, in case the user is using xauth, since
     * the "games" or whatever user probably doesn't have permission to open
     * a window on the user's display.  This code is harmless if the binary
     * is not installed setuid.  See include/system.h on compilation failures.
     */
#ifdef UNIX
    savuid = geteuid();
    (void) seteuid(getuid());
    hide_privileges(TRUE);
#endif
    
    gtk_init(argc, &argv);

#ifdef UNIX
    hide_privileges(FALSE);
    (void) seteuid(savuid);
#endif
    
    init_select_player(TRUE);

/*
  creat credit widget and show
*/
    credit_window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_position(GTK_WINDOW(credit_window), GTK_WIN_POS_CENTER);

    gtk_container_border_width(GTK_CONTAINER(credit_window), 2);
/*
  gtk_signal_connect(GTK_OBJECT(credit_window), "check_resize",
  GTK_SIGNAL_FUNC(credit_map_event), NULL);
*/

    gtk_signal_connect(GTK_OBJECT(credit_window), "expose_event",
		       GTK_SIGNAL_FUNC(credit_expose_event), NULL);

    gtk_widget_realize(credit_window);
    root_width = gdk_screen_width();
    root_height = gdk_screen_height();

    credit_vbox = nh_gtk_new_and_add(
	gtk_vbox_new(FALSE, 0), credit_window, "");
     
    credit_style = gtk_widget_get_style(credit_window);
#ifndef FILE_AREAS
    credit_file = "credit.xpm";
#else
    credit_file = make_file_name(FILE_AREA_SHARE, "credit.xpm");
#endif
    credit_pixmap = gdk_pixmap_create_from_xpm(
	credit_window->window,
	&credit_mask,
	&credit_style->bg[GTK_STATE_NORMAL],
	credit_file);
#ifdef FILE_AREAS
    free(credit_file);
#endif
    if (credit_pixmap) {
	credit_credit = nh_gtk_new_and_pack(
	    gtk_pixmap_new(credit_pixmap, credit_mask), credit_vbox, "",
	    FALSE, FALSE, NH_PAD);
	gdk_pixmap_unref(credit_pixmap);
    }
    gtk_widget_show_all(credit_window);

    gtk_main();

/*
  create main widget
*/
    main_window = gtk_window_new(GTK_WINDOW_TOPLEVEL);

    gtk_window_set_policy(GTK_WINDOW(main_window), TRUE, TRUE, TRUE);

    gtk_signal_connect(
	GTK_OBJECT(main_window), "delete_event",
	GTK_SIGNAL_FUNC(main_window_destroy), 0);

    gtk_window_set_title(GTK_WINDOW(main_window), DEF_GAME_NAME);
    
    gtk_widget_set_events(main_window, GDK_KEY_PRESS_MASK);
    gtk_widget_realize(main_window);

/*
  allocate color
 */
    {
	int 		i;
	GdkColormap	*cmap;

	cmap = gdk_window_get_colormap(main_window->window);
  
	for(i=0 ; i < N_NH_COLORS ; ++i){
	    if(0 && nh_color[i].pixel){
		nh_color[i] = *(main_window->style->fg);
	    }
	    else if(gdk_colormap_alloc_color(cmap, &nh_color[i], FALSE, TRUE) == TRUE){
		;
	    }
	    else{
		fprintf(stderr, "cannot allocate color\n");
	    }
	}
    }

    main_vbox = nh_gtk_new_and_add(gtk_vbox_new(FALSE, 0), main_window, "");

    {
	int nplaymenu_items = sizeof(playmenu_items) / sizeof(playmenu_items[0]);
	int nhelpmenu_items = sizeof(helpmenu_items) / sizeof(helpmenu_items[0]);

	accel_group = gtk_accel_group_new();

	main_item_factory = gtk_item_factory_new(
	    GTK_TYPE_MENU_BAR, "<main>",
	    accel_group);

	gtk_item_factory_create_items(
	    main_item_factory,
	    nmenu_items, menu_items,
	    NULL);

	gtk_item_factory_create_items(
	    main_item_factory,
	    nplaymenu_items, playmenu_items,
	    NULL);

	gtk_item_factory_create_items(
	    main_item_factory,
	    nhelpmenu_items, helpmenu_items,
	    NULL);


	gtk_accel_group_attach(accel_group, GTK_OBJECT(main_window));
    }

    main_bar = nh_gtk_new_and_pack(
	gtk_item_factory_get_widget(main_item_factory, "<main>"), main_vbox, "",
	FALSE, FALSE, 0);
    nh_menu_sensitive("/Game/Save", FALSE);
/*
  nh_menu_sensitive("/Game/Option", FALSE);
*/
    nh_menu_sensitive("/Move", FALSE);
    nh_menu_sensitive("/Fight", FALSE);
    nh_menu_sensitive("/Check", FALSE);
    nh_menu_sensitive("/Equip", FALSE);
    nh_menu_sensitive("/You", FALSE);
    nh_menu_sensitive("/Adventure", FALSE);
    nh_menu_sensitive("/Action", FALSE);
    nh_menu_sensitive("/Religion", FALSE);
    nh_menu_sensitive("/Special", FALSE);

    main_hbox = nh_gtk_new_and_pack(
	gtk_hbox_new(FALSE, 1), main_vbox, "",
	FALSE, FALSE, 0);
	
    (void) nh_gtk_new_and_pack(nh_message_new(), main_hbox, "",
	TRUE, TRUE, 0);
    (void) nh_gtk_new_and_pack(nh_status_new(), main_hbox, "",
	FALSE, FALSE, 0);

#ifdef RADAR
    main_radar = nh_radar_new();
#endif
    
    (void) nh_gtk_new_and_pack(
	nh_map_new(main_window), main_vbox, "",
	TRUE, TRUE, 0);

    /*
     * Initialize standard windows. It used to be the case that window type
     * and id were equivalent (and therefore there could only be one type
     * of each window). We maintain this correlation for the standard
     * windows for compatibility -ALI
     */
    gtkWindows[NHW_MESSAGE].type = NHW_MESSAGE;
    gtkWindows[NHW_MESSAGE].w = main_window;	
    gtkWindows[NHW_STATUS].type = NHW_STATUS;
    gtkWindows[NHW_STATUS].w = main_window;	
    gtkWindows[NHW_MAP].type = NHW_MAP;
    gtkWindows[NHW_MAP].w = main_window;	

 selection:
    initialized = 1;

    gtk_widget_hide(credit_window);
    gtk_widget_destroy(credit_window);
    gtk_widget_show_all(main_window);
    
    iflags.window_inited = 1;
}

#ifdef MONITOR_XRESOURCES
static int GTK_X11_error_handler(Display *dpy, XErrorEvent *e)
{
    return 0;
}
#endif

void
GTK_exit_nhwindows(const char *str)
{
    int id;

    if(str && *str){
	id = GTK_create_nhwindow(NHW_MENU);
	GTK_putstr(id, 0, str);
	GTK_display_nhwindow(id, TRUE);
	GTK_destroy_nhwindow(id);
    }

    init_select_player(FALSE);

    for(id = 0; id < MAXWIN; id++)
	if (gtkWindows[id].type != NHW_NONE)
	    GTK_destroy_nhwindow(id);

    nh_map_destroy();
    nh_status_destroy();

#ifdef RADAR
    nh_radar_destroy();
#endif
    gtk_widget_destroy(main_window);
    g_blow_chunks();

#ifdef MONITOR_XRESOURCES
    /* ALI: An exceedingly unportable way of checking that
     * there are no X drawables still allocated at this point.
     */
    {
	struct _XDisplay *dpy = (struct _XDisplay *)GDK_DISPLAY();
	XID i;
	Drawable d;
	Window root;
	int x, y;
	unsigned int w, h, bw, dp;
	XWindowAttributes wa;
	char *type;
	fprintf(stderr, "X Drawables not freed:\n");
	fprintf(stderr, "XID       Type    Width   Height\n");
	XSetErrorHandler(GTK_X11_error_handler);
	for(i = 0; i < dpy->resource_id; i++) {
	    d = (Drawable)(dpy->resource_base + (i << dpy->resource_shift));
	    if (XGetGeometry(dpy, d, &root, &x, &y, &w, &h, &bw, &dp)) {
		if (x || y || bw)
		    type = "Window";
		else if (XGetWindowAttributes(dpy, (Window)d, &wa))
		    type = wa.screen ? "Window" : "Pixmap";
		else
		    type = "Pixmap";
		fprintf(stderr, "%-10lX%-8s%-8u%u\n",
		  (unsigned long)d, type, w, h);
	    }
	}
    }
#endif
}

void
GTK_init_nhwindows2()
{
    if(initialized2)
	return;
    nh_menu_sensitive("/Game/Play", FALSE);
    nh_menu_sensitive("/Game/Save", TRUE);
/*
    nh_menu_sensitive("/Game/Option", TRUE);
*/
    nh_menu_sensitive("/Game/Save", TRUE);
    nh_menu_sensitive("/Game/Option", TRUE);
    nh_menu_sensitive("/Move", TRUE);
    nh_menu_sensitive("/Fight", TRUE);
    nh_menu_sensitive("/Check", TRUE);
    nh_menu_sensitive("/Equip", TRUE);
    nh_menu_sensitive("/You", TRUE);
    nh_menu_sensitive("/Adventure", TRUE);
    nh_menu_sensitive("/Action", TRUE);
    nh_menu_sensitive("/Religion", TRUE);
    nh_menu_sensitive("/Special", TRUE);

    nh_option_lock();

    gtk_signal_connect(
	GTK_OBJECT(main_window), "key_press_event",
	GTK_SIGNAL_FUNC(GTK_default_key_press), NULL);

#ifdef RADAR
    gtk_signal_connect(
	GTK_OBJECT(main_radar), "key_press_event",
	GTK_SIGNAL_FUNC(GTK_default_key_press), NULL);
#endif

    initialized2 = 1;
    GTK_update_inventory();
}

winid
GTK_create_nhwindow(int type)
{
    winid	id;
    NHWindow	*w;

    switch(type){
/* 
   these windows have already been created
*/
    case NHW_MESSAGE:
    case NHW_STATUS:
    case NHW_MAP:
	if (gtkWindows[type].type != type)
	    panic("GTK_create_nhwindow: standard window (%d) not valid",type);
	return type;
	break;
/*
  create new window
*/
    case NHW_MENU:
    case NHW_TEXT:
	for (id = 0; id < MAXWIN; id++) {
	    w = &gtkWindows[id];
	    if (w->type == NHW_NONE)
	    {
		memset(w, 0, sizeof(NHWindow));
		w->type = type;
		if (type == NHW_MENU)
		    GTK_create_menu_window(w);
		return id;
	    }
	}
	panic("GTK_create_nhwindow: no free windows!");
	break;
    default:
	panic("GTK_create_nhwindow: Unknown type (%d)!", type);
    }
    return 0;
}

void
GTK_destroy_nhwindow(winid id)
{
/*    int i;*/
    NHWindow *w;

    if(id == NHW_STATUS || id == NHW_MESSAGE || id == NHW_MAP)
	return;

    w = &gtkWindows[id]; 

    if (w->type == NHW_MENU)
	GTK_destroy_menu_window(w);

    if(w->w){
	gtk_widget_hide_all(w->w);
	if(w->hid > 0)
	    gtk_signal_disconnect(GTK_OBJECT(w->w), w->hid);

	gtk_widget_destroy(w->w);
    }
    memset(w, 0, sizeof(NHWindow));
    w->type = NHW_NONE;
}

void
GTK_display_nhwindow(winid id, BOOLEAN_P blocking)
{
    NHWindow *w;
    extern int root_height;

    if(id == NHW_STATUS || id == NHW_MESSAGE){
    }
    else if(id == NHW_MAP){	/* flush out */
	nh_map_flush();
    }
    else{
	w = &gtkWindows[id];

	if(w->clist && w->clist->requisition.height >= (2 * root_height) / 3)
	    gtk_widget_set_usize(w->clist, -1, (2 * root_height) / 3);
	gtk_grab_add(w->w);
	gtk_widget_show_all(w->w);
    }

    if((id != NHW_MESSAGE && blocking) || id == NHW_MENU)
	main_hook();
}

void
GTK_clear_nhwindow(winid id)
{
    if(id == NHW_MAP){
	nh_map_clear();
    }
    return;
}

/*
  ATR_ULINE
  ATR_BOLD
  ATR_BLINK
  ATR_INVERSE
 */
void
GTK_putstr(winid id, int attr, const char *str)
{
    const gchar	*text[1];
    NHWindow	*w;

    w = &gtkWindows[id]; 

    if(id == NHW_MESSAGE){
	nh_message_putstr(str);
	return;
    }
    else if(id == NHW_STATUS){
	nh_status_update();
	return;
    }
    else if(id <= 3){
	panic("bad window");
	return;
    }

    if(!w->w){
	w->w = nh_gtk_window_dialog();
	gtk_widget_set_name(GTK_WIDGET(w->w), "fixed font");
	nh_position_popup_dialog(GTK_WIDGET(w->w));

	gtk_signal_connect(
	    GTK_OBJECT(w->w), "key_press_event",
	    GTK_SIGNAL_FUNC(GTK_default_key_press), NULL);
	w->hid = gtk_signal_connect(
	    GTK_OBJECT(w->w), "destroy",
	    GTK_SIGNAL_FUNC(default_destroy), &w->w);

	w->frame = nh_gtk_new_and_add(
	    gtk_frame_new(NULL), w->w, "");

	w->vbox = nh_gtk_new_and_add(
	    gtk_vbox_new(FALSE, 0), w->frame, "");

	w->hbox2 = nh_gtk_new_and_pack(
	    gtk_hbox_new(FALSE, 0), w->vbox, "",
	    FALSE, FALSE, NH_PAD);

	w->clist = nh_gtk_new_and_pack(
	    gtk_clist_new(1), w->hbox2, "",
	    FALSE, FALSE, NH_PAD);
	gtk_clist_set_column_auto_resize(GTK_CLIST(w->clist), 0, TRUE);

	w->adj = (GtkAdjustment *)gtk_adjustment_new(
	    0.0, 0.0, 0.0, 0.0, 0.0, 0.0);
	gtk_clist_set_vadjustment(GTK_CLIST(w->clist), w->adj);

	w->scrolled = nh_gtk_new_and_pack(
	    gtk_vscrollbar_new(GTK_CLIST(w->clist)->vadjustment), w->hbox2,
	    "", FALSE, FALSE, NH_PAD);

	w->hbox3 = nh_gtk_new_and_pack(
	    gtk_hbox_new(FALSE, 0), w->vbox, "",
	    FALSE, FALSE, NH_PAD);

	w->button[0] = nh_gtk_new_and_pack(
	    gtk_button_new_with_label("Close"), w->hbox3, "",
	    TRUE, FALSE, 0);
	gtk_signal_connect(
	    GTK_OBJECT(w->button[0]), "clicked",
	    GTK_SIGNAL_FUNC(default_button_press), (gpointer)'\033');
    }

    text[0] = str;
    gtk_clist_append(GTK_CLIST(w->clist), (gchar **)text);

    if(attr != 0){
	gtk_clist_set_foreground(
	    GTK_CLIST(w->clist), 
	    GTK_CLIST(w->clist)->rows - 1,
	    GTK_WIDGET(w->clist)->style->bg); 
	gtk_clist_set_background(
	    GTK_CLIST(w->clist), 
	    GTK_CLIST(w->clist)->rows - 1,
	    GTK_WIDGET(w->clist)->style->fg); 
    }
}

void
GTK_get_nh_event()
{
    return;
}

int
GTK_nhgetch(void)
{
    int key;
#ifdef RADAR
    nh_radar_update();
#endif

    keysym = keysym1;
    keysym1 = 0;
    while(!keysym)
	main_hook();

    key = keysym;
    keysym = 0;

    return key;
}

int
GTK_nh_poskey(int *x, int *y, int *mod)
{
    int key;

#ifdef RADAR
    nh_radar_update();
#endif

    keysym = keysym1;
    keysym1 = 0;

    while(!keysym)
    {
	nh_map_click(TRUE);
	main_hook();
	nh_map_pos(x, y, mod);
	nh_map_click(FALSE);
    }

    key = keysym;
    keysym = 0;

    return key;
}

#ifdef FILE_AREAS
void
GTK_display_file(const char *farea, const char *fname, BOOLEAN_P complain)
#else
void
GTK_display_file(const char *fname, BOOLEAN_P complain)
#endif
{
    guint hid;	
    GtkWidget *w;
    GtkWidget *scrollbar;
    GtkWidget *label;
    GtkWidget *vbox;
    GtkWidget *hbox, *hbox2;
    GtkWidget *text;
    GtkWidget *button;

    dlb		*fp;
    char	buf[NH_BUFSIZ];

#ifdef FILE_AREAS
    fp = dlb_fopen_area(farea, fname, RDTMODE);
#else
    fp = dlb_fopen(fname, RDTMODE);
#endif

    if(!fp) {
	if(complain) pline("Cannot open %s.  Sorry.",fname);

	return;
    }

    w = nh_gtk_window_dialog();
    gtk_widget_set_name(GTK_WIDGET(w), "fixed font");

    nh_position_popup_dialog(GTK_WIDGET(w));
    gtk_signal_connect(
	GTK_OBJECT(w), "key_press_event",
	GTK_SIGNAL_FUNC(GTK_default_key_press), NULL);
    hid = gtk_signal_connect(
	GTK_OBJECT(w), "destroy",
	GTK_SIGNAL_FUNC(default_destroy), &w);

    vbox = nh_gtk_new_and_add(gtk_vbox_new(FALSE, 0), w, "");

    label = nh_gtk_new_and_pack(
	gtk_label_new("HELP"), vbox, "",
	FALSE, FALSE, NH_PAD);

    hbox = nh_gtk_new_and_pack(
	gtk_hbox_new(FALSE, 0), vbox, "",
	FALSE, FALSE, NH_PAD);

    text = nh_gtk_new_and_pack(
	gtk_text_new(NULL, NULL), hbox, "",
	FALSE, FALSE, NH_PAD);

    gtk_widget_set_usize(
	GTK_WIDGET(text), 600, (root_height * 2)/3);

    scrollbar = nh_gtk_new_and_pack(
	gtk_vscrollbar_new(GTK_TEXT(text)->vadj), hbox, "",
	FALSE, FALSE, NH_PAD);

    hbox2 = nh_gtk_new_and_pack(
	gtk_hbox_new(FALSE, 0), vbox, "",
	FALSE, FALSE, NH_PAD);

    button = nh_gtk_new_and_pack(
	gtk_button_new_with_label("Close"), hbox2, "",
	TRUE, FALSE, NH_PAD);
    gtk_signal_connect(
	GTK_OBJECT(button), "clicked",
	GTK_SIGNAL_FUNC(default_button_press), (gpointer)'\033');

    while(dlb_fgets(buf, NH_BUFSIZ, fp)){
	char *s;
	if ((s = index(buf, '\r')) != 0)
	    memmove(s, s + 1, strlen(s));
	gtk_text_insert(GTK_TEXT(text), NULL, NULL, NULL, buf, strlen(buf));
    }

    gtk_widget_show_all(w);
    main_hook();

    (void) dlb_fclose(fp);

    if (w) {
	gtk_signal_disconnect(GTK_OBJECT(w), hid);
	gtk_widget_destroy(button);
	gtk_widget_destroy(hbox2);
	gtk_widget_destroy(scrollbar);
	gtk_widget_destroy(text);
	gtk_widget_destroy(hbox);
	gtk_widget_destroy(label);
	gtk_widget_destroy(vbox);
	gtk_widget_destroy(w);
    }
}

void
GTK_player_selection(void)
{
    if(!pl_character[0]){
	gtk_main();
	pl_character[0] = pl_selection;
    }
}

void
GTK_update_inventory(void)
{
    if (!initialized2)
	return;
    if (flags.perm_invent)
	(void) display_inventory((char *)0, FALSE);
    else if (WIN_INVEN != WIN_ERR)
	GTK_unmap_menu_window(&gtkWindows[WIN_INVEN]);
}

void
GTK_wait_synch()
{
    ;
}

void
GTK_mark_synch()
{
    ;
}

volatile int delay_finished;

static gint delay_timeout(gpointer data)
{
    delay_finished++;
    return FALSE;
}

void
GTK_delay_output()
{
    if (initialized)
    {
	delay_finished = 0;
	gtk_timeout_add(50L, delay_timeout, 0);
	while (!delay_finished)
	    gtk_main_iteration();
    }
}

#define NAME_LINE 0		/* line # for player name */
#define GOLD_LINE 1		/* line # for amount of gold */
#define DEATH_LINE 2		/* line # for death description */
#define YEAR_LINE 6		/* line # for year */

static struct{
    GdkWChar	str[NH_BUFSIZ];
    int		len;
    int		width;
} rip_line[YEAR_LINE + 1];

void
GTK_outrip(winid id, int how)
{
    int		x, y;
    int		width;
    int		total_len, len, line;
    GtkWidget	*w;
    GtkWidget	*vbox;
    GtkWidget	*rip;
    GdkPixmap 	*rip_pixmap;
    char	mstr[NH_BUFSIZ];
    GdkWChar	*wc;
    GdkWChar	wstr[NH_BUFSIZ];
    extern const char *killed_by_prefix[];
    char	*rip_file;

    w = nh_gtk_window_dialog();
    gtk_window_set_position(GTK_WINDOW(w), GTK_WIN_POS_CENTER);

    gtk_widget_set_events(
	w, GDK_EXPOSURE_MASK | GDK_BUTTON_PRESS_MASK);

    gtk_signal_connect(
	GTK_OBJECT(w), "button_press_event",
	GTK_SIGNAL_FUNC(default_button_press), NULL);
    gtk_signal_connect(
	GTK_OBJECT(w), "key_press_event",
	GTK_SIGNAL_FUNC(GTK_default_key_press), NULL);
    gtk_signal_connect(
	GTK_OBJECT(w), "destroy",
	GTK_SIGNAL_FUNC(default_destroy), &w);

    gtk_widget_realize(w);

    vbox = nh_gtk_new_and_add(gtk_vbox_new(FALSE, 0), w, "");

#ifndef FILE_AREAS
    rip_file = "rip.xpm";
#else
    rip_file = make_file_name(FILE_AREA_SHARE, "rip.xpm");
#endif
    rip_pixmap = gdk_pixmap_create_from_xpm(
	w->window, 0, 0, rip_file);
#ifdef FILE_AREAS
    free(rip_file);
#endif

    rip = nh_gtk_new_and_pack(
	gtk_pixmap_new(rip_pixmap, 0), vbox, "",
	FALSE, FALSE, NH_PAD);

    Sprintf(mstr, "%s", plname);
    rip_line[NAME_LINE].len = gdk_mbstowcs(rip_line[NAME_LINE].str, mstr, NH_BUFSIZ);

    Sprintf(mstr, "%ld Au", u.ugold);
    rip_line[GOLD_LINE].len = gdk_mbstowcs(rip_line[GOLD_LINE].str, mstr, NH_BUFSIZ);

    Sprintf(mstr, "%4d", getyear());
    rip_line[YEAR_LINE].len = gdk_mbstowcs(rip_line[YEAR_LINE].str, mstr, NH_BUFSIZ);

    switch (killer_format) {
    default:
	impossible("bad killer format?");
    case KILLED_BY_AN:
	Strcpy(mstr, killed_by_prefix[how]);
	Strcat(mstr, an(killer));
	break;
    case KILLED_BY:
	Strcpy(mstr, killed_by_prefix[how]);
	Strcat(mstr, killer);
	break;
    case NO_KILLER_PREFIX:
	Strcpy(mstr, killer);
	break;
    }

    total_len = gdk_mbstowcs(wstr, mstr, NH_BUFSIZ);
    line = DEATH_LINE;
    wc = wstr;

    while(total_len > 0 && line < YEAR_LINE){
	len = total_len;
	while(1){
	    width = gdk_text_width_wc(rip->style->font, wc, len);
	    if(width < 96)
		break;
	    --len;
	}
	memcpy(rip_line[line].str, wc, len * sizeof(GdkWChar));
	rip_line[line].len = len;
	wc += len;
	total_len -= len;

	++line;
    }

    x = 155;
    y = 78;

    {
	gint dummy;
	gint height = 0, ascent, descent;

	for(line = 0 ; line <= YEAR_LINE ; ++line){
	    gdk_text_extents_wc(
		rip->style->font,
		rip_line[line].str, rip_line[line].len,
		&dummy,
		&dummy,
		&rip_line[line].width,
		&ascent,
		&descent);
	    if(height < (ascent + descent))
		height = ascent + descent;
	}

	for(line = 0 ; line <= YEAR_LINE ; ++line){
	    gdk_draw_text_wc(
		rip_pixmap,
		rip->style->font,
		rip->style->black_gc,
		x - rip_line[line].width / 2, y,
		rip_line[line].str, rip_line[line].len);
	    y += height;
	}
    }

    gtk_widget_show_all(w);
    gtk_main();
}

void
GTK_raw_print(const char *str)
{
    if(rawprint_win != WIN_ERR)
	GTK_putstr(rawprint_win, 0, str);
    else {
#ifdef TTY_GRAPHICS
	tty_raw_print(str);
#else
	puts(str); (void) fflush(stdout);
#endif
    }
}

void
GTK_raw_print_bold(const char *str)
{
    if(rawprint_win != WIN_ERR)
	GTK_putstr(rawprint_win, ATR_BOLD, str);
    else {
#ifdef TTY_GRAPHICS
	tty_raw_print_bold(str);
#else
	puts(str); (void) fflush(stdout);
#endif
    }
}