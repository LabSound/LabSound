#include "ExampleBaseApp.h"
#include <string>
#include <vector>

using std::string;
using std::vector;

// takes a string, and separates out according to embedded quoted strings
// examples
// abc > abc
// abc "def" > abc, "def"
// a "def" ghi > a, "def", ghi
// a\"bc > a\"bc
// as you can see, the quotes are preserved.
// and quotes are escaped

vector<string> Separate(const std::string& input)
{
    vector<string>output;

    size_t curr = 0;
    size_t start = 0;
    size_t end = input.length();
    bool inQuotes = false;

    while (curr < end)
    {
        if (input[curr] == '\\')
        {
            ++curr;
            if (curr != end && input[curr] == '\"')
                ++curr;
        }
        else
        {
            if (input[curr] == '\"')
            {
                // no empty string if not in quotes, otherwise preserve it
                if (inQuotes || (start != curr))
                {
                    output.push_back(input.substr(start - (inQuotes?1:0), curr - start + (inQuotes?2:0)));
                }
                inQuotes = !inQuotes;
                start = curr+1;
            }
            ++curr;
        }
    }

    // catch the case of a trailing substring that was not quoted, or a completely unquoted string
    if (curr - start > 0)
        output.push_back(input.substr(start, curr-start));

    return output;
}

vector<string> LabSoundExampleApp::SplitCommandLine(int argc, char** argv)
{
    // join the command line together so quoted strings can be found
    string cmd;
    for (int i = 1; i < argc; ++i)
    {
        if (i > 1)
            cmd += " ";
        cmd += string(argv[i]);
    }

    // separate the command line, respecting quoted strings
    vector<string> result = Separate(cmd);
    result.insert(result.begin(), string { argv[0] });
    return result;
}

std::shared_ptr<AudioBus> LabSoundExampleApp::MakeBusFromSampleFile(char const*const name, int argc, char** argv)
{
    std::string path_prefix;
    auto cmds = SplitCommandLine(argc, argv);

    if (cmds.size() > 1)
        path_prefix = cmds[1] + "/";    // cmds[0] is the path to the exe

    std::string path = path_prefix + name;
    std::cout << "Loading: " << path << std::endl;
    std::shared_ptr<AudioBus> musicClip = MakeBusFromFile(path, false);
    if (!musicClip) throw std::runtime_error("couldn't open " + path);

    return musicClip;
}
