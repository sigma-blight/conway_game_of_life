#include <string>
#include <vector>
#include <random>
#include <fstream>

#include <mpi.h>
#include <omp.h>

class Data
{
	class Grid : public std::vector<int>
	{
		using vector_t = std::vector<int>;

	public:
		using vector_t::vector;

		int & at(const std::size_t & x, const std::size_t & y) {
			return vector_t::at(y * Data::grid_size() + x);
		}

		const int & at(const std::size_t & x, const std::size_t & y) const {
			return vector_t::at(y * Data::grid_size() + x);
		}
	};

	struct MPI
	{
		const int & size;
		const int & rank;

		MPI(const int & size, const int & rank) :
			size{ size },
			rank{ rank }
		{}

		static const MPI & load_mpi(void)
		{
			static int size = 0;
			static int rank = 0;

			MPI_Comm_size(MPI_COMM_WORLD, &size);
			MPI_Comm_rank(MPI_COMM_WORLD, &rank);

			static MPI mpi{ size, rank };
			return mpi;
		}

		std::size_t part_size(void) const { 
			return Data::grid_size() / size; 
		}
	};

	enum HoodType
	{
		MOORE,
		VON_NEUMANN
	};

public:

	//****************************************************
	//****************************************************
	//					EDIT VALUES HERE
	//****************************************************
	//****************************************************

	static constexpr std::size_t iterations(void) { return 100; }
	static const std::size_t digit_size(void) { return 3; }
	static constexpr std::size_t grid_size(void) { return 50; }
	static constexpr HoodType hood_type(void) { return MOORE; }

	static constexpr std::size_t lonely(void) { return 2; } // less then this gives death
	static constexpr std::size_t overpopulated(void) { return 3; } // more then this gives death
	static constexpr std::size_t birth(void) { return 3; } // exactly this gives life

private:

	Grid				_grid;
	const MPI &			_mpi;

public:

	// c++ for the win
	Data(void) : Data(MPI::load_mpi()) {}
	
	void update(void)
	{
		Grid next(Data::_grid.size());

		std::size_t x, y;
#pragma omp parallel for private(x, y)
		for (x = 1; x < Data::grid_size() - 1; ++x)
			for (y = 1; y < Data::grid_size() - 1; ++y)
			{
				std::size_t hood = Data::hood_count(x, y);

				if (Data::_grid.at(x, y) && 
					(hood < lonely() || hood > overpopulated()))
					next.at(x, y) = false;

				else if (!Data::_grid.at(x, y) && 
					hood == birth())
					next.at(x, y) = true;

				else
					next.at(x, y) = Data::_grid.at(x, y);
			}

		Data::_grid = next;
	}	

	const Grid & grid(void) const { return Data::_grid; }
	const MPI & mpi(void) const { return Data::_mpi; }

private:

	Data(const MPI & mpi) :
		_grid(Data::grid_size() * Data::grid_size()),
		_mpi{ mpi }
	{
		for (int & cell : Data::_grid)
			cell = random();
	}

	static int random(void)
	{
		static std::random_device rd;
		static std::mt19937 gen(rd());
		static std::uniform_int_distribution<> dis(0, 1);
		return dis(gen);
	}

	std::size_t hood_count(const std::size_t & x, const std::size_t & y) const
	{
		const Grid & grid = Data::_grid; // just for ease of naming

		switch (hood_type())
		{
		case MOORE:
			return 
				grid.at(x - 1, y - 1) + grid.at(  x  , y - 1) + grid.at(x + 1, y - 1) +
				grid.at(x - 1,   y  ) + /*   no centre     */ + grid.at(x + 1,   y  ) +
				grid.at(x - 1, y + 1) + grid.at(  x  , y + 1) + grid.at(x + 1, y + 1);

		case VON_NEUMANN:
			return 
										grid.at(  x  , y - 1) + 
				grid.at(x - 1,   y  ) + /*   no centre     */ + grid.at(x + 1,   y  ) +
										grid.at(  x  , y + 1);
		}
		
		return 0; // default
	}
};

void write_to_file(const Data &, const std::size_t &);

int main(int argc, char ** argv)
{
	MPI_Init(&argc, &argv);
	Data data;

	for (std::size_t generation = 0;
		generation != Data::iterations();
		++generation)
	{
		data.update();
		write_to_file(data, generation);
	}

	MPI_Finalize();
}

void write_to_file(const Data & data, const std::size_t & generation)
{
	std::ofstream file{
		std::string{ "files/" }.append(
			std::string(data.digit_size() - 
				std::to_string(generation).size(), '0').append(
					std::to_string(generation).append(
						"data.save"
		))), std::ios::trunc 
	};	

	for (std::size_t y = 0; y < Data::grid_size(); file << "\n", ++y)
		for (std::size_t x = 0; x < Data::grid_size(); ++x) {
			file << data.grid().at(x, y);
		}
}
