#define __CL_ENABLE_EXCEPTIONS

#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <CL/cl.hpp>
#include <boost/program_options.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/timer/timer.hpp>

#include "convolution.hpp"
#include "filters.hpp"
#include "filter_factory.hpp"
#include "bmp.hpp"

using std::string;
using std::ifstream;
using std::endl;
using std::cout;
using std::vector;
using std::ostringstream;
using cl::Program;
using cl::Platform;
using cl::Event;
using cl::Context;
using cl::Kernel;
using cl::Error;
using cl::CommandQueue;
using cl::NDRange;
using cl::NullRange;
using cl::Buffer;
using cl::Device;
using boost::lexical_cast;

namespace po = boost::program_options;

struct Args
{
    string inputFile, outputFile;
    vector<string> filters;
};

struct Environment
{
    Context             context;
    vector<Device> devices;
    Device            device;
    CommandQueue        queue;
    Program             program;
    Kernel kernel;
};

struct Buffers
{
    Buffer inputImage, outputImage, filter;
};

struct Images
{
    int imageWidth, imageHeight, imageSize;
    int bufferedWidth, bufferedHeight, bufferedSize;
    Bitmap inputImage, outputImage, bufferedImage;
    size_t dataSize, bufferedDataSize;
};

static const int LOCAL_WORK_GROUP_SIZE = 16;

size_t readSource(string filename, string &source)
{
    ifstream file (filename.c_str(), std::ios::binary);

    file.seekg (0, std::ios::end);
    std::streampos size = file.tellg();
    if (size == -1)
    {
        cout << "File " << filename << " could not be read";
        exit(-1);
    }
    file.seekg (0, std::ios::beg);

    source.resize(size);
    file.read (&source[0],size);
    file.close();

    return size;
}


void copyOutputToInput(Images &imgs, const Buffers &buffs, Environment &env)
{
    // Copy back to input image for the next filter, if any
    if(imgs.inputImage.grey)
    {
    // Read the image back to the host
        env.queue.enqueueReadBuffer(buffs.outputImage, CL_TRUE, 0,
                                    imgs.dataSize,
                                    imgs.outputImage.greyData, 0);


        std::copy(imgs.outputImage.greyData,
                  imgs.outputImage.greyData + imgs.imageSize,
                  imgs.inputImage.greyData);
    }
    else
    {
        env.queue.enqueueReadBuffer(buffs.outputImage, CL_TRUE, 0,
                                    imgs.dataSize,
                                    imgs.outputImage.colourData, 0);


        std::copy(imgs.outputImage.colourData,
                  imgs.outputImage.colourData + imgs.imageSize,
                  imgs.inputImage.colourData);
    }
}

void parseArgs (const int argc, const char * const * argv, Args &args)
{
    string usage = "convolution [-fhio] [<input file>] [-fhio]";

    ostringstream filterHelp;
    filterHelp
        << "filter to run on the image\n"
        << "Possible filters are:\n\n"
        << "blur:a,b\n"
        << "  a = filter size\n"
        << "  b = direction\n\n"
        << "sharpen:a\n"
        << "  a = filter size\n\n"
        << "brighten:a\n"
        << "  a = brighten amount\n\n"
        << "darken:a\n"
        << "  a = darken amount\n\n"
        << "edgedetect:a,b\n"
        << "  a = filter size\n"
        << "  b = direction\n\n"
        << "emboss:a\n"
        << "  a = filter size\n\n\n"
        << "directions are:\n"
        << "  0 = full\n"
        << "  1 = horizontal\n"
        << "  2 = vertical\n"
        << "  3 = top right -> bottom left\n"
        << "  4 = top left -> bottom right\n\n"
        << "filter sizes must be odd and in the interval [1,15]";
    po::options_description desc("Allowed options");
    desc.add_options()
        ("help,h", "produce help message")
        ("input-file,i",
         po::value<string>(&args.inputFile)->default_value("input.bmp"),
         "input file\nany option without a flag is also parsed as an input file")
        ("output-file,o",
         po::value<string>(&args.outputFile)->default_value("output.bmp"),
         "output file")
        ("filter,f",
         po::value< vector<string> >(&args.filters),
         filterHelp.str().c_str())
        ;

    po::positional_options_description p;
    p.add("input-file", -1);

    po::variables_map vm;
    po::store(po::command_line_parser(argc, argv)
              .options(desc)
              .positional(p)
              .run(), vm);
    po::notify(vm);

    if (vm.count("help"))
    {
        cout << "Usage: " << usage << endl;
        cout << desc << endl;
        exit(0);
    }

    if (!vm.count("filter"))
    {
        cout << "No filters specified. Pass \"-h\" for help" << endl;
        exit(-1);
    }
}

float strToFloat (string s)
{
    return lexical_cast<float>(s);
}

Filter *createFilter(string filterDesc)
{
    static FilterFactory ff;

    vector<string> strs;
    boost::split(strs, filterDesc, boost::is_any_of(":"));

    vector<string> args;
    boost::split(args, strs[1], boost::is_any_of(","));

    vector<float> floatArgs;
    std::transform(args.begin(), args.end(),
                   back_inserter(floatArgs),
                   &strToFloat);

    return ff.createFilter(strs[0], floatArgs);
}

template <class T>
T *bufferInputImage (Images &imgs, const Filter *filter, T *toBuffer)
{
    int bufferWidth = filter->size()/2;

    imgs.bufferedWidth = bufferWidth * 2 + imgs.imageWidth;
    imgs.bufferedHeight = bufferWidth * 2 + imgs.imageHeight;
    imgs.bufferedSize = imgs.bufferedWidth * imgs.bufferedHeight;

    T *bufferedImage = new T[imgs.bufferedSize]();

    int inputIndex, bufferedIndex;
    for (int i = 0; i < imgs.imageHeight; i++)
    {
        for (int j = 0; j < imgs.imageWidth; j++)
        {
            inputIndex = i * imgs.imageWidth + j;
            bufferedIndex = (i + bufferWidth)
                * (imgs.imageWidth+bufferWidth*2)
                + j + bufferWidth;
            bufferedImage[bufferedIndex] = toBuffer[inputIndex];
        }
    }

    imgs.bufferedDataSize = imgs.bufferedSize * sizeof(T);

    return bufferedImage;
}

void initImages (Images &imgs, const string &inputFile)
{
    imgs.inputImage.read(inputFile);
    imgs.imageHeight = imgs.inputImage.infoHeader->biHeight;
    imgs.imageWidth = imgs.inputImage.infoHeader->biWidth;

    imgs.bufferedImage.grey = imgs.inputImage.grey;
    //not going to change any info, so can just copy pointers
    imgs.outputImage.infoHeader = imgs.inputImage.infoHeader;
    imgs.outputImage.fileHeader = imgs.inputImage.fileHeader;
    imgs.outputImage.extraHeader = imgs.inputImage.extraHeader;
    imgs.outputImage.grey = imgs.inputImage.grey;

    // Size of the input and output images on the host
    imgs.imageSize = imgs.imageHeight * imgs.imageWidth;
    imgs.dataSize = imgs.imageSize *
        (imgs.inputImage.grey?sizeof(float):sizeof(cl_float4));

    if (imgs.inputImage.grey)
    {
        imgs.outputImage.greyData = new float[imgs.imageSize];
    }
    else
    {
        imgs.outputImage.colourData = new cl_float4[imgs.imageSize];
    }
}

void bufferCorrectInputImage(Images &imgs, const Filter *filter)
{
    if (imgs.inputImage.grey)
    {
        imgs.bufferedImage.greyData =
            bufferInputImage(imgs, filter, imgs.inputImage.greyData);
    }
    else
    {
        imgs.bufferedImage.colourData =
            bufferInputImage(imgs, filter,
                             imgs.inputImage.colourData);
    }
}

void setKernelArgs(Environment &env, const Buffers &buffs,
                   size_t bufferSize, size_t pixelSize)
{
    env.kernel.setArg(0, buffs.inputImage);
    env.kernel.setArg(1, buffs.outputImage);
    env.kernel.setArg(2, buffs.filter);

    size_t localDimension = (LOCAL_WORK_GROUP_SIZE + bufferSize*2);
    size_t localSize = localDimension * localDimension;

    clSetKernelArg(env.kernel(), 3, localSize*pixelSize, NULL);
}

void runKernel(const CommandQueue &queue, const Kernel &kernel,
               int workSizeX, int workSizeY)
{
    const NDRange global_work_size (workSizeX,
                                    workSizeY);
    const NDRange local_work_size (LOCAL_WORK_GROUP_SIZE,
                                   LOCAL_WORK_GROUP_SIZE);

    Event event;

    queue.enqueueNDRangeKernel(kernel, NullRange,
                                   global_work_size, local_work_size,
                                   NULL, &event);
    event.wait();

    cl_ulong time_start, time_end;
    double total_time;

    event.getProfilingInfo(CL_PROFILING_COMMAND_START, &time_start);
    event.getProfilingInfo(CL_PROFILING_COMMAND_END, &time_end);
    total_time = time_end - time_start;
    printf("Filter took %0.3f ms to apply\n", (total_time / 1000000.0) );
}

void createBuffers(const Images &imgs, Filter *filter,
                   const Context &context, Buffers &buffs)
{
    void *buffMem;
    void *outMem;
    if (imgs.inputImage.grey)
    {
        buffMem = imgs.bufferedImage.greyData;
        outMem = imgs.outputImage.greyData;
    }
    else
    {
        buffMem = imgs.bufferedImage.colourData;
        outMem = imgs.outputImage.colourData;
    }

    buffs.inputImage = Buffer (context,
                               CL_MEM_READ_ONLY|CL_MEM_COPY_HOST_PTR,
                               imgs.bufferedDataSize, buffMem);

    buffs.outputImage = Buffer (context,
                                CL_MEM_WRITE_ONLY|CL_MEM_COPY_HOST_PTR,
                                imgs.dataSize, outMem);

    buffs.filter = Buffer (context, CL_MEM_READ_ONLY|CL_MEM_COPY_HOST_PTR,
                           sizeof(float)*filter->size()*filter->size(),
                           filter->filter());
}

void initEnvironment(Environment &env, const string &sourceFile)
{
    vector <Platform> platforms;
    Platform::get(&platforms);

    cl_context_properties cprops[3] =
        { CL_CONTEXT_PLATFORM, (cl_context_properties) platforms[0](), 0 };
    env.context = Context (CL_DEVICE_TYPE_GPU, cprops);

    env.devices = env.context.getInfo<CL_CONTEXT_DEVICES>();
    env.device = env.devices[0];
    env.queue = CommandQueue (env.context, env.device,
                              CL_QUEUE_PROFILING_ENABLE);

    string source;
    size_t sourceLength = readSource(
        sourceFile,
        source);

    Program::Sources sources(1, std::make_pair(source.c_str(), sourceLength));
    env.program = Program (env.context, sources);
}

void buildProgram(const Images &imgs, const Filter *filter, Environment &env)
{
    int bufferSize = filter->size()/2;
    ostringstream options;
    options << "-D BUFFER_SIZE=" << bufferSize << " "
            << "-D DOUBLE_BUFFER_SIZE=" << bufferSize*2 << " "
            << "-D HEIGHT=" << imgs.bufferedHeight << " "
            << "-D WIDTH=" << imgs.bufferedWidth << " "
            << "-D FACTOR=" << filter->factor() << " "
            << "-D BIAS=" << filter->bias();

    try
    {
        env.program.build(env.devices,options.str().c_str());
    }
    catch (Error e)
    {
        string info;
        env.program.getBuildInfo(env.device, CL_PROGRAM_BUILD_LOG, &info);
        cout << info;
        exit(-1);
    }
}

int main(int argc, char** argv) {
    try
    {
        Args args;
        parseArgs(argc, argv, args);

        Images imgs;
        initImages(imgs, args.inputFile);

        vector<string>::iterator filterIt;
        for (filterIt = args.filters.begin();
             filterIt != args.filters.end(); filterIt++)
        {
            Filter *filter = createFilter(*filterIt);
            cout << "Applying " << filter->filterName() << endl;

            bufferCorrectInputImage(imgs, filter);

            Environment env;
            string sourceFile = imgs.inputImage.grey?
                "convolutiongrey.cl":"convolutioncolour.cl";
            initEnvironment(env, sourceFile);


            Buffers buffs;
            createBuffers(imgs, filter, env.context, buffs);

            buildProgram(imgs, filter, env);
            env.kernel = Kernel (env.program, "convolution");

            size_t pixelSize = imgs.inputImage.grey?
                sizeof(float):sizeof(cl_float4);
            setKernelArgs(env, buffs, filter->size()/2, pixelSize);

            runKernel(env.queue, env.kernel,
                      imgs.bufferedHeight, imgs.bufferedWidth);

            copyOutputToInput(imgs, buffs, env);

            imgs.outputImage.write(args.outputFile);
        }

        if (imgs.inputImage.grey)
        {
            delete[] imgs.inputImage.greyData;
            delete[] imgs.bufferedImage.greyData;
        }
        else
        {
            delete[] imgs.inputImage.colourData;
            delete[] imgs.bufferedImage.colourData;
        }
    }
    catch(Error error)
    {
        std::cout << error.what() << "(" << error.err() << ")" << std::endl;
        exit(-1);
    }
    return 0;
}
