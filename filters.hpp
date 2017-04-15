#ifndef FILTERS_HPP_GUARD
#define FILTERS_HPP_GUARD
#include <vector>
#include <exception>
#include <boost/algorithm/string.hpp>

using boost::algorithm::to_lower;

class Filter
{
public:
    virtual const std::string filterName() {return "filter";}
    float *filter() {return _filter;}
    float factor() const {return _factor;}
    float bias() const {return _bias;}
    float size () const {return _size;}

    void checkArgs (std::vector<float> args, size_t size);

protected:
    float *_filter;
    float _factor;
    float _bias;
    int _size;

    void fillFilterForDirection(float toFill, int direction);
    void printFilter();
};

class Sharpen : public Filter
{
public:
    const std::string filterName() {return "sharpen";}

    Sharpen (std::vector<float> args)
    {
        checkArgs(args, 1);

        _size = args[0];
        _filter = new float[_size*_size]();

        fillFilterForDirection(-1, 0);
        _filter[_size*_size/2] = _size*_size;

        _factor = 1.0;
        _bias = 0.0;
    }
};

class Blur : public Filter
{
public:
    const std::string filterName() {return "blur";}

    Blur (std::vector<float> args)
    {
        checkArgs(args, 2);

        _size = args[0];
        int direction = args[1];

        _filter = new float[_size*_size]();

        fillFilterForDirection(1, direction);

        _factor = 1.0 / (direction == 0? (_size*_size) : _size);
        _bias = 0.0;
    }
};

class EdgeDetect : public Filter
{
public:
    const std::string filterName() {return "edgedetect";}
    EdgeDetect (std::vector<float> args)
    {
        checkArgs(args, 2);

        _size = args[0];
        _filter = new float[_size*_size]();

        int direction = args[1];
        fillFilterForDirection(-1, direction);

        _filter[_size*_size/2] = direction == 0? _size*_size - 1 : _size-1;

        _factor = 1.0;
        _bias = 0.0;
    }
};

class Darken : public Filter
{
public:
    const std::string filterName() {return "darken";}
    Darken (std::vector<float> args)
    {
        checkArgs(args, 1);

        _size = 1;
        _filter = new float[1];
        _filter[0] = 1;
        _factor = 1.0;
        _bias = -args[0];
    }
};

class Brighten : public Filter
{
public:
    const std::string filterName() {return "brighten";}

    Brighten (std::vector<float> args)
    {
        checkArgs(args, 1);

        _size = 1;
        _filter = new float[1];
        _filter[0] = 1;
        _factor = 1.0;
        _bias = args[0];
    }
};

class Emboss : public Filter
{
public:
    const std::string filterName() {return "emboss";}

    Emboss (std::vector<float> args)
    {
        checkArgs(args, 1);

        _size = args[0];
        _filter = new float[_size*_size];

        for (int i = 0; i < _size; i++)
        {
            for (int j = 0; j < _size; j++)
            {
                _filter[i*_size+j] = i+j>_size-1? 1 : i+j<_size-1? -1 : 0;
            }
        }

        _factor = 1.0;
        _bias = 128;
    }
};

class BadFilterArguments : public std::exception
{
public:
    BadFilterArguments(Filter *filter);
    ~BadFilterArguments() throw() {}
    const char* what() const throw() { return msg.c_str(); }

private:
    std::string msg;
};

#endif
