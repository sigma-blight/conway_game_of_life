#include <vector>
#include <string>
#include <fstream>
#include <sstream>
#include <cstdlib>
#include <ctime>
#include <cmath>

typedef unsigned long size_t;
namespace std {
    typedef unsigned long size_t;
}

#include <mpi.h>
#include <omp.h>
MPI_Comm comm = MPI_COMM_WORLD;

struct Grid
{
    static const std::size_t cols = 1000;

    typedef unsigned short type_t;
    typedef std::vector<type_t> grid_t;

    grid_t data;
    const std::size_t rows;

    Grid(const std::size_t & rows) :
        data(rows * cols),
        rows{ rows }
    {}

    type_t & at(const std::size_t & row, const std::size_t & col)
    {
        return data[row * cols + col];
    }

    const type_t & at(const std::size_t & row, const std::size_t & col) const
    {
        return data[row * cols + col];
    }
};

struct Ruleset
{
    static const std::size_t overpopulation = 3;
    static const std::size_t loneliness = 2;
    static const std::size_t birth = 3;
};

std::size_t get_population(const Grid &, const std::size_t &, const std::size_t &);
void init_grid(Grid &);
void generate(Grid &, const Grid &);
void save_grid(const Grid &, const std::size_t &);
void print_grid(std::ofstream &, const Grid &);

int main(int argc, char ** argv)
{
    static const std::size_t iterations = 1000;
    static const int north_tag = 1;
    static const int south_tag = 2;

    int rank, num_ranks;

    MPI::Init(argc, argv);
    MPI_Comm_rank(comm, &rank);
    MPI_Comm_size(comm, &num_ranks);

    size_t rank_grid_size = std::ceil((double)Grid::cols / num_ranks);
    if (rank == num_ranks - 1 &&
        rank_grid_size * num_ranks >= Grid::cols) 
        rank_grid_size -= rank_grid_size * num_ranks - Grid::cols;

//    if (rank > 0 && num_ranks - 1 > rank) rank_grid_size += 2;
//    else ++rank_grid_size;

    Grid current{ rank_grid_size };
    Grid next{ rank_grid_size };
    init_grid(current);
    MPI_Request requests[2];
    MPI_Status status[2];

    std::cout << "Rank " << rank 
            << " \t- Grid Size: " << rank_grid_size << "\n"
            << " \t- World Size: " << num_ranks << "\n";

    for (std::size_t i = 0; i != iterations; ++i)
    {
        if (num_ranks != 1)
        {
            if (rank != 0)        
                MPI_Irecv(&current.data[0], Grid::cols, MPI::BYTE, 
                          rank - 1, north_tag, comm, &requests[0]);        
        
            if (rank != num_ranks - 1)
                MPI_Isend(&current.at(current.rows - 1, 0), Grid::cols, MPI::BYTE, 
                          rank + 1, north_tag, comm, &requests[0]);

            if (rank != num_ranks - 1)
                MPI_Irecv(&current.at(current.rows - 1, 0), Grid::cols, MPI::BYTE,
                          rank + 1, south_tag, comm, &requests[1]);

            if (rank != 0)
                MPI_Isend(&current.data[0], Grid::cols, MPI::BYTE,
                          rank - 1, south_tag, comm, &requests[1]);

            MPI_Wait(&requests[0], &status[0]);
            MPI_Wait(&requests[1], &status[1]);
            MPI_Waitall(2, requests, status);
        }

//        save_grid(current, i);
        generate(next, current);

        current.data = next.data;
    }

    MPI::Finalize();

    return 0;
}


void generate(Grid & next, const Grid & current)
{ 
    for (std::size_t r = 1; r != current.rows - 1; ++r)
        for (std::size_t c = 1; c != current.cols - 1; ++c)
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
    srand(time(NULL));
    for (std::size_t r = 0; r != grid.rows; ++r)
        for (std::size_t c = 0; c != grid.cols; ++c)
            grid.at(r, c) = rand() % 2;
}

void save_grid(const Grid & grid, const std::size_t & generation)
{
    static const std::size_t size_tag = 3;
    static const std::size_t data_tag = 4;

    int rank, num_ranks;
    MPI_Comm_rank(comm, &rank);
    MPI_Comm_size(comm, &num_ranks);

    if (rank == 0)
    {
        std::stringstream ss;
        ss << "files/save_" << generation;
        std::ofstream file{ ss.str(), std::ios::trunc };

        print_grid(file, grid);

        for (int i = 1; i != num_ranks; ++i)
        {
            MPI_Request requests[2];
            MPI_Status status[2];

            size_t rank_rows;
            MPI_Irecv(&rank_rows, 1, MPI::UNSIGNED, i, size_tag, 
                MPI_COMM_WORLD, &requests[0]);

            Grid data{ rank_rows };
            MPI_Irecv(&data.data[0], Grid::cols * rank_rows,
                    MPI::BYTE, i, data_tag, MPI_COMM_WORLD,
                    &requests[1]);

            MPI_Wait(&requests[0], &status[0]);
            MPI_Wait(&requests[1], &status[1]);
            MPI_Waitall(2, requests, status);

            print_grid(file, data);
        }
    }
    else
    {
        MPI_Request requests[2];
        MPI_Status status[2];

        MPI_Isend(&grid.rows, 1, MPI::UNSIGNED, 0, size_tag,
            MPI_COMM_WORLD, &requests[0]);

        MPI_Isend(&grid.data[0], grid.data.size(), MPI::BYTE, 0,
            data_tag, MPI_COMM_WORLD, &requests[1]);

 
        MPI_Wait(&requests[0], &status[0]);
        MPI_Wait(&requests[1], &status[1]);
        MPI_Waitall(2, requests, status);
    }
}

void print_grid(std::ofstream & file, const Grid & grid)
{
    for (std::size_t r = 0; r != grid.rows; ++r, file << '\n')
        for (std::size_t c = 0; c != grid.cols; ++c)
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
