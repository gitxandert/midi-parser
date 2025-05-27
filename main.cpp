#include <string>
#include <iostream>
#include <fstream>

short parseMIDIHeader(std::ifstream& inf);

void parseTracks(std::ifstream& inf, short quarter_note);

int main()
{
    std::string filename{ "midi/eyelash.mid" };

    std::ifstream inf{ filename };

    if(!inf)
    {
        std::cerr << filename << " could not be opened for reading\n";

        return -1;
    }

    std::cout << "\nReading MIDI file: " << filename << "\n\n";

    short quarter_note{ parseMIDIHeader(inf) };

    std::cout << '\n';

    parseTracks(inf, quarter_note);

	return 0;
}