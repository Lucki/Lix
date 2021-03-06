/*
 * lix/jumper.cpp
 *
 */

#include "ac.h"

// will be defined in tumbler.cpp
int  jumper_and_tumbler_collision(Lixxie&);
void tumbler_frame_selection     (Lixxie&);



void become_jumper(Lixxie& l)
{
    l.become_default(LixEn::JUMPER);
    if (l.get_runner()) {
        l.set_special_x(  8); // X-speed
        l.set_special_y(-12); // Y-speed
        l.set_frame(12);
    }
    else {
        l.set_special_x( 6); // X-speed
        l.set_special_y(-8); // Y-speed
    }

    for (int i = -4; i > -16; --i)
     if (l.is_solid(0, i)) {
        l.become(LixEn::STUNNER);
        return;
    }
}



void update_jumper(Lixxie& l, const UpdateArgs& ua)
{
    if (l.get_special_x() % 2 == 1) {
        if (l.get_special_x() > 0) l.set_special_x(l.get_special_x() + 1);
        else                       l.set_special_x(l.get_special_x() - 1);
    }
    const int       sgn = l.get_special_y() >= 0 ? 1 : -1; // sgn(Y-Spd.)
    const unsigned  abs = l.get_special_y() / sgn;         // |Y-Spd.|
    const unsigned& spe = l.get_special_x();               // X-Spd.
    // Das Signum ist auch fuer 0 gleich +1, das macht im Code allerdings
    // nix, weil in dem Fall null Schleifendurchlaeufe passieren oder
    // mit null multipliziert wird.

    // Schrittweise schraeg vorruecken und jedes Mal auf
    // Kollision mit der Landschaft testen.
    // Die groessere Laenge wird pixelweise abgearbeitet.
    for (unsigned i = 0; i < (abs >= spe ? abs : spe); ++i) {
        // 2 * (... / 2) sorgt fuer das Einhalten der geraden X-Zahlen.
        // Es wird jeweils geguckt, ob der Zaehler i weit genug vor-
        // geschritten ist, damit in die kurze Richtung ein
        // Schritt stattfinden kann - wenn nicht, ist das Argument
        // von move_ahead stets 0. In die lange Richtung geht's immer.
        const int old_ex = l.get_ex();
        const int old_ey = l.get_ey();
        const Lookup::LoNr old_enc_foot = l.get_foot_encounters();
        const Lookup::LoNr old_enc_body = l.get_body_encounters();

        if (abs >= spe) {
            l.move_ahead(2 * ((i+1)*spe/2 / abs - i*spe/2 / abs));
            l.move_down(sgn);
        }
        else {
            l.move_down(sgn * ((i+1)*abs/spe - i*abs/spe));
            l.move_ahead(i%2 * 2);
        }

        int coll = jumper_and_tumbler_collision(l);
        if (coll == 0) {
            // nothing hit, proceed with next step for this frame
        }
        else if (coll == 1) {
            // we hit something, but don't have to reset anything. Stop motion.
            break;
        }
        else if (coll == 2) {
            // We hit something, reset encounters and check again at position.
            l.set_foot_encounters(old_enc_foot);
            l.set_body_encounters(old_enc_body);
            l.set_ey(l.get_ey()); // re-check encounters at current position
            break;
        }
        else if (coll == 3) {
            // Like coll == 2, but also reset position.
            l.set_foot_encounters(old_enc_foot);
            l.set_body_encounters(old_enc_body);
            l.set_ex(old_ex);
            l.set_ey(old_ey);
            // Do this for completely immobilizied tumblers
            if (l.is_solid(0, 0)) l.become(LixEn::STUNNER);
            break;
        }
        // end checking return val of collision
    }
    // end of motion

    // Wenn nicht beim Bewegen irgendwo angestossen...
    if (l.get_ac() == LixEn::JUMPER
     || l.get_ac() == LixEn::TUMBLER) {
        const int sp_y = l.get_special_y();
        if      (sp_y <= 12) l.set_special_y(sp_y + 2);
        else if (sp_y <  64) l.set_special_y(sp_y + 1);

        if (l.get_ac() == LixEn::TUMBLER
         && (l.get_foot_encounters() & Lookup::bit_trampoline)
         && l.get_special_y() > 0) {
            // don't use the tumbler frame selection that is used onwards here,
            // but take a nicer frame
            l.set_frame(std::max(0, l.get_frame() - 4));
        }
        else if (l.get_special_y() > 14) {
            if (l.get_floater()) {
                const int sx = l.get_special_x();
                const int sy = l.get_special_y();
                l.become(LixEn::FLOATER);
                l.set_special_x(sx);
                l.set_special_y(sy);
            }
            else if (l.get_ac() == LixEn::JUMPER)  {
                l.set_ac(LixEn::TUMBLER);
                l.set_frame(3); // override autoframechoice for 1 frame now
            }
            else { // we're already a tumbler
                tumbler_frame_selection(l);
            }
        }
        else if (l.get_ac() == LixEn::TUMBLER) {
            tumbler_frame_selection(l);
        }
        else {
            // we are a jumper
            if (l.is_last_frame()) {
                if (l.get_runner()) l.set_frame(12);
                else                l.set_frame(l.get_frame() - 1);
            }
            else l.next_frame();
        }
    }

    switch (l.get_ac()) {
        case LixEn::STUNNER:  l.play_sound(ua, Sound::OUCH);  break;
        case LixEn::SPLATTER: l.play_sound(ua, Sound::SPLAT); break;
        case LixEn::CLIMBER:  l.play_sound(ua, Sound::CLIMBER); break;
        default: break;
    }
}

