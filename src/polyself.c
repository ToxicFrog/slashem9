/*	SCCS Id: @(#)polyself.c	3.4	2003/01/08	*/
/*	Copyright (C) 1987, 1988, 1989 by Ken Arromdee */
/* NetHack may be freely redistributed.  See license for details. */

/*
 * Polymorph self routine.
 *
 * Note:  the light source handling code assumes that both youmonst.m_id
 * and youmonst.mx will always remain 0 when it handles the case of the
 * player polymorphed into a light-emitting monster.
 */

#include "hack.h"

static void set_uasmon(int new_monnum);
static void polyman(const char *, const char *);
static void break_armor(void);
static void drop_weapon(int);
static void uunstick(void);
static int armor_to_dragon(int);
static void newman(void);
static void merge_with_armor(void);
static bool polysense(struct permonst *mptr);

/* Assumes u.umonster is set up already */
/* Use u.umonster since we might be restoring and you may be polymorphed */
void init_uasmon(void) {
	int i;

	upermonst = mons[u.umonster];

	/* Fix up the flags */
	/* Default flags assume human,  so replace with your race's flags */

	upermonst.mflags1 &= ~(mons[PM_HUMAN].mflags1);
	upermonst.mflags1 |= (mons[urace.malenum].mflags1);

	upermonst.mflags2 &= ~(mons[PM_HUMAN].mflags2);
	upermonst.mflags2 |= (mons[urace.malenum].mflags2);

	upermonst.mflags3 &= ~(mons[PM_HUMAN].mflags3);
	upermonst.mflags3 |= (mons[urace.malenum].mflags3);

	/* Fix up the attacks */
	for (i = 0; i < NATTK; i++) {
		upermonst.mattk[i] = mons[urace.malenum].mattk[i];
	}

	set_uasmon(u.umonnum);
}

/* update the youmonst.data structure pointer */
void set_uasmon(int new_monnum) {
	int old_monnum = u.umonnum;

	bool did_vampshift = ((youmonst.data && is_vampire(youmonst.data)) || (!Upolyd && is_vampire(&upermonst)))
			     && old_monnum != new_monnum;
			    // && is_valid_vampshift(youmonst.cham, u.umonnum) //TODO

	u.umonnum = new_monnum;

	set_mon_data(&youmonst, ((u.umonnum == u.umonster) ? &upermonst : &mons[u.umonnum]), 0);

	if (youmonst.cham == CHAM_ORDINARY && did_vampshift)
		youmonst.cham = old_monnum;
	else
		youmonst.cham = CHAM_ORDINARY;

	bot_reconfig();
}

/* make a (new) human out of the player */
static void polyman(const char *fmt, const char *arg) {
	boolean sticky = sticks(youmonst.data) && u.ustuck && !u.uswallow,
		was_mimicking = (youmonst.m_ap_type == M_AP_OBJECT);
	boolean was_blind = !!Blind;

	if (Upolyd) {
		u.acurr = u.macurr; /* restore old attribs */
		u.amax = u.mamax;
		flags.female = u.mfemale;
	}

	set_uasmon(Upolyd ? u.umonster : u.umonnum);

	u.mh = u.mhmax = 0;
	u.mtimedone = 0;
	skinback(false);
	u.uundetected = 0;
	newsym(u.ux, u.uy);

	if (sticky) uunstick();
	find_ac();
	if (was_mimicking) {
		if (multi < 0) unmul("");
		youmonst.m_ap_type = M_AP_NOTHING;
	}

	newsym(u.ux, u.uy);

	pline(fmt, arg);
	/* check whether player foolishly genocided self while poly'd */
	if ((mvitals[urole.malenum].mvflags & G_GENOD) ||
	    (urole.femalenum != NON_PM &&
	     (mvitals[urole.femalenum].mvflags & G_GENOD)) ||
	    (mvitals[urace.malenum].mvflags & G_GENOD) ||
	    (urace.femalenum != NON_PM &&
	     (mvitals[urace.femalenum].mvflags & G_GENOD))) {
		/* intervening activity might have clobbered genocide info */
		struct kinfo *kptr = find_delayed_killer(POLYMORPH);
		if (kptr && kptr->name.len) {
			killer.format = kptr->format;
			nhsmove(&killer.name, &kptr->name);
		} else {
			killer.format = KILLED_BY;
			nhscopyz(&killer.name, "self-genocide");
		}

		dealloc_killer(kptr);

		done(GENOCIDED);
	}

	if (u.twoweap && !could_twoweap(youmonst.data))
		untwoweapon();

	if (u.utraptype == TT_PIT && u.utrap) {
		u.utrap = rn1(6, 2); // time to escape resets
	}
	if (was_blind && !Blind) { /* reverting from eyeless */
		Blinded = 1L;
		make_blinded(0L, true); /* remove blindness */
	}

	if (!Levitation && !u.ustuck &&
	    (is_pool(u.ux, u.uy) || is_lava(u.ux, u.uy)))
		spoteffects(true);

	see_monsters();
}

void change_sex(void) {
	/* setting u.umonster for caveman/cavewoman or priest/priestess
	   swap unintentionally makes `Upolyd' appear to be true */
	boolean already_polyd = (boolean)Upolyd;

	/* Some monsters are always of one sex and their sex can't be changed */
	/* succubi/incubi can change, but are handled below */
	/* !already_polyd check necessary because is_male() and is_female()
	   are true if the player is a priest/priestess */
	if (!already_polyd || (!is_male(youmonst.data) && !is_female(youmonst.data) && !is_neuter(youmonst.data)))
		flags.female = !flags.female;
	if (already_polyd) /* poly'd: also change saved sex */
		u.mfemale = !u.mfemale;
	max_rank_sz(); /* [this appears to be superfluous] */
	if ((already_polyd ? u.mfemale : flags.female) && urole.name.f)
		strcpy(pl_character, urole.name.f);
	else
		strcpy(pl_character, urole.name.m);
	u.umonster = ((already_polyd ? u.mfemale : flags.female) && urole.femalenum != NON_PM) ?
			     urole.femalenum :
			     urole.malenum;

	int new_monnum = u.umonnum;
	if (!already_polyd) {
		new_monnum = u.umonster;
	} else if (is_foocubus(&mons[u.umonnum])) {
		flags.female = !flags.female;
		/* change monster type to match new sex */
		new_monnum = (u.umonnum == PM_SUCCUBUS) ? PM_INCUBUS : PM_SUCCUBUS;
	}
	set_uasmon(new_monnum);
}

static void newman(void) {
	int tmp, oldlvl;

	if (Race_if(PM_DOPPELGANGER)) {
		if (!rn2(10)) change_sex();
	} else {
		tmp = u.uhpmax;
		oldlvl = u.ulevel;
		u.ulevel = u.ulevel + rn1(5, -2);
		if (u.ulevel > 127 || u.ulevel < 1) { /* level went below 0? */
			u.ulevel = oldlvl;	      /* restore old level in case they lifesave */
			goto dead;
		}
		if (u.ulevel > MAXULEV) u.ulevel = MAXULEV;
		/* If your level goes down, your peak level goes down by
		   the same amount so that you can't simply use blessed
		   full healing to undo the decrease.  But if your level
		   goes up, your peak level does *not* undergo the same
		   adjustment; you might end up losing out on the chance
		   to regain some levels previously lost to other causes. */
		if (u.ulevel < oldlvl) u.ulevelmax -= (oldlvl - u.ulevel);
		if (u.ulevelmax < u.ulevel) u.ulevelmax = u.ulevel;

		if (!rn2(10)) change_sex();

		adjabil(oldlvl, (int)u.ulevel);
		reset_rndmonst(NON_PM); /* new monster generation criteria */

		/* random experience points for the new experience level */
		u.uexp = rndexp(false);

		/* u.uhpmax * u.ulevel / oldlvl: proportionate hit points to new level
		 * -10 and +10: don't apply proportionate HP to 10 of a starting
		 *   character's hit points (since a starting character's hit points
		 *   are not on the same scale with hit points obtained through level
		 *   gain)
		 * 9 - rn2(19): random change of -9 to +9 hit points
		 */
		u.uhpmax = ((u.uhpmax - 10) * (long)u.ulevel / oldlvl + 10) +
			   (9 - rn2(19));
		u.uhp = u.uhp * (long)u.uhpmax / tmp;

		tmp = u.uenmax;
		u.uenmax = u.uenmax * (long)u.ulevel / oldlvl + 9 - rn2(19);
		if (u.uenmax < 0) u.uenmax = 0;
		u.uen = (tmp ? u.uen * (long)u.uenmax / tmp : u.uenmax);
	}

	redist_attr();
	u.uhunger = rn1(500, 500);
	if (Sick) make_sick(0L, NULL, false, SICK_ALL);
	if (Stoned) make_stoned(0, NULL, 0, new_nhs());
	if (Race_if(PM_DOPPELGANGER)) {
		if (u.uhp <= 10) u.uhp = 10;
		if (u.uhpmax <= 10) u.uhpmax = 10;
		if (u.uen <= u.ulevel) u.uen = u.ulevel;
		if (u.uenmax <= u.ulevel) u.uenmax = u.ulevel;
	}
	if (u.uhp <= 0 || u.uhpmax <= 0) {
		if (Polymorph_control) {
			if (u.uhp <= 0) u.uhp = 1;
			if (u.uhpmax <= 0) u.uhpmax = 1;
		} else {
		dead: /* we come directly here if their experience level went to 0 or less */
			pline("Your new form doesn't seem healthy enough to survive.");
			killer.format = KILLED_BY_AN;
			nhscopyz(&killer.name, "unsuccessful polymorph");
			done(DIED);
			newuhs(false);
			polysense(youmonst.data);
			return; /* lifesaved */
		}
	}
	newuhs(false);
	polyman("You feel like a new %s!",
		(flags.female && urace.individual.f) ? urace.individual.f :
						       (urace.individual.m) ? urace.individual.m : urace.noun);
	if (Slimed) {
		make_slimed(10, "Your body transforms, but there is still slime on you.");
	}
	polysense(youmonst.data);
	context.botl = 1;
	vision_full_recalc = 1;
	encumber_msg();
	see_monsters();
}

void polyself(int psflags) {
	char buf[BUFSZ];
	int old_light, new_light;
	int mntmp = NON_PM;
	int tries = 0;
	bool draconian = (uarm &&
			  uarm->otyp >= GRAY_DRAGON_SCALE_MAIL &&
			  uarm->otyp <= YELLOW_DRAGON_SCALES);
	bool iswere = (u.ulycn >= LOW_PM || is_were(youmonst.data));
	bool isvamp = (is_vampire(youmonst.data) || is_vampshifter(&youmonst));
	bool was_floating = (Levitation || Flying);
	bool forcecontrol = (psflags == 1);
	bool monsterpoly = (psflags == 2);

	/* [Tom] I made the chance of dying from Con check only possible for
		 really weak people (it was out of 20) */

	if (!Polymorph_control && !forcecontrol && !draconian && !iswere &&
	    !isvamp && !Race_if(PM_DOPPELGANGER)) {
		if (rn2(12) > ACURR(A_CON)) {
			pline("You shudder for a moment.");
			losehp(rnd(30), "system shock", KILLED_BY_AN);
			exercise(A_CON, false);
			return;
		}
	}
	old_light = Upolyd ? emits_light(youmonst.data) : 0;

	if ((Polymorph_control || forcecontrol) && !monsterpoly) {
		do {
			getlin("Become what kind of monster? [type the name]",
			       buf);
			mntmp = name_to_mon(buf);
			if (mntmp < LOW_PM)
				pline("I've never heard of such monsters.");
			/* Note:  humans are illegal as monsters, but an
			 * illegal monster forces newman(), which is what we
			 * want if they specified a human.... */
			else if (!polyok(&mons[mntmp]) && !your_race(&mons[mntmp]))
				pline("You cannot polymorph into that.");
			else
				break;
		} while (++tries < 5);
		if (tries == 5) pline("That's enough tries!");
		/* allow skin merging, even when polymorph is controlled */
		if (draconian &&
		    (mntmp == armor_to_dragon(uarm->otyp) || tries == 5))
			goto do_merge;
	} else if (Race_if(PM_DOPPELGANGER)) {
		/* Not an experienced Doppelganger yet */
		do {
			/* Slightly different wording */
			getlin("Attempt to become what kind of monster? [type the name]",
			       buf);
			mntmp = name_to_mon(buf);
			if (mntmp < LOW_PM)
				pline("I've never heard of such monsters.");
			/* Note:  humans are illegal as monsters, but an
			 * illegal monster forces newman(), which is what we
			 * want if they specified a human.... */
			else if (!polyok(&mons[mntmp]) && !your_race(&mons[mntmp]))
				pline("You cannot polymorph into that.");
			else if (!mvitals[mntmp].eaten) {
				pline("You attempt an unfamiliar polymorph.");
				if ((rn2(5) + u.ulevel) < mons[mntmp].mlevel)
					mntmp = LOW_PM - 1; /* Didn't work for sure */
				/* Either way, give it a shot */
				break;
			}

			else
				break;
		} while (++tries < 5);
		if (tries == 5) pline("That's enough tries!");
		/* allow skin merging, even when polymorph is controlled */
		if (draconian &&
		    (mntmp == armor_to_dragon(uarm->otyp) || tries == 5))
			goto do_merge;
	} else if (draconian || iswere || isvamp) {
		/* special changes that don't require polyok() */
		if (draconian) {
do_merge:
			mntmp = armor_to_dragon(uarm->otyp);

			if (mntmp >= 0 && !(mvitals[mntmp].mvflags & G_GENOD)) {
				/* Code that was here is now in merge_with_armor */
				merge_with_armor();
			}
		} else if (iswere) {
			if (is_were(youmonst.data))
				mntmp = PM_HUMAN; /* Illegal; force newman() */
			else
				mntmp = u.ulycn;
		} else if (isvamp) {
			if (is_vampshifter(&youmonst)) {
				// return to vampiric form
				mntmp = youmonst.cham;
			} else {
				mntmp = (youmonst.data != &mons[PM_VAMPIRE] && !rn2(10)) ? PM_WOLF :
					!rn2(4) ? PM_FOG_CLOUD :
					PM_VAMPIRE_BAT;
			}

			if (Polymorph_control) {
				char buf[BUFSZ];
				sprintf(buf, "Become %s?", an(mons[mntmp].mname));
				if (yn(buf) != 'y') return;
			}
			if (Unchanging) {
				pline("You fail to transform!");
				return;
			}
		}
		/* if polymon fails, "you feel" message has been given
		   so don't follow up with another polymon or newman */
		if (mntmp == PM_HUMAN)
			newman(); /* werecritter */
		else
			polymon(mntmp);
		goto made_change; /* maybe not, but this is right anyway */
	}
	if (mntmp < LOW_PM) {
		tries = 0;
		do {
			/* randomly pick an "ordinary" monster */
			mntmp = rn1(SPECIAL_PM - LOW_PM, LOW_PM);
		} while ((!polyok(&mons[mntmp]) || is_placeholder(&mons[mntmp])) && tries++ < 200);
	}

	/* The below polyok() fails either if everything is genocided, or if
	 * we deliberately chose something illegal to force newman().
	 */
	/* WAC Doppelgangers go through a 1/20 check rather than 1/5 */
	if (!polyok(&mons[mntmp]) ||
	    (Race_if(PM_DOPPELGANGER) ? (
						((u.ulevel < mons[mntmp].mlevel) || !mvitals[mntmp].eaten) && !rn2(20)) :
					!rn2(5)) ||
	    your_race(&mons[mntmp]))
		newman();
	else if (!polymon(mntmp))
		return;

	if (!uarmg) selftouch("No longer petrify-resistant, you");

made_change:
	new_light = Upolyd ? emits_light(youmonst.data) : 0;
	if (old_light != new_light) {
		if (old_light)
			del_light_source(LS_MONSTER, monst_to_any(&youmonst));
		if (new_light == 1) ++new_light; /* otherwise it's undetectable */
		if (new_light)
			new_light_source(u.ux, u.uy, new_light, LS_MONSTER, monst_to_any(&youmonst));
	}
	if (is_pool(u.ux, u.uy) && was_floating && !(Levitation || Flying) &&
	    !breathless(youmonst.data) && !amphibious(youmonst.data) &&
	    !Swimming) drown();
}

/* (try to) make a mntmp monster out of the player */
/* returns 1 if polymorph successful */
int polymon(int mntmp) {
	boolean sticky = sticks(youmonst.data) && u.ustuck && !u.uswallow,
		was_blind = !!Blind, dochange = false;
	int mlvl;

	if (mvitals[mntmp].mvflags & G_GENOD) { /* allow G_EXTINCT */
		pline("You feel rather %s-ish.", mons[mntmp].mname);
		exercise(A_WIS, true);
		return 0;
	}

	/* STEPHEN WHITE'S NEW CODE */

	/* If your an Undead Slayer, you can't become undead! */

	if (is_undead(&mons[mntmp]) && Role_if(PM_UNDEAD_SLAYER)) {
		if (Polymorph_control) {
			pline("You hear a voice boom out: \"How dare you take such a form!\"");
			u.ualign.record -= 5;
			exercise(A_WIS, false);
		} else {
			pline("You start to change into %s, but a voice booms out:", an(mons[mntmp].mname));
			pline("\"No, I will not allow such a change!\"");
		}
		return 0;
	}

	/* KMH, conduct */
	u.uconduct.polyselfs++;

	if (!Upolyd) {
		/* Human to monster; save human stats */
		u.macurr = u.acurr;
		u.mamax = u.amax;
		u.mfemale = flags.female;
	} else {
		/* Monster to monster; restore human stats, to be
		 * immediately changed to provide stats for the new monster
		 */
		u.acurr = u.macurr;
		u.amax = u.mamax;
		flags.female = u.mfemale;
	}

	if (youmonst.m_ap_type) {
		/* stop mimicking immediately */
		if (multi < 0) unmul("");
	} else if (mons[mntmp].mlet != S_MIMIC) {
		/* as in polyman() */
		youmonst.m_ap_type = M_AP_NOTHING;
	}
	if (is_male(&mons[mntmp])) {
		if (flags.female) dochange = true;
	} else if (is_female(&mons[mntmp])) {
		if (!flags.female) dochange = true;
	} else if (!is_neuter(&mons[mntmp]) && mntmp != u.ulycn) {
		if (!rn2(10)) dochange = true;
	}
	if (dochange) {
		flags.female = !flags.female;
		pline("You %s %s%s!",
		      (u.umonnum != mntmp) ? "turn into a" : "feel like a new",
		      (is_male(&mons[mntmp]) || is_female(&mons[mntmp])) ? "" :
									   flags.female ? "female " : "male ",
		      mons[mntmp].mname);
	} else {
		if (u.umonnum != mntmp)
			pline("You turn into %s!", an(mons[mntmp].mname));
		else
			pline("You feel like a new %s!", mons[mntmp].mname);
	}
	if (Stoned && poly_when_stoned(&mons[mntmp])) {
		/* poly_when_stoned already checked stone golem genocide */
		mntmp = PM_STONE_GOLEM;
		make_stoned(0, "You turn to stone!", 0, new_nhs());
	}

	u.mtimedone = rn1(500, 500);
	set_uasmon(mntmp);

	/* New stats for monster, to last only as long as polymorphed.
	 * Currently only strength gets changed.
	 */
	if (strongmonst(&mons[mntmp])) ABASE(A_STR) = AMAX(A_STR) = STR18(100);

	if (Stone_resistance && Stoned) { /* parnes@eniac.seas.upenn.edu */
		make_stoned(0, "You no longer seem to be petrifying.", 0, new_nhs());
	}
	if (Sick_resistance && Sick) {
		make_sick(0L, NULL, false, SICK_ALL);
		pline("You no longer feel sick.");
	}
	if (Slimed) {
		if (flaming(youmonst.data)) {
			make_slimed(0, "The slime burns away!");
		} else if (mntmp == PM_GREEN_SLIME) {
			/* do it silently */
			make_slimed(0, NULL);
		}
	}
	if (nohands(youmonst.data)) Glib = 0;

	/*
	mlvl = adj_lev(&mons[mntmp]);
	 * We can't do the above, since there's no such thing as an
	 * "experience level of you as a monster" for a polymorphed character.
	 */
	mlvl = ((mntmp == u.ulycn) ? u.ulevel : (int)mons[mntmp].mlevel);
	if (youmonst.data->mlet == S_DRAGON && mntmp >= PM_GRAY_DRAGON) {
		u.mhmax = In_endgame(&u.uz) ? (8 * mlvl) : (4 * mlvl + d(mlvl, 4));
	} else if (is_golem(youmonst.data)) {
		u.mhmax = golemhp(mntmp);
	} else {
		if (!mlvl)
			u.mhmax = rnd(4);
		else
			u.mhmax = d(mlvl, 8);
		if (is_home_elemental(&mons[mntmp])) u.mhmax *= 3;
	}
	u.mh = u.mhmax;

	if (u.ulevel < mlvl) {
		/* Low level characters can't become high level monsters for long */
		u.mtimedone = u.mtimedone * u.ulevel / mlvl;
	}

	/* WAC Doppelgangers can stay much longer in a form they know well */
	if (Race_if(PM_DOPPELGANGER) && mvitals[mntmp].eaten) {
		u.mtimedone *= 2;
		u.mtimedone += mvitals[mntmp].eaten;
	}

	if (uskin && mntmp != armor_to_dragon(uskin->otyp))
		skinback(false);
	break_armor();
	drop_weapon(1);
	if (hides_under(youmonst.data))
		u.uundetected = OBJ_AT(u.ux, u.uy);
	else if (youmonst.data->mlet == S_EEL)
		u.uundetected = is_pool(u.ux, u.uy);
	else
		u.uundetected = 0;

	if (u.utraptype == TT_PIT && u.utrap) {
		u.utrap = rn1(6, 2); // time to escape resets
	}
	if (was_blind && !Blind) { /* previous form was eyeless */
		Blinded = 1L;
		make_blinded(0L, true); /* remove blindness */
	}
	newsym(u.ux, u.uy); /* Change symbol */

	if (!sticky && !u.uswallow && u.ustuck && sticks(youmonst.data))
		setustuck(0);
	else if (sticky && !sticks(youmonst.data))
		uunstick();
	if (u.usteed) {
		if (touch_petrifies(u.usteed->data) &&
		    !Stone_resistance && rnl(3)) {
			char buf[BUFSZ];

			pline("No longer petrifying-resistant, you touch %s.",
			      mon_nam(u.usteed));
			sprintf(buf, "riding %s", an(u.usteed->data->mname));
			instapetrify(buf);
		}
		if (!can_ride(u.usteed)) dismount_steed(DISMOUNT_POLY);
	}

	if (flags.verbose) {
		static const char use_thec[] = "Use the command #%s to %s.";
		static const char monsterc[] = "monster";
		if (can_breathe(youmonst.data))
			pline(use_thec, monsterc, "use your breath weapon");
		if (attacktype(youmonst.data, AT_SPIT))
			pline(use_thec, monsterc, "spit venom");
		if (youmonst.data->mlet == S_NYMPH)
			pline(use_thec, monsterc, "remove an iron ball");
		if (attacktype(youmonst.data, AT_GAZE))
			pline(use_thec, monsterc, "gaze at monsters");
		if (is_hider(youmonst.data))
			pline(use_thec, monsterc, "hide");
		if (is_were(youmonst.data))
			pline(use_thec, monsterc, "summon help");
		if (webmaker(youmonst.data))
			pline(use_thec, monsterc, "spin a web");
		if (u.umonnum == PM_GREMLIN)
			pline(use_thec, monsterc, "multiply in a fountain");
		if (is_unicorn(youmonst.data))
			pline(use_thec, monsterc, "use your horn");
		if (is_mind_flayer(youmonst.data))
			pline(use_thec, monsterc, "emit a mental blast");
		if (youmonst.data->msound == MS_SHRIEK) /* worthless, actually */
			pline(use_thec, monsterc, "shriek");
		if (lays_eggs(youmonst.data) && flags.female)
			pline(use_thec, "sit", "lay an egg");
	}
	/* you now know what an egg of your type looks like */
	if (lays_eggs(youmonst.data)) {
		/* make queen bees recognize killer bee eggs */
		learn_egg_type(egg_type_from_parent(u.umonnum, true));
	}
	find_ac();
	if ((!Levitation && !u.ustuck && !Flying &&
	     (is_pool(u.ux, u.uy) || is_lava(u.ux, u.uy))) ||
	    (Underwater && !Swimming))
		spoteffects(true);
	if (Passes_walls && u.utrap && u.utraptype == TT_INFLOOR) {
		u.utrap = 0;
		pline("The rock seems to no longer trap you.");
	} else if (likes_lava(youmonst.data) && u.utrap && u.utraptype == TT_LAVA) {
		u.utrap = 0;
		pline("The lava now feels soothing.");
	}
	if (amorphous(youmonst.data) || is_whirly(youmonst.data) || unsolid(youmonst.data)) {
		if (Punished) {
			pline("You slip out of the iron chain.");
			unpunish();
		}
	}
	if (u.utrap && (u.utraptype == TT_WEB || u.utraptype == TT_BEARTRAP) &&
	    (amorphous(youmonst.data) || is_whirly(youmonst.data) || unsolid(youmonst.data) ||
	     (youmonst.data->msize <= MZ_SMALL && u.utraptype == TT_BEARTRAP))) {
		pline("You are no longer stuck in the %s.",
		      u.utraptype == TT_WEB ? "web" : "bear trap");
		/* probably should burn webs too if PM_FIRE_ELEMENTAL */
		u.utrap = 0;
	}
	if (webmaker(youmonst.data) && u.utrap && u.utraptype == TT_WEB) {
		pline("You orient yourself on the web.");
		u.utrap = 0;
	}
	polysense(youmonst.data);
	context.botl = 1;
	vision_full_recalc = 1;
	see_monsters();
	exercise(A_CON, false);
	exercise(A_WIS, true);
	encumber_msg();
	return 1;
}

static void break_armor(void) {
	struct obj *otmp;
	const bool controlled_change = (Race_if(PM_DOPPELGANGER) || (Race_if(PM_HUMAN_WEREWOLF) && u.umonnum == PM_WEREWOLF));

	if (breakarm(youmonst.data)) {
		if ((otmp = uarm) != 0) {
			if (otmp->oartifact) {
				if (donning(otmp)) cancel_don();
				pline("Your armor falls off!");
				Armor_gone();
				dropx(otmp); /*WAC Drop instead of destroy*/
			} else if (controlled_change && !otmp->cursed) {
				if (donning(otmp)) cancel_don();
				pline("You quickly remove your armor as you start to change.");
				Armor_gone();
				dropx(otmp); /*WAC Drop instead of destroy*/
			} else {
				if (donning(otmp)) cancel_don();
				pline("You break out of your armor!");
				exercise(A_STR, false);
				Armor_gone();
				useup(otmp);
			}
		}
		if ((otmp = uarmc) != 0) {
			if (otmp->oartifact) {
				pline("Your %s falls off!", cloak_simple_name(otmp));
				Cloak_off();
				dropx(otmp);
			} else if (controlled_change && !otmp->cursed) {
				pline("You remove your %s before you transform.",
				      cloak_simple_name(otmp));
				Cloak_off();
				dropx(otmp);
			} else {
				pline("Your %s tears apart!", cloak_simple_name(otmp));
				Cloak_off();
				useup(otmp);
			}
		}
		if ((otmp = uarmu) != 0) {
			if (controlled_change && !otmp->cursed && !uskin) {
				pline("You take off your shirt just before it starts to rip.");
				setworn(NULL, otmp->owornmask & W_ARMU);
				dropx(otmp);
			} else {
				pline("Your shirt rips to shreds!");
				useup(uarmu);
			}
		}
	} else if (sliparm(youmonst.data)) {
		if (((otmp = uarm) != 0) && (racial_exception(&youmonst, otmp) < 1)) {
			if (donning(otmp)) cancel_don();
			pline("Your armor falls around you!");
			Armor_gone();
			dropx(otmp);
		}
		if ((otmp = uarmc) != 0) {
			if (is_whirly(youmonst.data))
				pline("Your %s falls, unsupported!", cloak_simple_name(otmp));
			else
				pline("You shrink out of your %s!", cloak_simple_name(otmp));
			Cloak_off();
			dropx(otmp);
		}
		if ((otmp = uarmu) != 0) {
			if (is_whirly(youmonst.data))
				pline("You seep right through your shirt!");
			else
				pline("You become much too small for your shirt!");
			setworn(NULL, otmp->owornmask & W_ARMU);
			dropx(otmp);
		}
	}
	if (has_horns(youmonst.data)) {
		if ((otmp = uarmh) != 0) {
			if (is_flimsy(otmp) && !donning(otmp)) {
				char hornbuf[BUFSZ];

				/* Future possiblities: This could damage/destroy helmet */
				sprintf(hornbuf, "horn%s", plur(num_horns(youmonst.data)));
				pline("Your %s %s through %s.", hornbuf, vtense(hornbuf, "pierce"), yname(otmp));
			} else {
				if (donning(otmp)) cancel_don();
				pline("Your %s falls to the %s!", helm_simple_name(otmp), surface(u.ux, u.uy));
				Helmet_off();
				dropx(otmp);
			}
		}
	}
	if (nohands(youmonst.data) || verysmall(youmonst.data)) {
		if ((otmp = uarmg) != 0) {
			if (donning(otmp)) cancel_don();
			/* Drop weapon along with gloves */
			pline("You drop your gloves%s!", uwep ? " and weapon" : "");
			drop_weapon(0);
			Gloves_off();
			dropx(otmp);
		}
		if ((otmp = uarms) != 0) {
			pline("You can no longer hold your shield!");
			Shield_off();
			dropx(otmp);
		}
		if ((otmp = uarmh) != 0) {
			if (donning(otmp)) cancel_don();
			pline("Your %s falls to the %s!", helm_simple_name(uarmh), surface(u.ux, u.uy));
			Helmet_off();
			dropx(otmp);
		}
	}
	if (nohands(youmonst.data) || verysmall(youmonst.data) ||
	    slithy(youmonst.data) || youmonst.data->mlet == S_CENTAUR) {
		if ((otmp = uarmf) != 0) {
			if (donning(otmp)) cancel_don();
			if (is_whirly(youmonst.data))
				pline("Your boots fall away!");
			else
				pline("Your boots %s off your feet!",
				      verysmall(youmonst.data) ? "slide" : "are pushed");
			Boots_off();
			dropx(otmp);
		}
	}
}

static void drop_weapon(int alone) {
	struct obj *otmp;
	struct obj *otmp2;

	if ((otmp = uwep) != 0) {
		/* !alone check below is currently superfluous but in the
		 * future it might not be so if there are monsters which cannot
		 * wear gloves but can wield weapons
		 */
		if (!alone || cantwield(youmonst.data)) {
			struct obj *wep = uwep;

			if (alone) pline("You find you must drop your weapon%s!",
					 u.twoweap ? "s" : "");
			otmp2 = u.twoweap ? uswapwep : 0;
			uwepgone();
			if (!wep->cursed || wep->otyp != LOADSTONE)
				dropx(otmp);
			if (otmp2 != 0) {
				uswapwepgone();
				if (!otmp2->cursed || otmp2->otyp != LOADSTONE)
					dropx(otmp2);
			}
			untwoweapon();
		} else if (!could_twoweap(youmonst.data)) {
			untwoweapon();
		}
	} else if (cantwield(youmonst.data))
		untwoweapon();
}

void rehumanize(void) {
	boolean forced = (u.mh < 1);

	/* KMH, balance patch -- you can't revert back while unchanging */
	if (Unchanging && forced) {
		killer.format = NO_KILLER_PREFIX;
		nhscopyz(&killer.name, "killed while stuck in creature form");
		done(DIED);
	}

	if (emits_light(youmonst.data))
		del_light_source(LS_MONSTER, monst_to_any(&youmonst));
	polyman("You return to %s form!", urace.adj);

	if (u.uhp < 1) {
		killer.format = KILLED_BY;
		nhscopyf(&killer.name, "reverting to unhealthy %S form", urace.adj);
		done(DIED);
	}

	if (forced || (!Race_if(PM_DOPPELGANGER) && (rn2(20) > ACURR(A_CON)))) {
		/* Exhaustion for "forced" rehumaization & must pass con chack for
		 * non-doppelgangers
		 * Don't penalize doppelgangers/polymorph running out */

		/* WAC Polymorph Exhaustion 1/2 HP to prevent abuse */
		u.uhp = (u.uhp / 2) + 1;
	}

	if (!uarmg) selftouch("No longer petrify-resistant, you");
	nomul(0);

	context.botl = 1;
	vision_full_recalc = 1;
	encumber_msg();
}

/* WAC -- MUHAHAHAAHA - Gaze attacks!
 * Note - you can only gaze at one monster at a time, to keep this
 * from getting out of hand ;B  Also costs 20 energy.
 */
int dogaze(void) {
	coord cc;
	struct monst *mtmp;

	if (Blind) {
		pline("You can't see a thing!");
		return 0;
	}
	if (u.uen < 20) {
		pline("You lack the energy to use your special gaze!");
		return 0;
	}
	pline("Where do you wish to look?");
	cc.x = u.ux;
	cc.y = u.uy;
	getpos(&cc, true, "the spot to look");

	if (cc.x == -10) return 0; /* user pressed esc */

	mtmp = m_at(cc.x, cc.y);

	if (!mtmp || !canseemon(mtmp)) {
		pline("You don't see a monster there!");
		return 0;
	}

	if ((flags.safe_dog && !Confusion && !Hallucination && mtmp->mtame) || (flags.confirm && mtmp->mpeaceful && !Confusion && !Hallucination)) {
		char qbuf[QBUFSZ];

		sprintf(qbuf, "Really gaze at %s?", mon_nam(mtmp));
		if (yn(qbuf) != 'y') return 0;
		if (mtmp->mpeaceful) setmangry(mtmp);
	}

	u.uen -= 20;

	pline("You gaze at %s...", mon_nam(mtmp));

	if ((mtmp->data == &mons[PM_MEDUSA]) && !mtmp->mcan) {
		pline("Gazing at the wakeful Medusa is not a very good idea.");
		/* as if gazing at a sleeping anything is fruitful... */
		pline("You turn to stone...");
		killer.format = KILLED_BY;
		nhscatz(&killer.name, "deliberately gazing at Medusa's hideous countenance");
		done(STONING);
	} else if (!mtmp->mcansee || mtmp->msleeping) {
		pline("But nothing happens.");
		return 1;
	} else if (Invis && !perceives(mtmp->data)) {
		pline("%s seems not to notice your gaze.", Monnam(mtmp));
		return 1;
	} else {
		struct attack *mattk;
		int i;

		for (i = 0; i < NATTK; i++) {
			mattk = &(youmonst.data->mattk[i]);
			if (mattk->aatyp == AT_GAZE) {
				damageum(mtmp, mattk);
				break;
			}
		}
	}
	return 1;
}

int dobreathe(void) {
	struct attack *mattk;
	int energy = 0;

	if (Strangled) {
		pline("You can't breathe.  Sorry.");
		return 0;
	}

	/* WAC -- no more belching.  Use up energy instead */
	if (Race_if(PM_DOPPELGANGER) || (Role_if(PM_FLAME_MAGE) && u.umonnum == PM_RED_DRAGON) || (Role_if(PM_ICE_MAGE) && u.umonnum == PM_WHITE_DRAGON))
		energy = 10;
	else
		energy = 15;

	if (u.uen < energy) {
		pline("You don't have enough energy to breathe!");
		return 0;
	}

	if (!getdir(NULL)) return 0;

	u.uen -= energy;
	context.botl = 1;

	mattk = attacktype_fordmg(youmonst.data, AT_BREA, AD_ANY);
	if (!mattk)
		impossible("bad breath attack?"); /* mouthwash needed... */
	else {
		/* Extra handling for AD_RBRE - player might poly into a crystal
		 * golem. */
		uchar adtyp;
		adtyp = mattk->adtyp == AD_RBRE ? rnd(AD_ACID) : mattk->adtyp;
		buzz((int)(20 + adtyp - 1), (int)mattk->damn,
		     u.ux, u.uy, u.dx, u.dy);
	}
	return 1;
}

int dospit(void) {
	struct obj *otmp;
	struct attack *mattk;

	if (!getdir(NULL)) return 0;
	mattk = attacktype_fordmg(youmonst.data, AT_SPIT, AD_ANY);
	if (!mattk)
		impossible("bad spit attack?");
	else {
		switch (mattk->adtyp) {
			case AD_BLND:
			case AD_DRST:
				otmp = mksobj(BLINDING_VENOM, true, false);
				break;
			default:
				impossible("bad attack type in do_spit");
			/* fall through */
			case AD_ACID:
				otmp = mksobj(ACID_VENOM, true, false);
				break;
		}
		otmp->spe = 1; /* to indicate it's yours */
		throwit(otmp, 0L, false, 0);
	}
	return 1;
}

int doremove(void) {
	if (!Punished) {
		pline("You are not chained to anything!");
		return 0;
	}
	unpunish();
	return 1;
}

int dospinweb(void) {
	struct trap *ttmp = t_at(u.ux, u.uy);

	if (Levitation || Is_airlevel(&u.uz) || Underwater || Is_waterlevel(&u.uz)) {
		pline("You must be on the ground to spin a web.");
		return 0;
	}
	if (u.uswallow) {
		pline("You release web fluid inside %s.", mon_nam(u.ustuck));
		if (is_animal(u.ustuck->data)) {
			expels(u.ustuck, u.ustuck->data, true);
			return 0;
		}
		if (is_whirly(u.ustuck->data)) {
			int i;

			for (i = 0; i < NATTK; i++)
				if (u.ustuck->data->mattk[i].aatyp == AT_ENGL)
					break;
			if (i == NATTK)
				impossible("Swallower has no engulfing attack?");
			else {
				char sweep[30];

				sweep[0] = '\0';
				switch (u.ustuck->data->mattk[i].adtyp) {
					case AD_FIRE:
						strcpy(sweep, "ignites and ");
						break;
					case AD_ELEC:
						strcpy(sweep, "fries and ");
						break;
					case AD_COLD:
						strcpy(sweep,
						       "freezes, shatters and ");
						break;
				}
				pline("The web %sis swept away!", sweep);
			}
			return 0;
		} /* default: a nasty jelly-like creature */
		pline("The web dissolves into %s.", mon_nam(u.ustuck));
		return 0;
	}
	if (u.utrap) {
		pline("You cannot spin webs while stuck in a trap.");
		return 0;
	}
	exercise(A_DEX, true);
	if (ttmp) switch (ttmp->ttyp) {
			case PIT:
			case SPIKED_PIT:
				pline("You spin a web, covering up the pit.");
				deltrap(ttmp);
				bury_objs(u.ux, u.uy);
				newsym(u.ux, u.uy);
				return 1;
			case SQKY_BOARD:
				pline("The squeaky board is muffled.");
				deltrap(ttmp);
				newsym(u.ux, u.uy);
				return 1;
			case TELEP_TRAP:
			case LEVEL_TELEP:
			case MAGIC_PORTAL:
				pline("Your webbing vanishes!");
				return 0;
			case WEB:
				pline("You make the web thicker.");
				return 1;
			case HOLE:
			case TRAPDOOR:
				pline("You web over the %s.",
				      (ttmp->ttyp == TRAPDOOR) ? "trap door" : "hole");
				deltrap(ttmp);
				newsym(u.ux, u.uy);
				return 1;
			case ROLLING_BOULDER_TRAP:
				pline("You spin a web, jamming the trigger.");
				deltrap(ttmp);
				newsym(u.ux, u.uy);
				return 1;
			case ARROW_TRAP:
			case DART_TRAP:
			case BEAR_TRAP:
			case ROCKTRAP:
			case FIRE_TRAP:
			case LANDMINE:
			case SLP_GAS_TRAP:
			case RUST_TRAP:
			case MAGIC_TRAP:
			case ANTI_MAGIC:
			case POLY_TRAP:
				pline("You have triggered a trap!");
				dotrap(ttmp, 0);
				return 1;
			default:
				impossible("Webbing over trap type %d?", ttmp->ttyp);
				return 0;
		}
	else if (On_stairs(u.ux, u.uy)) {
		/* cop out: don't let them hide the stairs */
		pline("Your web fails to impede access to the %s.",
		      (levl[u.ux][u.uy].typ == STAIRS) ? "stairs" : "ladder");
		return 1;
	}
	ttmp = maketrap(u.ux, u.uy, WEB);
	if (ttmp) {
		ttmp->madeby_u = 1;
		feeltrap(ttmp);
	}
	return 1;
}

int dosummon(void) {
	int placeholder;
	if (u.uen < 10) {
		pline("You lack the energy to send forth a call for help!");
		return 0;
	}
	u.uen -= 10;
	context.botl = 1;

	pline("You call upon your brethren for help!");
	exercise(A_WIS, true);
	if (!were_summon(youmonst.data, true, &placeholder, NULL))
		pline("But none arrive.");
	return 1;
}

#if 0
/* WAC supplanted by dogaze (). */
int
dogaze (void) {
	struct monst *mtmp;
	int looked = 0;
	char qbuf[QBUFSZ];
	int i;
	uchar adtyp = 0;

	for (i = 0; i < NATTK; i++) {
		if(youmonst.data->mattk[i].aatyp == AT_GAZE) {
			adtyp = youmonst.data->mattk[i].adtyp;
			break;
		}
	}
	if (adtyp != AD_CONF && adtyp != AD_FIRE) {
		impossible("gaze attack %d?", adtyp);
		return 0;
	}


	if (Blind) {
		pline("You can't see anything to gaze at.");
		return 0;
	}
	if (u.uen < 15) {
		pline("You lack the energy to use your special gaze!");
		return 0;
	}
	u.uen -= 15;
	context.botl = 1;

	for (mtmp = fmon; mtmp; mtmp = mtmp->nmon) {
		if (DEADMONSTER(mtmp)) continue;
		if (canseemon(mtmp) && couldsee(mtmp->mx, mtmp->my)) {
			looked++;
			if (Invis && !perceives(mtmp->data))
				pline("%s seems not to notice your gaze.", Monnam(mtmp));
			else if (mtmp->minvis && !See_invisible)
				pline("You can't see where to gaze at %s.", Monnam(mtmp));
			else if (mtmp->m_ap_type == M_AP_FURNITURE
			                || mtmp->m_ap_type == M_AP_OBJECT) {
				looked--;
				continue;
			} else if (flags.safe_dog && !Confusion && !Hallucination
			                && mtmp->mtame) {
				pline("You avoid gazing at %s.", y_monnam(mtmp));
			} else {
				if (flags.confirm && mtmp->mpeaceful && !Confusion
				                && !Hallucination) {
					sprintf(qbuf, "Really %s %s?",
					        (adtyp == AD_CONF) ? "confuse" : "attack",
					        mon_nam(mtmp));
					if (yn(qbuf) != 'y') continue;
					setmangry(mtmp);
				}
				if (!mtmp->mcanmove || mtmp->mstun || mtmp->msleeping ||
				                !mtmp->mcansee || !haseyes(mtmp->data)) {
					looked--;
					continue;
				}
				/* No reflection check for consistency with when a monster
				 * gazes at *you*--only medusa gaze gets reflected then.
				 */
				if (adtyp == AD_CONF) {
					if (!mtmp->mconf)
						pline("Your gaze confuses %s!", mon_nam(mtmp));
					else
						pline("%s is getting more and more confused.",
						      Monnam(mtmp));
					mtmp->mconf = 1;
				} else if (adtyp == AD_FIRE) {
					int dmg = d(2,6);
					pline("You attack %s with a fiery gaze!", mon_nam(mtmp));
					if (resists_fire(mtmp)) {
						pline("The fire doesn't burn %s!", mon_nam(mtmp));
						dmg = 0;
					}
					if((int) u.ulevel > rn2(20))
						destroy_mitem(mtmp, SCROLL_CLASS, AD_FIRE);
					if((int) u.ulevel > rn2(20))
						destroy_mitem(mtmp, POTION_CLASS, AD_FIRE);
					if((int) u.ulevel > rn2(25))
						destroy_mitem(mtmp, SPBOOK_CLASS, AD_FIRE);
					if (dmg && !DEADMONSTER(mtmp)) mtmp->mhp -= dmg;
					if (mtmp->mhp <= 0) killed(mtmp);
				}
				/* For consistency with passive() in uhitm.c, this only
				 * affects you if the monster is still alive.
				 */
				if (!DEADMONSTER(mtmp) &&
				                (mtmp->data==&mons[PM_FLOATING_EYE]) && !mtmp->mcan) {
					if (!Free_action) {
						pline("You are frozen by %s gaze!",
						      s_suffix(mon_nam(mtmp)));
						nomul((u.ulevel > 6 || rn2(4)) ?
						      -d((int)mtmp->m_lev+1,
						         (int)mtmp->data->mattk[0].damd)
						      : -200);
						return 1;
					} else
						pline("You stiffen momentarily under %s gaze.",
						      s_suffix(mon_nam(mtmp)));
				}
				/* Technically this one shouldn't affect you at all because
				 * the Medusa gaze is an active monster attack that only
				 * works on the monster's turn, but for it to *not* have an
				 * effect would be too weird.
				 */
				if (!DEADMONSTER(mtmp) &&
				                (mtmp->data==&mons[PM_MEDUSA]) && !mtmp->mcan) {
					pline("Gazing at the awake %s is not a very good idea.", l_monnam(mtmp));
					/* as if gazing at a sleeping anything is fruitful... */
					pline("You turn to stone...");
					killer.format = KILLED_BY;
					nhscopyz(&killer.name, "deliberately meeting Medusa's gaze");
					done(STONING);
				}
			}
		}
	}
	if (!looked) pline("You gaze at no place in particular.");
	return 1;
}
#endif

int dohide(void) {
	boolean ismimic = youmonst.data->mlet == S_MIMIC;

	if (u.uundetected || (ismimic && youmonst.m_ap_type != M_AP_NOTHING)) {
		pline("You are already hiding.");
		return 0;
	}
	if (ismimic) {
		/* should bring up a dialog "what would you like to imitate?" */
		youmonst.m_ap_type = M_AP_OBJECT;
		youmonst.mappearance = STRANGE_OBJECT;
	} else
		u.uundetected = 1;
	newsym(u.ux, u.uy);
	return 1;
}

int dopoly(void) {
	struct permonst *savedat = youmonst.data;

	if (is_vampire(youmonst.data) || is_vampshifter(&youmonst)) {
		polyself(2);
		if (savedat != youmonst.data) {
			pline("You transform into %s.", an(youmonst.data->mname));
			newsym(u.ux,u.uy);
		}
	}

	return 1;
}


int domindblast(void) {
	struct monst *mtmp, *nmon;

	if (u.uen < 10) {
		pline("You concentrate but lack the energy to maintain doing so.");
		return 0;
	}
	u.uen -= 10;
	context.botl = 1;

	pline("A wave of psychic energy pours out.");
	for (mtmp = fmon; mtmp; mtmp = nmon) {
		int u_sen;

		nmon = mtmp->nmon;
		if (DEADMONSTER(mtmp))
			continue;
		if (distu(mtmp->mx, mtmp->my) > BOLT_LIM * BOLT_LIM)
			continue;
		if (mtmp->mpeaceful)
			continue;
		u_sen = telepathic(mtmp->data) && !mtmp->mcansee;
		if (u_sen || (telepathic(mtmp->data) && rn2(3)) || !rn2(2)) {
			pline("You lock in on %s %s.", s_suffix(mon_nam(mtmp)),
			      u_sen ? "telepathy" :
				      telepathic(mtmp->data) ? "latent telepathy" :
							       "mind");
			mtmp->mhp -= rn1(4, 4);
			if (mtmp->mhp <= 0)
				killed(mtmp);
		}
	}
	return 1;
}

static void
uunstick() {
	pline("%s is no longer in your clutches.", Monnam(u.ustuck));
	setustuck(0);
}

void skinback(boolean silently) {
	if (uskin) {
		if (!silently) pline("Your skin returns to its original form.");
		uarm = uskin;
		uskin = NULL;
		/* undo save/restore hack */
		uarm->owornmask &= ~I_SPECIAL;
	}
}

const char *mbodypart(struct monst *mon, int part) {
	static const char
		*humanoid_parts[] = {"arm", "eye", "face", "finger",
				     "fingertip", "foot", "hand", "handed", "head", "leg",
				     "light headed", "neck", "spine", "toe", "hair",
				     "blood", "lung", "nose", "stomach"},
		*jelly_parts[] = {"pseudopod", "dark spot", "front",
				  "pseudopod extension", "pseudopod extremity",
				  "pseudopod root", "grasp", "grasped", "cerebral area",
				  "lower pseudopod", "viscous", "middle", "surface",
				  "pseudopod extremity", "ripples", "juices",
				  "surface", "sensor", "stomach"},
		*animal_parts[] = {"forelimb", "eye", "face", "foreclaw", "claw tip",
				   "rear claw", "foreclaw", "clawed", "head", "rear limb",
				   "light headed", "neck", "spine", "rear claw tip",
				   "fur", "blood", "lung", "nose", "stomach"},
		*bird_parts[] = {"wing", "eye", "face", "wing", "wing tip",
				 "foot", "wing", "winged", "head", "leg",
				 "light headed", "neck", "spine", "toe",
				 "feathers", "blood", "lung", "bill", "stomach"},
		*horse_parts[] = {"foreleg", "eye", "face", "forehoof", "hoof tip",
				  "rear hoof", "forehoof", "hooved", "head", "rear leg",
				  "light headed", "neck", "backbone", "rear hoof tip",
				  "mane", "blood", "lung", "nose", "stomach"},
		*sphere_parts[] = {"appendage", "optic nerve", "body", "tentacle",
				   "tentacle tip", "lower appendage", "tentacle", "tentacled",
				   "body", "lower tentacle", "rotational", "equator", "body",
				   "lower tentacle tip", "cilia", "life force", "retina",
				   "olfactory nerve", "interior"},
		*fungus_parts[] = {"mycelium", "visual area", "front", "hypha",
				   "hypha", "root", "strand", "stranded", "cap area",
				   "rhizome", "sporulated", "stalk", "root", "rhizome tip",
				   "spores", "juices", "gill", "gill", "interior"},
		*vortex_parts[] = {"region", "eye", "front", "minor current",
				   "minor current", "lower current", "swirl", "swirled",
				   "central core", "lower current", "addled", "center",
				   "currents", "edge", "currents", "life force",
				   "center", "leading edge", "interior"},
		*snake_parts[] = {"vestigial limb", "eye", "face", "large scale",
				  "large scale tip", "rear region", "scale gap", "scale gapped",
				  "head", "rear region", "light headed", "neck", "length",
				  "rear scale", "scales", "blood", "lung", "forked tongue", "stomach"},
		*fish_parts[] = {"fin", "eye", "premaxillary", "pelvic axillary",
				 "pelvic fin", "anal fin", "pectoral fin", "finned", "head", "peduncle",
				 "played out", "gills", "dorsal fin", "caudal fin",
				 "scales", "blood", "gill", "nostril", "stomach"};
	/* claw attacks are overloaded in mons[]; most humanoids with
	   such attacks should still reference hands rather than claws */
	static const char not_claws[] = {
		S_HUMAN, S_MUMMY, S_ZOMBIE, S_ANGEL,
		S_NYMPH, S_LEPRECHAUN, S_QUANTMECH, S_VAMPIRE,
		S_ORC, S_GIANT, /* quest nemeses */
		'\0'		/* string terminator; assert( S_xxx != 0 ); */
	};
	struct permonst *mptr = mon->data;

	if (part == HAND || part == HANDED) { /* some special cases */
		if (mptr->mlet == S_DOG || mptr->mlet == S_FELINE ||
		    mptr->mlet == S_YETI)
			return part == HAND ? "paw" : "pawed";
		if (humanoid(mptr) && attacktype(mptr, AT_CLAW) &&
		    !index(not_claws, mptr->mlet) &&
		    mptr != &mons[PM_STONE_GOLEM] &&
		    !is_foocubus(mptr))
			return part == HAND ? "claw" : "clawed";
	}
	if ((mptr == &mons[PM_MUMAK] || mptr == &mons[PM_MASTODON]) &&
	    part == NOSE)
		return "trunk";
	if (mptr == &mons[PM_SHARK] && part == HAIR)
		return "skin"; /* sharks don't have scales */
	if (mptr == &mons[PM_JELLYFISH] && (part == ARM || part == FINGER ||
					    part == HAND || part == FOOT || part == TOE))
		return "tentacle";
	if (mptr == &mons[PM_FLOATING_EYE] && part == EYE)
		return "cornea";
	if (humanoid(mptr) &&
	    (part == ARM || part == FINGER || part == FINGERTIP ||
	     part == HAND || part == HANDED))
		return humanoid_parts[part];
	if (mptr == &mons[PM_RAVEN])
		return bird_parts[part];
	if (mptr->mlet == S_CENTAUR || mptr->mlet == S_UNICORN ||
	    (mptr == &mons[PM_ROTHE] && part != HAIR))
		return horse_parts[part];
	if (mptr->mlet == S_LIGHT) {
		if (part == HANDED)
			return "rayed";
		else if (part == ARM || part == FINGER ||
			 part == FINGERTIP || part == HAND)
			return "ray";
		else
			return "beam";
	}
	if (mptr->mlet == S_EEL && mptr != &mons[PM_JELLYFISH])
		return fish_parts[part];
	if (slithy(mptr) || (mptr->mlet == S_DRAGON && part == HAIR))
		return snake_parts[part];
	if (mptr->mlet == S_EYE)
		return sphere_parts[part];
	if (mptr->mlet == S_JELLY || mptr->mlet == S_PUDDING ||
	    mptr->mlet == S_BLOB || mptr == &mons[PM_JELLYFISH])
		return jelly_parts[part];
	if (mptr->mlet == S_VORTEX || mptr->mlet == S_ELEMENTAL)
		return vortex_parts[part];
	if (mptr->mlet == S_FUNGUS)
		return fungus_parts[part];
	if (humanoid(mptr))
		return humanoid_parts[part];
	return animal_parts[part];
}

const char *
body_part(int part) {
	return mbodypart(&youmonst, part);
}

int poly_gender(void) {
	/* Returns gender of polymorphed player; 0/1=same meaning as flags.female,
	 * 2=none.
	 */
	if (is_neuter(youmonst.data) || !humanoid(youmonst.data)) return 2;
	return flags.female;
}

void ugolemeffects(int damtype, int dam) {
	int heal = 0;
	/* We won't bother with "slow"/"haste" since players do not
	 * have a monster-specific slow/haste so there is no way to
	 * restore the old velocity once they are back to human.
	 */
	if (u.umonnum != PM_FLESH_GOLEM && u.umonnum != PM_IRON_GOLEM)
		return;
	switch (damtype) {
		case AD_ELEC:
			if (u.umonnum == PM_FLESH_GOLEM)
				heal = dam / 6; /* Approx 1 per die */
			break;
		case AD_FIRE:
			if (u.umonnum == PM_IRON_GOLEM)
				heal = dam;
			break;
	}
	if (heal && (u.mh < u.mhmax)) {
		u.mh += heal;
		if (u.mh > u.mhmax) u.mh = u.mhmax;
		context.botl = 1;
		pline("Strangely, you feel better than before.");
		exercise(A_STR, true);
	}
}

static int armor_to_dragon(int atyp) {
	switch (atyp) {
		case GRAY_DRAGON_SCALE_MAIL:
		case GRAY_DRAGON_SCALES:
			return PM_GRAY_DRAGON;
		case SILVER_DRAGON_SCALE_MAIL:
		case SILVER_DRAGON_SCALES:
			return PM_SILVER_DRAGON;
		case SHIMMERING_DRAGON_SCALE_MAIL:
		case SHIMMERING_DRAGON_SCALES:
			return PM_SHIMMERING_DRAGON;
		case DEEP_DRAGON_SCALE_MAIL:
		case DEEP_DRAGON_SCALES:
			return PM_DEEP_DRAGON;
		case RED_DRAGON_SCALE_MAIL:
		case RED_DRAGON_SCALES:
			return PM_RED_DRAGON;
		case ORANGE_DRAGON_SCALE_MAIL:
		case ORANGE_DRAGON_SCALES:
			return PM_ORANGE_DRAGON;
		case WHITE_DRAGON_SCALE_MAIL:
		case WHITE_DRAGON_SCALES:
			return PM_WHITE_DRAGON;
		case BLACK_DRAGON_SCALE_MAIL:
		case BLACK_DRAGON_SCALES:
			return PM_BLACK_DRAGON;
		case BLUE_DRAGON_SCALE_MAIL:
		case BLUE_DRAGON_SCALES:
			return PM_BLUE_DRAGON;
		case GREEN_DRAGON_SCALE_MAIL:
		case GREEN_DRAGON_SCALES:
			return PM_GREEN_DRAGON;
		case YELLOW_DRAGON_SCALE_MAIL:
		case YELLOW_DRAGON_SCALES:
			return PM_YELLOW_DRAGON;
		default:
			return -1;
	}
}

static struct {
	int mon;
	int reqtime;
	boolean merge;
} draconic;

/* called each move during transformation process */
static int
mage_transform() {
	if (--draconic.reqtime)
		return 1;
	if (draconic.merge)
		merge_with_armor();
	polymon(draconic.mon);
	return 0;
}

/* Polymorph under conscious control (#youpoly) */
int polyatwill(void) {
#define EN_DOPP		20 /* This is the "base cost" for a polymorph
				 * Actual cost is this base cost + 5 * monster level
				 * of the final form you actually assume.
				 * Energy will be taken first, then you will get
				 * more hungry if you do not have enough energy.
				 */
#define EN_WERE		10
#define EN_BABY_DRAGON	10
#define EN_ADULT_DRAGON 15

	boolean scales = ((uarm && uarm->otyp == RED_DRAGON_SCALES && Role_if(PM_FLAME_MAGE)) ||
			  (uarm && uarm->otyp == WHITE_DRAGON_SCALES && Role_if(PM_ICE_MAGE)));
	boolean scale_mail = ((uarm && uarm->otyp == RED_DRAGON_SCALE_MAIL && Role_if(PM_FLAME_MAGE)) ||
			      (uarm && uarm->otyp == WHITE_DRAGON_SCALE_MAIL && Role_if(PM_ICE_MAGE)));

	/* KMH, balance patch -- new intrinsic */
	if (Unchanging) {
		pline("You cannot change your form.");
		return 0;
	}

	/* First, if in correct polymorphed form, rehumanize (for free)
	 * Omit Lycanthropes,  who need to spend energy to change back and forth
	 */
	if (Upolyd && (Race_if(PM_DOPPELGANGER) ||
		       (Role_if(PM_FLAME_MAGE) && (u.umonnum == PM_RED_DRAGON ||
						   u.umonnum == PM_BABY_RED_DRAGON)) ||
		       (Role_if(PM_ICE_MAGE) && (u.umonnum == PM_WHITE_DRAGON ||
						 u.umonnum == PM_BABY_WHITE_DRAGON)))) {
		rehumanize();
		return 1;
	}

	if ((Role_if(PM_ICE_MAGE) || Role_if(PM_FLAME_MAGE)) &&
	    (u.ulevel > 6 || scale_mail)) {
		/* [ALI]
		 * I've rewritten the logic here to fix the failure messages,
		 * but the requirements for polymorphing into the two dragon
		 * forms remains the same:
		 *
		 * Polymorph into adult dragon form if one of:
		 *
		 * - Wearing scale mail (no charge).
		 * - Wearing scales and experience level 7 and
		 *   energy level 11 or more (charge is 10).
		 * - Not wearing scales or scale mail and experience level 14 and
		 *   energy level 16 or more (charge is 15).
		 *
		 * Polymorph into baby dragon form if one of:
		 *
		 * - Wearing scales and experience level 7 and
		 *   energy level 10 or less (no charge).
		 * - Not wearing scales or scale mail and experience level 14 and
		 *   energy level 11-15 (charge is 10).
		 * - Not wearing scales or scale mail and experience level 7-13 and
		 *   energy level 11 or more (charge is 10).
		 *
		 * Fail if one of:
		 *
		 * - Not wearing scales or scale mail and experience level 7 and
		 *   energy level 10 or less (not enough energy).
		 * - Not wearing scale mail and experience level 6 or less
		 *   (not experienced enough).
		 *
		 * The transformation takes a few turns. If interrupted during this
		 * period then the ritual must be begun again from the beginning.
		 * We deliberately don't say what form the ritual takes since it
		 * is unaffected by blindness, confusion, stun etc.
		 */
		if (yn("Transform into your draconic form?") == 'n')
			return 0;
		else if (!scales && !scale_mail && u.uen <= EN_BABY_DRAGON) {
			pline("You don't have the energy to polymorph.");
			return 0;
		} else {
			/* Check if you can do the adult form */
			if ((u.ulevel > 13 && u.uen > EN_ADULT_DRAGON) ||
			    (scales && u.uen > EN_BABY_DRAGON) || scale_mail) {
				/* If you have scales, energy cost is less */
				/* If you have scale mail,  there is no cost! */
				if (!scale_mail) {
					if (scales)
						u.uen -= EN_BABY_DRAGON;
					else
						u.uen -= EN_ADULT_DRAGON;
				}

				draconic.mon = Role_if(PM_FLAME_MAGE) ?
						       PM_RED_DRAGON :
						       PM_WHITE_DRAGON;
				draconic.merge = scales || scale_mail;
				/* Otherwise use the baby form */
			} else {
				if (!scales) u.uen -= EN_BABY_DRAGON;

				draconic.mon = Role_if(PM_FLAME_MAGE) ?
						       PM_BABY_RED_DRAGON :
						       PM_BABY_WHITE_DRAGON;
				draconic.merge = scales;
			}
			draconic.reqtime = 2;
			if (mvitals[draconic.mon].mvflags & G_GENOD)
				draconic.merge = false;
			set_occupation(mage_transform,
				       "transforming into your draconic form", 0);
			pline("You begin the transformation ritual.");
			return 1;
		}
	}
	if (Race_if(PM_DOPPELGANGER)) {
		if (yn("Polymorph at will?") == 'n')
			return 0;
		else if (u.uen < EN_DOPP) {
			pline("You don't have the energy to polymorph!");
			return 0;
		} else {
			u.uen -= EN_DOPP;
			if (multi >= 0) {
				if (occupation)
					stop_occupation();
				else
					nomul(0);
			}
			polyself(0);
			if (Upolyd) { /* You actually polymorphed */
				u.uen -= 5 * mons[u.umonnum].mlevel;
				if (u.uen < 0) {
					morehungry(-u.uen);
					u.uen = 0;
				}
			}
			return 1;
		}
	} else if (Race_if(PM_HUMAN_WEREWOLF) &&
		   (!Upolyd || u.umonnum == u.ulycn)) {
		if (yn("Change form?") == 'n')
			return 0;
		else if (u.ulycn == NON_PM) {
			/* Very serious */
			pline("You are no longer a lycanthrope!");
		} else if (u.ulevel <= 2) {
			pline("You can't invoke the change at will yet.");
			return 0;
		} else if (u.uen < EN_WERE) {
			pline("You don't have the energy to change form!");
			return 0;
		} else {
			/* Committed to the change now */
			u.uen -= EN_WERE;
			if (!Upolyd) {
				if (multi >= 0) {
					if (occupation)
						stop_occupation();
					else
						nomul(0);
				}
				you_were();
			} else {
				rehumanize();
			}
			return 1;
		}
	} else {
		pline("You can't polymorph at will%s.",
		      ((Role_if(PM_FLAME_MAGE) || Role_if(PM_ICE_MAGE) ||
			Race_if(PM_HUMAN_WEREWOLF) || Race_if(PM_DOPPELGANGER)) ?
			       " yet" :
			       ""));
		return 0;
	}

	context.botl = 1;
	return 1;
}

/*
 * Some species have awareness of other species
 */
static bool polysense(struct permonst *mptr) {
	short warnidx = 0;

	context.warntype.speciesidx = 0;
	context.warntype.species = 0;
	context.warntype.polyd = 0;

	switch (monsndx(mptr)) {
		case PM_PURPLE_WORM:
			warnidx = PM_SHRIEKER; // just for fun :)
			break;
		case PM_VAMPIRE:
		case PM_VAMPIRE_LORD:
		case PM_VAMPIRE_MAGE:
			context.warntype.polyd = M2_HUMAN | M2_ELF;
			HWarn_of_mon |= FROMRACE;
			return true;
	}

	if (warnidx) {
		context.warntype.speciesidx = warnidx;
		context.warntype.species = &mons[warnidx];
		HWarn_of_mon |= FROMRACE;
		return true;
	}

	context.warntype.speciesidx = 0;
	context.warntype.species = 0;
	HWarn_of_mon &= ~FROMRACE;

	return false;
}

static void merge_with_armor(void) {
	/* This function does hides the armor being worn
	 * It currently assumes that you are changing into a dragon
	 * Should check that monster being morphed into is not genocided
	 * see do_merge above for correct use
	 */
	if (Race_if(PM_DOPPELGANGER) && !uarm->cursed && uarmu &&
	    !uarmu->cursed) {
		struct obj *otmp = uarmu;
		pline("You quickly remove your shirt as you start to change.");
		setworn(NULL, otmp->owornmask & W_ARMU);
		dropx(otmp);
	}
	pline("You merge with your scaly armor.");
	uskin = uarm;
	uarm = NULL;
	/* save/restore hack */
	uskin->owornmask |= I_SPECIAL;
}

/*polyself.c*/
