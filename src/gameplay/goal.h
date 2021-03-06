/*
 * gameplay/goal.h
 *
 * Ein Ziel ist ein Grafikobjekt, das im Mehrspielermodus zusaetzlich die
 * Farben der Spieler zeigt, denen eintretende Lixen gutgeschrieben werden.
 *
 * Ferner bietet diese Klasse, aehnlich wie die anderen interaktiven Objekte,
 * eine Update-Methode zum Wechseln auf das naechste Frame.
 *
 */

#pragma once

#include <vector>

#include "tribe.h"

#include "../editor/graph_ed.h"

class Goal : public EdGraphic {

private:

    std::vector <const Tribe*> tribes;

public:

    typedef std::vector <Goal> ::iterator         It;
    typedef std::vector <Goal> ::reverse_iterator RIt;
    typedef std::vector <Goal> ::const_iterator   CIt;
    typedef std::vector <const Tribe*>            TribeVec;

             Goal(Torbit&, const Object*, const int = 0, const int = 0);
    virtual ~Goal();

    bool                   has_tribe (const Tribe* t) const;
    inline const TribeVec& get_tribes() const         { return tribes;       }
    inline void            add_tribe (const Tribe* t) { tribes.push_back(t); }

    //                draw owners, draw no-sign
    virtual void draw(bool, bool);

};
