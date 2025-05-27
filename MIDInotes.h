//)if status = Note On, create new MIDInote with channel and MIDI note
//// (in the eventual score editor, this will also instantiate a value-less notehead
//// at the current time relative to the current bar, to be value-defined later with NoteVector::printNotes)
//)add new MIDInote to NoteVector
//)if status = Note Off or velocity = 0, sift through NoteVector
//// for MIDInote with corresponding channel and pitch and set MIDInote.m_on to false;
//)for every ensuing non-zero delta time value,
//// += every MIDInote.m_rhythm with the new value
//// and increment endex (for NoteVector::printNotes)
//)when track arrives at the first of a set of meta events, or at the end of the track,
//// print all MIDInotes between m_index and m_endex in "pitch-octave metric-rhythm" format

#pragma once

#include <vector>
#include <sstream>
#include <iostream>
#include <array>
#include <string>

//create class to hold separate pitch octave info

class Pitch8ve
{
public:
    explicit Pitch8ve(int midinote=0)
        : m_midinote{ midinote }
        , m_pitch{ midinote % 12 }
        , m_octave{ (midinote / 12) - 1 }
    {
    }

    int MIDInote() const { return m_midinote; }

    friend std::ostream& operator<< (std::ostream& out, const Pitch8ve& p8);

private:
    //store unparsed MIDI note number byte for comparison
    int m_midinote{};

    int m_pitch{};
    int m_octave{};
};

//define Pitch8ve friend function to print Pitch8ve in pitch-octave format

inline std::string_view toPitch(int pitch)
{
    switch (pitch)
    {
    case 0: return "C";
    case 1: return "C#";
    case 2: return "D";
    case 3: return "D#";
    case 4: return "E";
    case 5: return "F";
    case 6: return "F#";
    case 7: return "G";
    case 8: return "G#";
    case 9: return "A";
    case 10: return "A#";
    case 11: return "B";
    default: return "??";
    }
}

inline std::ostream& operator<< (std::ostream& out, const Pitch8ve& p8)
{
    out << toPitch(p8.m_pitch) << p8.m_octave;

    return out;
}

//create class to store MIDI note info;
//default rhythm to 0 since every new MIDI note has no duration yet

class MIDInote
{
public:
    MIDInote(int channel, int pitch, int rhythm=0)
        : m_channel{ channel & 0x0f }
        , m_pitch{ pitch }
        , m_rhythm{ rhythm }
    {
    }

    void hold(int delta) { m_rhythm += delta; }

    void turnOff() { m_on = false; }

    bool isOn() { return m_on; }

    int channel() { return m_channel; }

    Pitch8ve getPitch() { return m_pitch; }

    void setRhythm(int r) { m_rhythm = r; }

    //return rhythm in relation to ticks per quarter note (derived from the header)
    double getRhythm(short quarter_note) { return static_cast<double>(m_rhythm) / quarter_note; }

private:
    int m_channel{};

    Pitch8ve m_pitch{};
    int m_rhythm{};

    bool m_on{ true };
};

//create class to hold all MIDI notes recorded in the track

class NoteVector
{
public:
    void addNote(MIDInote m) { m_notes.push_back( m ); }

    void addDelta(int d);

    void noteOff(int status, int pitch);

    void printNotes(short quarter_note);

private:
    std::vector<MIDInote> m_notes{};

    std::size_t m_index{ 0 };
    std::size_t m_endex{ 0 };
};

//define NoteVector member functions

inline void NoteVector::addDelta(int d)
{
    for ( auto& n : m_notes )
    {
        //only increment m_rhythm if the note is still on
        if ( n.isOn() )
        {
            n.hold(d);
        }
    }
}

inline void NoteVector::noteOff(int status, int pitch)
{
    for ( auto& n : m_notes )
    {
        if ( n.isOn() )
        {
            if ( n.channel() == (status & 0x0f) && n.getPitch().MIDInote() == pitch )
            {
                n.turnOff();
            }
        }
    }
}

//NoteVector::printNotes will only print if index < endex;
//create method to ensure that multiple meta events occurring at the same time will not trigger reprint;
//for example, only implement NoteVector::addDelta() if the delta time byte > 0, in which case ++endex
inline void NoteVector::printNotes(short quarter_note)
{
    if (m_index != std::size(m_notes) )
        std::cout << "MIDI Notes:\n";

    while( m_index < std::size(m_notes) )
    {
        std::cout << m_notes[m_index].getPitch() << ' ';

        //if this note is still on, it is being held over a meta event,
        //which could be, for instance, a time signature or tempo change
        if ( m_notes[m_index].isOn() )
        {
            std::cout << '(' << m_notes[m_index].getRhythm( quarter_note ) << ")\n";

            //in the eventual score, this note will be tied over

            //set rhythm back to 0 to specify later for how many more quarter notes to hold the note after the meta event
            m_notes[m_index].setRhythm( 0 );
        }
        else
        {
            std::cout << m_notes[m_index].getRhythm( quarter_note ) << '\n';
        }

        ++m_index;
    } 
}

//will eventually need to reconcile a method of writing moving notes over held ones