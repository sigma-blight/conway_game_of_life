#include <cstdlib>
#include <vector>
#include <cstring>
#include <cstdio>
#include <cstdint>
#include <string>
#include <fstream>
#include <iostream>
#include <iomanip>
#include <sstream>

class CMDData
{
    static void exit_invalid_arg(const char * arg, const char * msg)
    {
        fprintf(stderr, "Invalid CMD Arg %s - %s\n", arg, msg);
        exit(-1);
    }

    static std::size_t get_size_t(const char * arg, const char * error_msg)
    {
        std::size_t res;
        if(sscanf(arg, "%zd", &res) != 1)
            exit_invalid_arg(arg, error_msg);
        return res;
    }

    static std::size_t get_grid_size(const char ** argv)
    {
        const std::size_t res = 
            get_size_t(argv[1], "Grid Size requires unsigned integer");

        if (res < 3)
            exit_invalid_arg(argv[1], "Grid size has to be larger than 3");

        return res;
    }

    static std::size_t get_overpopulation(const char ** argv)
    {
        return get_size_t(argv[2], "Overpopulation requires unsigned integer");
    }

    static std::size_t get_birth(const char ** argv)
    {
        return get_size_t(argv[3], "Birth requires unsigned integer");
    }

    static std::size_t get_loneliness(const char ** argv)
    {
        return get_size_t(argv[4], "Loneliness requires unsigned integer");
    }

    static std::size_t get_iterations(const char ** argv)
    {
        return get_size_t(argv[5], "Iterations requires unsigned integer");
    }

public:

    const std::size_t grid_size;
    const std::size_t overpopulation;
    const std::size_t birth;
    const std::size_t loneliness;
    const std::size_t iterations;

    CMDData(const int argc, const char ** argv) :
        grid_size{ get_grid_size(argv) },
        overpopulation{ get_overpopulation(argv) },
        birth{ get_birth(argv) },
        loneliness{ get_loneliness(argv) },
        iterations{ get_iterations(argv) }
    {
        std::cout
            << "Conways Game of Life Environment: \n"
            << "\tGrid Size: " << grid_size << "\n"
            << "\tOverpopulation: " << overpopulation << "\n"
            << "\tLoneliness: " << loneliness << "\n"
            << "\tBirth: " << birth << "\n"
            << "\tIterations: " << iterations << "\n\n";
    }
};

struct Grid
{
    using type_t = std::uint8_t;
    using grid_t = std::vector<type_t>;
    grid_t data;
    const std::size_t size;

    Grid(const std::size_t & size) :
        data(size * size),
        size{ size }
    {}

    type_t & at(const std::size_t & row, const std::size_t & col)
    {
        return data[row * size + col];
    }

    const type_t & at(const std::size_t & row, const std::size_t & col) const
    {
        return data[row * size + col];
    }
};

void validate_argc(const int, const char **);
std::size_t get_population(const Grid &, const std::size_t &, const std::size_t &);
void generate(Grid &, const Grid &, const CMDData &);
void save_grid(const Grid &, const std::size_t &);

/*
 * ./game <grid_size> <neighbourhood_type> <overpopulation> <birth> <loneliness> <iterations>
 *
*/
int main(const int argc, const char ** argv)
{
    validate_argc(argc, argv);
    CMDData cmd_data{ argc, argv };
    Grid current{ cmd_data.grid_size };
    Grid next{ cmd_data.grid_size };

    std::fill(current.data.begin(), current.data.end(), 0);
    current.at(1, 3) = current.at(2, 1) = current.at(2, 3) = current.at(3, 2) = current.at(3, 3) = 1;

    for (std::size_t i = 0; i != cmd_data.iterations; ++i)
    {
        generate(next, current, cmd_data);
        save_grid(current, i);
        current.data = next.data;
    }

    return 0;
}


void generate(Grid & next, const Grid & current, const CMDData & cmd_data)
{ 
    for (std::size_t r = 1; r != current.size - 1; ++r)
    {
        for (std::size_t c = 1; c != current.size - 1; ++c)
        {
            const std::size_t population = get_population(current, r, c);

            if (current.at(r, c) != 0 &&
                (population > cmd_data.overpopulation ||
                 population < cmd_data.loneliness))
                next.at(r, c) = 0;
            else if (current.at(r, c) == 0 &&
                     population == cmd_data.birth)
                next.at(r, c) = 1;
            else
                next.at(r, c) = current.at(r, c);
        }
    }
}

std::size_t get_population(const Grid & grid, 
                           const std::size_t & r,
                           const std::size_t & c)
{
    return grid.at(r - 1, c - 1) +
            grid.at(r - 1, c) +
            grid.at(r - 1, c + 1) +
            grid.at(r, c - 1) +
            grid.at(r, c + 1) +
            grid.at(r + 1, c - 1) +
            grid.at(r + 1, c) +
            grid.at(r + 1, c + 1);
}

void save_grid(const Grid & grid, const std::size_t & generation)
{
    std::stringstream ss;
    ss << "files/save_" << generation;
    std::ofstream file{ ss.str(), std::ios::trunc };

    for (std::size_t r = 0; r != grid.size; ++r, file << '\n')
        for (std::size_t c = 0; c != grid.size; ++c)
            file << (grid.at(r, c) == 0 ? '_' : 'X');
}

void validate_argc(const int argc, const char ** argv)
{
    if (argc != 6)
    {
        fprintf(stderr, "Usage: %s  \
<grid_size> <overpopulation> \
<birth> <loneliness> <iterations>\n",
            argv[0]);
        exit(-1);
    }   

}
