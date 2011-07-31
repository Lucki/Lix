/*
 * lemupd.cpp
 *
 * Was tut ein Lixxie in einem Update? Dies ist allerdings nur der grosse
 * Rahmen, der das Zusammenspiel aller Lixxiee mit den interaktiven Objekten
 * regelt. Einzel-Landbearbeitung findet in den Funktionen statt, die ueber
 * Lixxie::update() aufgerufen werden.
 *
 */

#include <cmath> // Knockback-Berechnung

#include "../gameplay/gameplay.h"
#include "../graphic/sound.h"
#include "../other/globals.h"

void Gameplay::update_lix(Lixxie& l, const UpdateArgs& ua)
{
    if (l.get_ac() == LixEn::NOTHING) return;

    // Exploder-Dinge separat!
    // 76 statt 75 Updates sind n�tig, weil 0->1->2 im selben Tick passiert.
    if (l.get_updates_since_bomb() > 0 && l.get_ac() != LixEn::EXPLODER) {
        l.inc_updates_since_bomb();
        // Exploder a la L2 mit Knockback und sofortigem Tod
        if (l.get_updates_since_bomb() == 76 && l.get_exploder_knockback()) {
            // Der Wert -6 (etwas ueber Fusshoehe) stammt aus L2-Screenshots
            make_knockback_explosion(ua.upd, l.get_tribe(),
             ua.id, l.get_ex(), l.get_ey() - 6);
            l.play_sound(ua, Sound::POP);
            l.set_ac(LixEn::NOTHING);
            --l.get_tribe().lix_out;
            return;
            // Death, don't do anything else now
        }
        // Normaler Exploder
        else if (l.get_updates_since_bomb() == 76)
        {
            l.set_updates_since_bomb(0);
            // Sowohl Climber als auch Ascender sollen entweder beide herunter
            // fallen oder beide in der Wand Oh-no-en.
            if      (l.get_ac() == LixEn::CLIMBER)  l.move_ahead(2);
            else if (l.get_ac() == LixEn::ASCENDER) l.move_ahead(-2);
            // Alles, was in der Luft ist, soll sofort explodieren
            if (l.get_ac() == LixEn::FALLER
             || l.get_ac() == LixEn::TUMBLER
             || l.get_ac() == LixEn::STUNNER && l.get_frame() < 13
             || l.get_ac() == LixEn::FLOATER
             || l.get_ac() == LixEn::JUMPER) {
                l.assign(LixEn::EXPLODER);
                l.set_frame(15);
            }
            else {
                bool b = (l.get_ac() == LixEn::BLOCKER);
                l.assign(LixEn::EXPLODER);
                l.set_frame(-1); // weil assign immer auf Frame 2 setzt
                if (b) l.set_special_x(1); // Damit der Exploder weiterblockt
                if (!l.get_tribe().nuke) l.play_sound(ua, Sound::OHNO);
            }
        }
    }



    // Haupt-Abarbeitung
    // Lixes that already did NOTHING escaped the current function at the top.
    l.update(ua);
    if (l.get_ac() == LixEn::NOTHING) {
        --l.get_tribe().lix_out;
        return;
    }



    // Blocker: Abarbeiten anderer Lixxiee geschieht hier, weil dies ja
    // nicht nur den einen Lixxie betrifft
    if (l.get_ac() == LixEn::BLOCKER
     || l.get_ac() == LixEn::EXPLODER && l.get_special_x() == 1)
     update_lix_blocker(l);



    // Ziele
    update_lix_goals(l, ua);
    // Ein Exiter, der seine Animation durchlaufen hat, verschwindet.
    // special_x gibt gerade das an.
    if (l.get_ac() == LixEn::EXITER && l.get_special_x() == 1) {
        if (goal[l.get_special_y()].has_tribe(&l.get_tribe())) {
            ++l.get_tribe().lix_saved;
            if (cs.tribes.size() == 1) {
                update_last_exiter = cs.update;
                if (l.get_tribe().lix_saved == level.required)
                 l.play_sound(ua, Sound::YIPPIE);
            }
        }
        else
         for (Tribe::It atr = cs.tribes.begin(); atr != cs.tribes.end(); ++atr)
         if (goal[l.get_special_y()].has_tribe(&*atr)) ++atr->lix_saved;
        l.set_ac(LixEn::NOTHING);
        --l.get_tribe().lix_out;
        return;
    }



    // Fallen
    for (IacIt i = cs.trap.begin(); i != cs.trap.end(); ++i) {
        if (l.get_in_trigger_area(*i)
         && i->get_x_frame() == 0 && i->get_y_frame() == 0) {
            i->set_y_frame(1);
            l.play_sound(ua, i->get_object()->sound);
            l.set_ac(LixEn::NOTHING);
            --l.get_tribe().lix_out;
            return;
        }
    }



    // Feuer und Wasser
    if (l.get_ac() != LixEn::BURNER
     && l.get_ac() != LixEn::DROWNER)
     for (IacIt i =  special[Object::WATER].begin();
                i != special[Object::WATER].end(); ++i)
     if (l.get_in_trigger_area(*i)) {
        int fire = i->get_object()->subtype;
        if (!fire) l.set_ey(i->get_y() + i->get_object()->trigger_y + 2);
        l.assign(fire ? LixEn::BURNER : LixEn::DROWNER);
        // Nicht explodieren lassen, das t�te er bei 76 :-)
        if (l.get_updates_since_bomb() == 75) l.set_updates_since_bomb(0);
        l.play_sound(ua, fire ? Sound::FIRE : Sound::WATER);
    }



    // Flinging
    if (l.get_nukable())
     for (IacIt i = cs.fling.begin(); i != cs.fling.end(); ++i)
     if (l.get_in_trigger_area(*i)) {
        int sub = i->get_object()->subtype;
        // non-constant?
        if (sub & 2) {
            if (i->get_x_frame() == 0 && i->get_y_frame() == 0)
             i->set_y_frame(1);
            else continue;
        }
        int dir = l.get_dir();
        l.assign(LixEn::TUMBLER);
        int special_x = i->get_object()->special_x;
        if (!(sub & 1)) special_x *= dir;
        l.set_special_x(std::abs(special_x));
        l.set_special_y(i->get_object()->special_y);
        l.set_dir(special_x);
        //l.play_sound(ua.upd, fire ? Sound::FIRE : Sound::WATER);
    }



    // Lixxie aus dem Bild?
    if (l.get_ey() >= map.get_yl() + 23
     || l.get_ey() >= map.get_yl() + 15 && l.get_ac() != LixEn::FLOATER
     || l.get_ex() >= map.get_xl() +  4
     || l.get_ey() < -1 && !l.get_pass_top() // -1 because ey is 2 over floor
     || l.get_ex() < -4) {
        l.play_sound(ua, Sound::OBLIVION);
        l.set_ac(LixEn::NOTHING);
        --l.get_tribe().lix_out;
        return;
    }

}





void Gameplay::update_lix_blocker(Lixxie& l)
{
    for (Tribe::It t = cs.tribes.begin(); t != cs.tribes.end(); ++t)
     for (LixIt i = t->lixvec.begin(); i != t->lixvec.end(); ++i) {
        int distance_side = i->get_ac() == LixEn::MINER ? block_m : block_s;
        if (i->get_ac() != LixEn::BLOCKER &&
            i->get_ac() != LixEn::EXPLODER) {
            const int dx = map.distance_x(i->get_ex(), l.get_ex());
            const int dy = map.distance_x(i->get_ey(), l.get_ey());
            if (dx > - distance_side && dx < distance_side
             && dy > - block_d       && dy < block_u) {
                if (i->get_dir() > 0 && dx > 0
                 || i->get_dir() < 0 && dx < 0) {
                    i->turn();
                    // Platformer drehten sonst um, hoeren auf, drehen erneut
                    if (i->get_ac() == LixEn::PLATFORMER) {
                        i->assign(LixEn::WALKER); // = mit kurzem Aufstehen
                        i->set_frame(i->get_frame() - 1);
}   }   }   }   }   }



void Gameplay::update_lix_goals(Lixxie& l, const UpdateArgs& ua)
{
    // false: Keine persoenlichen Einstellungen wie !multbuilders beachten
    if (l.get_priority(LixEn::EXITER, cs.tribes.size(), goal, false) > 1)
     for (int i = 0; i < (int) goal.size(); ++i)
     if (l.get_in_trigger_area(goal[i])) {
        // Lixxie soll ins Ziel gehen und sich merken, in welches
        l.assign(LixEn::EXITER);
        l.set_special_y(i);
        // Wem gehoert das Ziel, um den Sound abzuspielen? Diese Kontrolle
        // wird beim Exiter nochmal gemacht, wenn der Lixxie verschwindet.
        // Sound is always played as if the player was the local player.
        // No sound is played if the player is not involved.
        if (goal[i].has_tribe(trlo))
         effect.add_sound(ua.upd, *trlo, ua.id, Sound::GOAL);
        else if (&l.get_tribe() == trlo)
         effect.add_sound(ua.upd, *trlo, ua.id, Sound::GOAL_BAD);
}   }



void Gameplay::make_knockback_explosion(
    const unsigned long upd,
    const Tribe& t,
    const unsigned lem_id,
    const int x,
    const int y)
{
    // Es gibt keine Lixxie-Funktion fuer den Kreis, nur fuer Rechtecke.
    // Also machen wir's ganz geschickt: Wir nehmen das kleinste Quadrat, was
    // den Kreis enthaelt, arbeiten es per Schleife ab und entfernen alle Pixel
    // mit gen�gend kleinem Abstand zum Mittelpunkt.
    const int radius = 23; // Diese Zahl stammt aus L2-Screenshots
    for  (int lx = -radius; lx  <=  radius; ++lx) // lx = Loop-X
     for (int ly = -radius; ly  <=  radius; ++ly)
     if  (map.hypot(lx, ly, 0, 0) <= (radius + 0.5))
     Lixxie::remove_pixel_absolute(x + lx, y + ly + 1); // +1, da Radius%2 = 1

    // Knockback
    // Fuer komische Werte siehe alle Kommentare zum lustigen Hochfliegen.
    const double strength_x = 0.8;
    const double strength_y = 0.9; // u. A. (!) ist dies fuer lustiges Fliegen
    const double range      = radius * 2.5 + 0.5;
    for (Tribe::It titr = cs.tribes.begin(); titr != cs.tribes.end(); ++titr)
     for (LixIt i = titr->lixvec.begin(); i != titr->lixvec.end(); ++i) {
        // Ausnahme:
        if (!i->get_nukable()) continue;
        // Mehr lustiges Hochfliegen durch die 10 tiefere Explosion!
        const int dx = map.distance_x(x,      i->get_ex());
        const int dy = map.distance_y(y + 10, i->get_ey());
        const double distance = map.hypot(dx, dy, 0, 0);
        if (distance <= range) {
            const int sx = (int) (dx * strength_x * (1 - distance / range));
            const int sy = (int) (dy * strength_y * (1 - distance / range));
            i->assign       (LixEn::TUMBLER);
            i->set_dir      (dx);
            i->set_special_x(sx/i->get_dir());
            i->set_special_y(sy - 6); // Mehr lustiges Hochfliegen!
        }
    }
    effect.add_explosion(upd, t, lem_id, x, y);
}