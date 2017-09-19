#include <vector>
#include <string>
#include <random>
#include <sstream>
#include <fstream>
#include <iostream>
#include <algorithm>

#include <mpi.h>
#include <omp.h>

//  Types

using grid_t = std::vector<std::vector<bool>>;

//  Constants

static constexpr std::size_t ARG_COUNT = 5;
static constexpr std::size_t ARG_ITERATIONS = 1;
static constexpr std::size_t ARG_GRID_SIZE = 2;
static constexpr std::size_t ARG_HOOD_TYPE = 3;
static constexpr std::size_t ARG_MAKE_IMAGES = 4;

//  RULESET
static constexpr std::size_t LONELY = 2;    // 2 or less dies
static constexpr std::size_t REBIRTH = 3;   // exactly 3 creates
static constexpr std::size_t OVERPOP = 4;   // 4 or more die

//  FILENAMES
static constexpr const char FILES_FOLDER_NAME[] = "files/";
static constexpr const char FILES_NAME[] = "img.save";

//  Prototypes

void usage(const int, const char **);
std::size_t get_iterations(const char **);
grid_t init_new_grid(const char **);
void init_grid_state(grid_t &);
bool is_moore(const char **);
void generate_moore(const grid_t &, grid_t &);
void generate_von(const grid_t &, grid_t &);
void file_out(const grid_t &, const std::size_t, const std::size_t);
void progress_bar(const std::size_t, const std::size_t);
std::size_t file_number_digits(const std::size_t);

bool user_wants_images(const char **);
void create_images_from_files(const std::size_t, const std::size_t);

int main(const int argc, const char ** argv)
{
    usage(argc, argv);
    MPI_Init(NULL, NULL);

    //  Setup requied variables
    //

    std::size_t iterations = get_iterations(argv);
    std::size_t digits = file_number_digits(iterations);
    grid_t current = init_new_grid(argv);
    grid_t next = init_new_grid(argv);

    // perform check only once
    auto generator = (is_moore(argv) ? generate_moore : generate_von);

    //  Setup the first generation

    init_grid_state(current);

    //  Main generation loop

// TODO: openMP this loop .. somehow
    std::size_t generation; 
    for (generation = 0; generation < iterations; ++generation)
    {
        progress_bar(generation, iterations);

        // generate next automaton
        generator(current, next);

        // transfer next generation to current
        current = next;

        // output to file     
        file_out(current, generation, digits);
    }

    if (user_wants_images(argv))
        create_images_from_files(iterations, digits);

    MPI_Finalize();
}

//  Implementations

void usage(const int argc, const char ** argv) 
{
    if (argc != ARG_COUNT)
    {
        std::cout << "Usage: "
            << argv[0]
            << " <iterations> <grid_size> <neighbourhood type>\n";
        exit(-1);
    }
}

std::size_t get_iterations(const char ** argv)
{
    std::stringstream stream{ argv[ARG_ITERATIONS] };
    std::size_t iterations;
    
    if (!(stream >> iterations))
    {
        std::cout << "Error: " << argv[ARG_ITERATIONS]
            << ", cannot be parsed as an integer\n";
            exit(-1);
    }

    return iterations;
}

grid_t init_new_grid(const char ** argv)
{
    std::stringstream stream{ argv[ARG_GRID_SIZE] };
    std::size_t grid_size;

    if (!(stream >> grid_size))
    {
        std::cout << "Error: " << argv[ARG_GRID_SIZE]
            << ", cannot be parsed as an integer\n";
        exit(-1);
    } else if (grid_size < 5)
    {
        std::cout << "Grid Size is required to be at least 5\n";
        exit(-1);
    }

    return std::vector<std::vector<bool>>(grid_size, 
                std::vector<bool>(grid_size, false));
}

void init_grid_state(grid_t & grid)
{
    std::random_device rd;
    std::mt19937 gen(rd()); 
    std::uniform_int_distribution<> dis(0, 1);

    for (std::size_t x = 2; x < grid.size() - 2; ++x)
        for (std::size_t y = 2; y < grid.size() - 2; ++y)
            grid[x][y] = dis(gen);
}

bool is_moore(const char ** argv)
{
    std::string hood{ argv[ARG_HOOD_TYPE] };
    if (hood == "m") return true;
    if (hood == "v") return false;
    
    std::cout << "Invalid hood type: " << hood << "\n";
    exit(-1);
}

void progress_bar(const std::size_t iteration, const std::size_t total)
{
    static constexpr std::size_t size = 70;
    double percentage = static_cast<double>(iteration) / 
                                static_cast<double>(total);
    std::size_t draw = percentage * size;

    std::cout << "\r " << iteration << " / " << total << " [";
    for (std::size_t i = 0; i < size; ++i)
    {
        if (i < draw) std::cout << "=";
        else std::cout << " ";
    }

    std::cout << "] " << percentage * 100.0 << "%       ";
    std::cout.flush();
}

template <typename Function_>
void generate(const grid_t & current, grid_t & next, Function_ hood)
{    
    int mpi_size, mpi_rank;
    MPI_Comm_size(MPI_COMM_WORLD, &mpi_size);
    MPI_Comm_rank(MPI_COMM_WORLD, &mpi_rank);

    std::size_t part_size = (current.size() - 4) / mpi_size;

    for (int part = 0; part < mpi_size; ++part)
    {
        std::size_t x_init = 2 + (part_size * mpi_rank);
        std::size_t x, y;
        if (mpi_rank == part)
#pragma omp parallel for private(x, y)
            for (x = x_init; x < (x_init + part_size); ++x)
                for (y = 2; y < current.size() - 2; ++y)
                {
                    // get hood
                    std::size_t alive = hood(x, y);
            
                    // rebirth
                    if (current[x][y] == false && alive == REBIRTH)
                        next[x][y] = true;

                    // dies
                    else if (current[x][y] && (alive < LONELY || alive > OVERPOP))
                        next[x][y] = false;
            
                    // stasis
                    else
                        next[x][y] = current[x][y];
                }
    }
}

void generate_moore(const grid_t & current, grid_t & next)
{
    generate(current, next, 
        [&current](const std::size_t x, const std::size_t y)
    {
        return 
            current[x - 1][y - 1] + current[  x  ][y - 1] + current[x + 1][y - 1] +
            current[x - 1][  y  ] + /*  not center     */ + current[x + 1][  y  ] +
            current[x - 1][y + 1] + current[  x  ][y + 1] + current[x + 1][y + 1];
    });
}

void generate_von(const grid_t & current, grid_t & next)
{
    generate(current, next, 
        [&current](const std::size_t x, const std::size_t y)
    {
        return 
                                    current[  x  ][y - 1] +
            current[x - 1][  y  ] + /*  not center     */ + current[x + 1][  y  ]
                                  + current[  x  ][y + 1];
    });
}

std::string file_name(const std::size_t generation, const std::size_t digits)
{
    // filename in the directory `FILES_FOLDER_NAME`
    //      called `ITERATION_NUMBER` + `FILES_NAME`
    //      where `ITERATION_NUMBER` is the right amount of digits with preseeding zeros

    return std::string{ FILES_FOLDER_NAME }.append(
                std::string(digits - std::to_string(generation).size(), '0').append(
                    std::to_string(generation).append(
                        FILES_NAME
    )));
}

void file_out(const grid_t & grid, const std::size_t generation, const std::size_t digits)
{
    std::ofstream file{ file_name(generation, digits), std::ios::trunc };

    for (const auto & vec : grid)
    {
        for (const std::size_t & x : vec)
            file << x << " ";
        file << "\n";
    }

    file.close();
}

std::size_t file_number_digits(const std::size_t iterations)
{
    return std::to_string(iterations).size();
}

bool user_wants_images(const char ** argv)
{
    std::cout << "\n\n"; // for readability
    return argv[ARG_MAKE_IMAGES] == std::string{ "y" };
}

void create_images_from_files(const std::size_t generation, const std::size_t digits)
{
    // TODO
    // remember to use progress bar
}
