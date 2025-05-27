//structure of event: <delta time><event> = <delta time><status byte><data byte>+
//ignore all system common/exclusive messages (status bytes F0-FE [240-254])
//only do something with status bytes of 80-EF or FF (128-239 or 255)

//for MIDI events, I think (for now, at least) it's only
//necessary to register Note Off and On events

#include <string_view>
#include <vector>
#include <fstream>
#include <iostream>
#include <bitset>
#include <cstdint>

#include "MIDInotes.h"

enum Meta
{
    seq_num,
    text,
    copyright,
    name,
    instrument,
    lyrics,
    marker,
    cue,
    channel,
    end,
    set_tempo,
    smpte_offset,
    time_sig,
    key_sig,
    sequencer,
    max_meta
};

bool operator== (const int& x, const Meta& m)
{
    return ( x == static_cast<int>( m ) );
}

bool operator> (const Meta& m, const int& x)
{
    return ( static_cast<int>(m) > x );
}

bool operator< (const Meta& m, const int& x)
{
    return ( x > m );
}

std::ostream& operator<< (std::ostream& out, Meta& m)
{
    switch(m)
    {
    case seq_num:
        out << "Sequence Number";
        break;
    case text:
        out << "Text";
        break;
    case copyright:
        out << "Copyright Notice";
        break;
    case name:
        out << "Sequence/Track Name";
        break;
    case instrument:
        out << "Instrument";
        break;
    case lyrics:
        out << "Lyrics";
        break;    
    case marker:
        out << "Marker";
        break;    
    case cue:
        out << "Cue Point";
        break;    
    case channel:
        out << "MIDI Channel";
        break;    
    case end:
        out << "End of Track";
        break;    
    case set_tempo:
        out << "Set Tempo";
        break;    
    case smpte_offset:
        out << "SMPTE Offset";
        break;
    case time_sig:
        out << "Time Signature";
        break;    
    case key_sig:
        out << "Key Signature";
        break;
    case sequencer:
        out << "Sequencer Specific Event";
        break;
    case max_meta:
        out << "Unidentified MIDI Event";
        break;
    }

    return out;
}

Meta metaEvent(int ff)
{
    switch(ff)
    {
    case 0: return seq_num;
    case 1: return text;
    case 2: return copyright;
    case 3: return name;
    case 4: return instrument;
    case 5: return lyrics;
    case 6: return marker;
    case 7: return cue;
    case 32: return channel;
    case 47: return end;
    case 81: return set_tempo;
    case 84: return smpte_offset;
    case 88: return time_sig;
    case 89: return key_sig;
    case 127: return sequencer;
    default: return max_meta;
    }
}

int power(int base, int exp)
{
    if ( base == 0 )
        return 0;

    if ( exp == 0 )
        return 1;

    return base * power(base, exp - 1);
}

void sequenceNumber(std::vector<int>& bytes, std::size_t& index)
{
    //instantiate short to store sequence number to print
    short number{};

    number = bytes[index] << 8;

    number |= bytes[++index];

    std::cout << number;
}

void tempoChange(std::vector<int>& bytes, std::size_t& index)
{
    long long microseconds{};

    microseconds = (static_cast<std::uint8_t>(bytes[index]) << 16);

    microseconds |= (static_cast<std::uint8_t>(bytes[++index]) << 8);

    microseconds |= static_cast<std::uint8_t>(bytes[++index]);

    std::cout << 60 / (double(microseconds) / 1000000) << " BPM";
}

void smpteOffset(std::vector<int>& bytes, std::size_t& index)
{
    std::size_t end { index + 4 };

    for ( index; index < end; ++index )
    {
        std::cout << bytes[index] << ':';
    }

    std::cout << bytes[index];
}

void timeSignature(std::vector<int>& bytes, std::size_t& index)
{
    //increment index again because we only incremented to the length byte before
    std::cout << bytes[index] << '/' << power(2, bytes[++index]);
    std::cout << "\n\t" << "MIDI clocks per quarter note: " << bytes[++index];
    std::cout << "\n\t" << "Number of 32nd notes per 24 MIDI clocks: " << bytes[++index];
}

void keySignature(std::vector<int>& bytes, std::size_t& index)
{
    switch(bytes[index])
    {
    case -7:
        std::cout << "Cb ";
        break;      
    case -6:
        std::cout << "Gb ";
        break;        
    case -5:
        std::cout << "Db ";
        break;    
    case -4:
        std::cout << "Ab ";
        break;
    case -3:
        std::cout << "Eb ";
        break;
    case -2:
        std::cout << "Bb ";
        break;
    case -1:
        std::cout << "F ";
        break;
    case 0:
        std::cout << "C ";
        break;
    case 1:
        std::cout << "G ";
        break;
    case 2:
        std::cout << "D ";
        break;
    case 3:
        std::cout << "A ";
        break;
    case 4:
        std::cout << "E ";
        break;
    case 5:
        std::cout << "B ";
        break;
    case 6:
        std::cout << "F# ";
        break;
    case 7:
        std::cout << "C# ";
        break;
    default:
        std::cout << "??? ";
        break;
    }

    if(bytes[++index])
    {
        std::cout << "minor";
    }
    else
    {
        std::cout << "Major";
    }
}

long calculateVariableLength(std::vector<int>& bytes, std::size_t& index)
{
    long vL{};
    int temp{};

    while(true)
    {
        temp = (bytes[index] & 0x7F);
        vL <<= 7;
        vL |= temp;

        if (bytes[index] & 0x80)
            ++index;
        else
            break;
    }

    return vL;
}

void printVLEvent(std::vector<int>& bytes, std::size_t& index, long& vL)
{
    std::size_t end { index + vL };

    while ( index < end )
    {
        std::cout << char(bytes[index]);
        ++index;

        if ( bytes[index] == 0 )
            break;
    }

    std::cout << '\n';
}

void lookahead(std::vector<int>& bytes, std::size_t& index)
{
    if ( bytes[index] == 0 )
    {
        if ( bytes[index + 1] < 0 )
        {
            return;
        }
        else
        {
            ++index;
        }
    }
    else
    {
        ++index;
    }
}

bool parseMetaEvent(std::vector<int>& bytes, std::size_t& index)
{
    //event is a meta; metaEvent returns the type of meta event
    //that corresponds with the byte immediately following FF
    Meta m_event{ metaEvent( bytes[index] ) };

    std::cout << m_event << ": ";

    //if the meta event is variable length, calculate variable length and print text
    if( (m_event > 0 && m_event < 8) || m_event == ( max_meta - 1 ) )
    {
        //increment index to start calculation with the next byte
        long variable_length{ calculateVariableLength(bytes, ++index) };

        if ( variable_length > 0 )
        {
            printVLEvent(bytes, ++index, variable_length);
        }
        else
        {
            std::cout << '\n';
            ++index;
        }

    }
    //if the meta event is not variable length, implement the correct fixed-length procedure
    else
    {
        //increment index to arrive at (fixed) length byte
        ++index;

        //increment index while entering function to start processing after length byte
        switch( m_event )
        {
        case seq_num:
            sequenceNumber(bytes, ++index);
            break;
        case channel:
            //trivial: print channel byte
            std::cout << bytes[++index];
            break;
        case end:
            //trivial: track is over, we are done
            std::cout << "---\n\n";
            return false;
        case set_tempo:
            tempoChange(bytes, ++index);
            break;
        case smpte_offset:
            smpteOffset(bytes, ++index);
            break;
        case time_sig:
            timeSignature(bytes, ++index);
            break;
        case key_sig:
            keySignature(bytes, ++index);
            break;
        default:
            std::cerr << "Cannot recognize meta event.";
            break;
        }

        std::cout << '\n';

        ++index;
    }

    return true;
}

void parseMIDIEvent(std::vector<int>& bytes, std::size_t& index, NoteVector& noteVector, int& status)
{
    char event { status & 0xF0 };
    //if Note On...
    if ( event == char(0x90) )
    {
        //current index is note number
        //next index is velocity

        if (bytes[index + 1] > 0)
        {
            noteVector.addNote( MIDInote{ status, bytes[index] } );
        }
        //else implicit Note Off
        else
        {
            noteVector.noteOff( status, bytes[index] );
        }

        //increment index to arrive at velocity byte
        ++index;
        //increment again to arrive at next delta byte
        ++index;
    }
    //if explicit Note Off...
    else if ( event == char(0x80) )
    {
        noteVector.noteOff( status, bytes[index] );

        //increment index to arrive at velocity byte
        ++index;
        //increment again to arrive at next delta byte
        ++index;
    }
    //if some other MIDI event...
    else
    {
        //...skip over this data

        //handle voice and mode messages
        //in higher level switch

        switch(event)
        {
        case char(0xA0): [[fallthrough]];
        case char(0xB0): [[fallthrough]];
        case char(0xE0):
            //increment index to arrive at
            //second data byte
            ++index;
            [[fallthrough]];
        case char(0xC0): [[fallthrough]];
        case char(0xD0): [[fallthrough]];
        case char(0xF0):
            //increment delta to arrive at
            //next delta time byte
            lookahead(bytes, index);
            break;
       //handle system messages in default
        default:
            char def { status & 0x0F };

            switch(def)
            {
            case (0x02):
                //increment index to arrive at
                //second data byte
                ++index;
                [[fallthrough]];
            case (0x03):
                //increment to arrive at
                //next delta time byte
                lookahead(bytes, index);
                [[fallthrough]];
            default:
                //there are no data bytes,
                //so index is already at
                //next delta time byte
                break;
            }
        }
    }
}

void parseSingleTrack(std::vector<int>& bytes, short quarter_note)
{
    //store MIDI notes in NoteVector class
    NoteVector noteVector{};

    //store delta time in an int
    int delta{ 0 };

    //store status byte in an int 
    int status{};

    std::size_t index{};

    for ( std::size_t begindex{}; begindex < std::size(bytes); ++begindex )
    {
        if (bytes[begindex] == char(0xFF) )
        {
            index = begindex;
            break;
        }
    }

    for ( index; index < std::size(bytes); ++index )
    {
        //interpret status byte
        //
        //if this is a system exclusive event...
        if ( bytes[index] == char(0xF0) || bytes[index] == char(0xF7) )
        {
            long variable_length{ calculateVariableLength(bytes, ++index) };
            std::size_t end { index + variable_length };

            //... just skip through all of this data
            while ( index < end )
                ++index;
            
            delta = calculateVariableLength(bytes, index);
            
            if (delta > 0)
            {
                noteVector.addDelta( delta );
            }

            continue;
        }
        //if this is a meta event...
        else if( bytes[index] == char(0xFF) )
        {
            status = bytes[index];

            continue;
        }
        //if this is a MIDI event...
        else if ( bytes[index] >= char(0x80) && bytes[index] < char(0xFF) )
        {
            status = bytes[index];

            continue;
        }
        //if it's not any of these events, it must be running status,
        //so go directly to following else statement

        status = bytes[index - 1];

        //parse meta events
        if ( status == char(0xFF) )
        {
            noteVector.printNotes( quarter_note );

            if( parseMetaEvent(bytes, index) )
            {
                //index is at the next delta time byte
                delta = calculateVariableLength(bytes, index);

                if (delta > 0)
                {
                    noteVector.addDelta( delta );
                }

                continue;
            }
            else
            {
                break;
            }
        }
        //parse MIDI events
        else
        {
            parseMIDIEvent( bytes, index, noteVector, status );

            //index is at the next delta time byte
            delta = calculateVariableLength(bytes, index);

            if (delta > 0)
            {
                noteVector.addDelta( delta );
            }
        }
    }

    noteVector.printNotes( quarter_note );
}

void parseTracks(std::ifstream& inf, short quarter_note)
{
    //store track bytes in vector
    std::vector<int> bytes{};

    //iterate through bytes with char
    char c;

    bool e_gate { false };
    bool p_gate { false };

    while (inf >> c)
    {
        if( p_gate )
        {
            bytes.push_back( int(c) );

            parseSingleTrack(bytes, quarter_note);
            
            //erase everything from the bytes vector
            bytes.erase( bytes.begin(), bytes.begin() + std::size(bytes) );

            p_gate = false;

            continue;
        }

        if ( e_gate )
        {
            if ( c == char(0x2F) )
            {
                p_gate = true;
            }

            e_gate = false;
        }

        if( c == char(0xFF) )
        {
            e_gate = true;
        }

        //until we're at a new track, keep adding bytes to the vector
        bytes.push_back( int(c) );
    }

    std::cout << '\n';

    //if we're at the end of the MIDI file, parse the most-recently stored track
    parseSingleTrack(bytes, quarter_note);
}