#include "filters.hpp"

#include <iostream>
#include <algorithm>

using std::string;

BadFilterArguments::BadFilterArguments (Filter *filter)
    : msg("Bad number of arguments to filter " + filter->filterName()) {}

void Filter::checkArgs (std::vector<float> args, size_t size)
{
    if (args.size() != size)
    {
        throw BadFilterArguments(this);
    }
}

void Filter::fillFilterForDirection (float toFill, int direction)
{
    if (direction == 0)
    {
        for (int i = 0; i < _size*_size; i++)
        {
            _filter[i] = toFill;
        }
        return;
    }

    int begin = 0;
    std::function<int(int)> indexer;
    switch (direction)
    {
    case 1:
        begin = _size*(_size/2);
        indexer = [=](int i){return begin+i;};
        break;
    case 2:
        begin = _size/2;
        indexer = [=](int i){return begin + i*_size;};
        break;
    case 3:
        indexer = [=](int i){return i + i*_size;};
        break;
    case 4:
        indexer = [=](int i){return _size-i-1 + i*_size;};
        break;
    }

    for (int i = 0; i < _size; i++)
    {
        _filter[indexer(i)] = toFill;
    }
}

void Filter::printFilter()
{
    for (int i = 0; i < _size; i++)
    {
        for (int j = 0; j < _size; j++)
        {
            std::cout << _filter[i*_size+j] << ' ';
        }
        std::cout << std::endl;
    }
}
