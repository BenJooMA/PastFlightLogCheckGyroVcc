#include <iostream>


#include <boost/filesystem.hpp>


#define MAX_NUM_FMT		256
#define MAX_SIZE_FMT	256


struct blob_t
{
	blob_t(unsigned char _id, unsigned char _sz, const std::string& _name)
		:
		id(_id),
		sz(_sz),
		name(_name)
	{
		memset(format, 0, 16);
	}

	unsigned char id;
	unsigned char sz;
	std::string name;
	char format[16];
	std::vector<std::string> columns;
};


const std::map<char, int>					g_DataTypes = { { 'a', 64 },
																{ 'b', 1  },
																{ 'B', 1  },
																{ 'c', 2  },
																{ 'C', 2  },
																{ 'd', 8  },
																{ 'e', 4  },
																{ 'f', 4  },
																{ 'h', 2  },
																{ 'H', 2  },
																{ 'i', 4  },
																{ 'I', 4  },
																{ 'L', 4  },
																{ 'M', 1  },
																{ 'n', 4  },
																{ 'N', 16 },
																{ 'Q', 8  },
																{ 'Z', 64 } };


std::ofstream g_Output;
static int g_Count = 0;


bool CheckFlightLog(boost::filesystem::path path)
{
	std::map<unsigned char, blob_t>	blob_types;

	std::ifstream input(path.string(), std::ifstream::in | std::ifstream::binary);
	if (!input.is_open())
	{
		std::cerr << "Could not read file " << path.filename().c_str() << std::endl;
		return false;
	}

	char header[4];
	memset(header, 0, 4);
	input.read(header, 4);
	if (header[0] != '\x0A3' || header[1] != '\x095' || header[2] != '\x080' || header[3] != '\x080')
	{
		std::cout << path.filename() << " is not a valid DataFlash log file!" << std::endl;
		input.close();
		return false;
	}

	char fmt_tag_size = 0;
	input.read(&fmt_tag_size, 1);
	int bytes_left = static_cast<int>(fmt_tag_size) - 5;
	char rubbish[MAX_SIZE_FMT];
	memset(rubbish, 0, MAX_SIZE_FMT);
	input.read(rubbish, bytes_left);

	int read_index = 0;
	char magic[2];
	int fmt_tags_found = 0;
	while (input.read(magic, 2))
	{
		read_index = 2;

		if (magic[0] != '\xA3' || magic[1] != '\x95')
		{
			break;
		}

		char curr_id = 0;
		input.read(&curr_id, 1);
		read_index++;

		if (curr_id == '\x080')
		{
			char blob_id = 0;
			input.read(&blob_id, 1);

			char blob_sz = 0;
			input.read(&blob_sz, 1);

			char blob_nm[5];
			memset(blob_nm, 0, 5);
			input.read(blob_nm, 4);

			char blob_ft[17];
			memset(blob_ft, 0, 17);
			input.read(blob_ft, 16);

			char blob_dt[65];
			memset(blob_dt, 0, 65);
			input.read(blob_dt, 64);

			blob_t blob(blob_id, blob_sz, std::string(blob_nm));
			memcpy(blob.format, blob_ft, 16);

			std::string name = "";
			for (int i = 0; i < 64; i++)
			{
				if (blob_dt[i] == '\0')
				{
					blob.columns.push_back(name);
					break;
				}
				else if (blob_dt[i] == ',')
				{
					blob.columns.push_back(name);
					name = "";
				}
				else
				{
					name += blob_dt[i];
				}
			}

			blob_types.insert({ blob_id, blob });
			std::cout << ".";
		}

		memset(magic, 0, 2);
	}

	std::cout << "+" << std::endl;
	g_Count++;

	input.close();

	return true;
}


void FindLogInDirectory(boost::filesystem::path path)
{
	try
	{
		if (boost::filesystem::exists(path))
		{
			if (boost::filesystem::is_regular_file(path))
			{
				if (path.filename().extension() == ".bin")
				{
					if (!CheckFlightLog(path))
					{
						std::cerr << "Check failed..." << std::endl;
					}
				}
			}
			else if (boost::filesystem::is_directory(path))
			{
				for (boost::filesystem::directory_entry& p : boost::filesystem::directory_iterator(path))
				{
					FindLogInDirectory(p);
				}
			}
		}
		else
		{
			std::cout << path << " does not exist!" << std::endl;
		}
	}
	catch (const std::exception & e)
	{
		std::cerr << e.what() << std::endl;
	}
}


void FindLog(const std::string& path_str)
{
	boost::filesystem::path file_path(path_str);
	FindLogInDirectory(file_path);
}


int main(int argc, char** argv)
{
	if (argc != 3)
	{
		std::cout << "Usage: " << boost::filesystem::path(argv[0]).filename() << " <path_to_flight_log_folder> <csv_output_path>" << std::endl;
		std::cout << std::endl;
		std::cout << "for example: " << boost::filesystem::path(argv[0]).filename() << " D:\\Work\\FlighLogs\\All D:\\Work\\FlightAnalysis\\log.csv" << std::endl;
		std::cout << std::endl;

		return 1;
	}

	boost::filesystem::path file_path(argv[1]);
	std::string output_path = std::string(argv[2]);

	g_Output = std::ofstream(output_path);
	if (!g_Output.is_open())
	{
		std::cerr << "Could not open " << output_path << " for writing!" << std::endl;
		return 1;
	}

	FindLog(std::string(argv[1]));

	g_Output.close();

	std::cout << std::endl;
	std::cout << "Flight logs scanned: " << g_Count << std::endl;

	return 0;
}