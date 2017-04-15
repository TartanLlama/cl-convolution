#ifndef FILTER_FACTORY_HPP_GUARD
#define FILTER_FACTORY_HPP_GUARD

#include <vector>
#include <string>
#include "filters.hpp"

class FilterFactory
{
public:
    Filter *createFilter(std::string name, std::vector<float> args);
};

#endif
