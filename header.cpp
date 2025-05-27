#include <iostream>
#include <bitset>
#include <fstream>

void printFormat(int c)
{
    if( c == 0 )
    {
        std::cout << "Format: single multi-channel track\n";
    }
    else if ( c == 1 )
    {
        std::cout << "Format: one or more simultaneous tracks of a sequence\n";
    }
    else
    {
        std::cout << "Format: one or more sequentially independent single-track patterns\n";
    }
}

void printNumTracks(int c)
{
    std::cout << "Number of tracks: " << c << '\n';
}

void printDivision(short& division)
{
    //test if MSB is 0 (metrical time) or 1 (time-code-based time); will currently not convert from time-code-based time
    if( division & 0x80 )
    {
        std::cerr << "Error: cannot convert from time-code-based time\n";
    }
    else
    {
        //bits 14-0 represent number of delta time ticks that make up a quarter note
        std::cout << "Ticks per quarter note: " << division << '\n';
    }
}

short parseMIDIHeader(std::ifstream& inf)
{
    char c{};

    //traverse lets us skip over bytes, since the first four bytes are ASCII (MThd) followed by the 32-bit <length> (which will always be six)
    int traverse{ 0 };

    //need the last two bytes of the header as one 16-bit representation;
    //return for use with MIDInotes.h functions
    short division{};

    while(inf >> c)
    {
        if (traverse == 9)
        {
            //skip first byte of <format> since the latter will always be 0, 1, or 2
            printFormat(c);
        }
        else if ( traverse == 11 )
        {
            //skip first byte since highly doubtful that there will be more than 127 tracks (a <format> of 0 will always be 1)
            printNumTracks(c);
        }
        else if ( traverse == 12 )
        {
            //assign byte to the most significant half of the 16-bit division variable (MSB determines if MIDI file is valid for conversion)
            division = c << 8;
        }
        else if ( traverse > 12 )
        {
            //assign to least significant half of the 16-bit division variable
            division |= c;
            printDivision(division);
            break;
        }
        ++traverse;
    }

    return division;
}