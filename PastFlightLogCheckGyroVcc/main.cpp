#include <iostream>


#include <boost/filesystem.hpp>


#include "Utilities.h"


#define MAX_NUM_FMT			256
#define MAX_SIZE_FMT		256

#define VCC_DROP_THRESHOLD	0.025f


using float_container = std::vector<std::pair<uint64_t, float>>;


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


const std::map<char, int>					g_DataTypes		= { { 'a', 64 },
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


bool AnaliseVccData(const float_container& data, std::vector<uint64_t>& problem_areas)
{
	bool result = true;

	std::cout << "Vcc data found: " << data.size() << std::endl;

	if (data.size() < 1)
	{
		return true;
	}

	std::cout << "Start time: " << MillisecondsToTimeStamp(data[0].first, ':', true) << std::endl;

	float vcc_value_acc = 0.0f;
	for (const auto& v : data)
	{
		vcc_value_acc += v.second;
	}
	float average = vcc_value_acc / static_cast<float>(data.size());
	std::cout << "Average Vcc: " << average << "V" << std::endl;

	float vcc_high_value_acc = 0.0f;
	int vcc_high_value_count = 0;
	for (const auto& v : data)
	{
		if (v.second > average)
		{
			vcc_high_value_acc += v.second;
			vcc_high_value_count++;
		}
	}
	float high_average = vcc_high_value_acc / static_cast<float>(vcc_high_value_count);
	std::cout << "High average Vcc: " << high_average << "V" << std::endl;

	float vcc_low_value_acc = 0.0f;
	int vcc_low_value_count = 0;
	for (const auto& v : data)
	{
		if (v.second < average)
		{
			vcc_low_value_acc += v.second;
			vcc_low_value_count++;
		}
	}
	float low_average = vcc_low_value_acc / static_cast<float>(vcc_low_value_count);
	std::cout << "Low average Vcc: " << low_average << "V" << std::endl;

	float tmp_limit = (high_average - low_average) * 0.6f;
	const float limit = (tmp_limit > VCC_DROP_THRESHOLD) ? tmp_limit : VCC_DROP_THRESHOLD;

	std::vector<std::pair<uint64_t, uint64_t>> drops;
	uint64_t temp_start = 0;
	int counter = 0;
	for (const auto& v : data)
	{
		if (v.second < (high_average - limit))
		{
			std::cout << "o";
			counter++;
		}
		else
		{
			std::cout << "_";
			if (counter > 2)
			{
				problem_areas.push_back(temp_start);
				temp_start = 0;
				counter = 0;
				result = false;
			}
		}

		if (counter == 1)
		{
			temp_start = v.first;
		}
	}
	std::cout << std::endl;

	return result;
}


float AnaliseGyroData(const float_container& data_set_0, const float_container& data_set_1)
{
	if (data_set_0.size() != data_set_1.size())
	{
		return false;
	}

	std::size_t size_0 = data_set_0.size();
	float* gyro_data_0 = new float[size_0];

	std::size_t size_1 = data_set_1.size();
	float* gyro_data_1 = new float[size_1];

	std::size_t i = 0ULL;
	for (const auto& a : data_set_0)
	{
		gyro_data_0[i] = a.second;
		i++;
	}

	i = 0ULL;
	for (const auto& a : data_set_1)
	{
		gyro_data_1[i] = a.second;
		i++;
	}

	float similarity = CrossCorrelation(gyro_data_0, gyro_data_1, static_cast<int>(size_1));

	delete[] gyro_data_0;
	delete[] gyro_data_1;

	return similarity;
}


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

	float_container vcc_data;
	float_container gyro_data_x_0;
	float_container gyro_data_y_0;
	float_container gyro_data_z_0;
	float_container gyro_data_x_1;
	float_container gyro_data_y_1;
	float_container gyro_data_z_1;

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
		}
		else
		{
			auto blob_type = blob_types.find(curr_id);
			if (blob_type == blob_types.end())
			{
				break;
			}
			else
			{
				int bytes_to_read = blob_type->second.sz - read_index;

				int size_from_format = 0;
				for (int i = 0; i < 16; i++)
				{
					char symbol = blob_type->second.format[i];
					if (symbol == 0)
					{
						continue;
					}

					auto type = g_DataTypes.find(symbol);
					if (type == g_DataTypes.end())
					{
						break;
					}

					size_from_format += type->second;
				}

				if (size_from_format != bytes_to_read)
				{
					break;
				}

				char* blob_data = new char[bytes_to_read];

				input.read(blob_data, bytes_to_read);

				auto param_name = blob_types.at(curr_id).name;

				uint64_t timestamp = 0;

				if (param_name == "GPS")
				{

				}

				if (param_name == "POWR")
				{
					auto num_columns = blob_types.at(curr_id).columns.size();
					int offset = 0;

					float vcc = 0.0f;
					
					for (auto i = 0U; i < num_columns; i++)
					{
						const std::string curr_column = blob_types.at(curr_id).columns[i];
						const char curr_type = blob_types.at(curr_id).format[i];
						const int curr_size = g_DataTypes.at(curr_type);

						if ((curr_column == "TimeUS") && (curr_type == 'Q'))
						{
							memcpy((char*)&timestamp, &blob_data[offset], curr_size);
						}
						else if ((curr_column == "Vcc") && (curr_type == 'f'))
						{
							memcpy((char*)&vcc, &blob_data[offset], curr_size);
						}

						offset += curr_size;
					}

					vcc_data.push_back({ timestamp, vcc });
				}
				
				if (param_name == "IMU")
				{
					auto num_columns = blob_types.at(curr_id).columns.size();
					int offset = 0;

					int8_t id = -1;
					float gyro_x = 0.0f;
					float gyro_y = 0.0f;
					float gyro_z = 0.0f;

					for (auto i = 0U; i < num_columns; i++)
					{
						const std::string curr_column = blob_types.at(curr_id).columns[i];
						const char curr_type = blob_types.at(curr_id).format[i];
						const int curr_size = g_DataTypes.at(curr_type);

						if ((curr_column == "TimeUS") && (curr_type == 'Q'))
						{
							memcpy((char*)&timestamp, &blob_data[offset], curr_size);
						}
						else if ((curr_column == "I") && (curr_type == 'B'))
						{
							memcpy((char*)&id, &blob_data[offset], curr_size);
						}
						else if ((curr_column == "GyrX") && (curr_type == 'f'))
						{
							memcpy((char*)&gyro_x, &blob_data[offset], curr_size);
						}
						else if ((curr_column == "GyrY") && (curr_type == 'f'))
						{
							memcpy((char*)&gyro_y, &blob_data[offset], curr_size);
						}
						else if ((curr_column == "GyrZ") && (curr_type == 'f'))
						{
							memcpy((char*)&gyro_z, &blob_data[offset], curr_size);
						}

						offset += curr_size;
					}

					if (id == 0)
					{
						gyro_data_x_0.push_back({ timestamp, gyro_x });
						gyro_data_y_0.push_back({ timestamp, gyro_y });
						gyro_data_z_0.push_back({ timestamp, gyro_z });
					}
					else if (id == 1)
					{
						gyro_data_x_1.push_back({ timestamp, gyro_x });
						gyro_data_y_1.push_back({ timestamp, gyro_y });
						gyro_data_z_1.push_back({ timestamp, gyro_z });
					}
				}

				delete[] blob_data;
			}
		}

		memset(magic, 0, 2);
	}


	float gyro_x_sim = AnaliseGyroData(gyro_data_x_0, gyro_data_x_1) * 100.0f;
	float gyro_y_sim = AnaliseGyroData(gyro_data_y_0, gyro_data_y_1) * 100.0f;
	float gyro_z_sim = AnaliseGyroData(gyro_data_z_0, gyro_data_z_1) * 100.0f;


	std::vector<uint64_t> drop_times;
	if (!AnaliseVccData(vcc_data, drop_times))
	{
		g_Output << path.string() << std::endl;
		for (const auto& t : drop_times)
		{
			g_Output << ", @" << std::to_string(t) << std::endl;
		}
		g_Output << std::endl;
	}

	std::cout << "Gyro_X similariy: " << std::to_string(gyro_x_sim) << std::endl;
	std::cout << "Gyro_Y similariy: " << std::to_string(gyro_y_sim) << std::endl;
	std::cout << "Gyro_Z similariy: " << std::to_string(gyro_z_sim) << std::endl;

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