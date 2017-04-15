#include "filter_factory.hpp"

Filter *FilterFactory::createFilter(std::string name, std::vector<float> args)
{
    to_lower(name);
    if (name == "sharpen")
        return new Sharpen(args);
    if (name == "emboss")
        return new Emboss(args);
    if (name == "blur")
        return new Blur(args);
    if (name == "darken")
        return new Darken(args);
    if (name == "brighten")
        return new Brighten(args);
    if (name == "edgedetect")
        return new EdgeDetect(args);
    else
        throw "Invalid filter " + name;

    return NULL;
}
