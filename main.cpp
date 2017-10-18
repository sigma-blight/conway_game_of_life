#include <vector>
#include <string>
#include <fstream>
#include <sstream>
#include <random>

#include <mpi.h>
#include <omp.h>

struct Grid
{
    using type_t = std::uint8_t;
    using grid_t = std::vector<type_t>;

    static constexpr std::size_t size = 30;
    grid_t data;

    Grid(void) : data(size * size) {}

    type_t & at(const std::size_t & row, const std::size_t & col)
    {
        return data[row * size + col];
    }

    const type_t & at(const std::size_t & row, const std::size_t & col) const
    {
        return data[row * size + col];
    }
};

struct Ruleset
{
    static constexpr std::size_t overpopulation = 3;
    static constexpr std::size_t loneliness = 2;
    static constexpr std::size_t birth = 3;
};

std::size_t get_population(const Grid &, const std::size_t &, const std::size_t &);
void init_grid(Grid &);
void generate(Grid &, const Grid &);
void save_grid(const Grid &, const std::size_t &);

int main(int argc, char ** argv)
{
    MPI::Init(argc, argv);

    static constexpr std::size_t iterations = 100;
    Grid current;
    Grid next;
    init_grid(current);

    for (std::size_t i = 0; i != iterations; ++i)
    {
        save_grid(current, i);
        generate(next, current);
        current.data = next.data;
    }

    MPI::Finalize();

    return 0;
}


void generate(Grid & next, const Grid & current)
{ 
    for (std::size_t r = 1; r != current.size - 1; ++r)
        for (std::size_t c = 1; c != current.size - 1; ++c)
        {
            const std::size_t population = get_population(current, r, c);

            if (current.at(r, c) != 0 &&
                (population > Ruleset::overpopulation ||
                 population < Ruleset::loneliness))
                next.at(r, c) = 0;
            else if (current.at(r, c) == 0 &&
                     population == Ruleset::birth)
                next.at(r, c) = 1;
            else
                next.at(r, c) = current.at(r, c);
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

void init_grid(Grid & grid)
{
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, 1);
    
    for (std::size_t r = 0; r != grid.size; ++r)
        for (std::size_t c = 0; c != grid.size; ++c)
            grid.at(r, c) = dis(gen); 
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
