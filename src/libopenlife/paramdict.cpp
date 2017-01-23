#include "paramdict.hpp"

std::tuple<std::string, double> sepParamLine(std::string line)
{
    bool began_whitespace = false;
    auto endkey   = line.begin(); 
    auto beginval = line.begin();
    for (auto it = line.begin(); it != line.end(); it++)
    {
        if (*it == ' ')
        {
            if (not began_whitespace)
            {
                endkey = it;
            }
            began_whitespace = true;
        }
        else if (began_whitespace)
        {
            beginval = it;
            break;
        }
    }
    std::string key(line.begin(), endkey);
    std::string val(beginval, line.end());
    return std::make_tuple(key, std::stod(val));
}

ParamDict::ParamDict(std::string filename)
{
    print("Reading parameter dictionary from " + filename);
    for (auto line : tools::readFile(filename))
    {
        auto seperated = sepParamLine(line);
        auto key = std::get<0>(seperated);
        auto val = std::get<1>(seperated);
        params[key] = val;
        print("\"" + key + "\", \"" + std::to_string(val) + "\"");
    }
}

ParamDict::ParamDict()
{}

double ParamDict::get(std::string param)
{
    auto got = params.find(param);
    if ( got == params.end() )
    {
        print(param + " is not an element of the paramdict");
        throw std::exception();
    }
    return params[param];
}