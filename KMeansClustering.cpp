#include <iostream>
#include <vector>
#include <string>
#include <fstream>
#include <filesystem>
#include <random>
#include <algorithm>

std::vector<std::string> parseCSVLine(const std::string& line) {
	std::vector<std::string> entry;
	std::string item;
	bool inQuotes = false;

	for (size_t i = 0; i < line.size(); i++) {
		char c = line[i];

		if (c == ',' && !inQuotes) {
			entry.push_back(item);
			item.clear();
		}
		else if (c == '"') {
			inQuotes = !inQuotes;
		}
		else {
			item += c;
		}

		if (entry.size() > 100) throw std::invalid_argument("Too many fields");
	}

	entry.push_back(item);
	return entry;
}

std::vector<int> cluster(const std::vector<std::vector<int>>& data, int k) {
	size_t entrySize = data[0].size();
	size_t numEntries = data.size();
	std::vector<int> clusters(data.size());

	//bounds for random centroids
	std::vector<int> centroidLowerBounds;
	std::vector<int> centroidUpperBounds;

	for (size_t i = 0; i < entrySize; i++) {
		int lowest = data[0][i];
		int highest = data[0][i];

		for (const std::vector<int>& row : data) {
			int val = row[i];

			if (val < lowest) lowest = val;
			if (val > highest) highest = val;
		}

		centroidLowerBounds.push_back(lowest);
		centroidUpperBounds.push_back(highest);
	}

	//create k random centroids
	std::mt19937 rng(std::random_device{}());
	std::vector<std::vector<int>> centroids(k);

	for (size_t i = 0; i < entrySize; i++) {
		std::uniform_int_distribution<int> dist(centroidLowerBounds[i], centroidUpperBounds[i]);
		
		for (size_t j = 0; j < k; j++) {
			centroids[j].push_back(dist(rng));
		}
	}

	//print centroids created
	for (size_t i = 0; i < entrySize; i++) {
		std::cout << "Column: " << i + 1 << std::endl;
		std::cout << "Largest: " << centroidUpperBounds[i] << std::endl;
		std::cout << "Smallest: " << centroidLowerBounds[i] << std::endl;

		for (size_t j = 0; j < centroids.size(); j++) {
			std::cout << "Centroid " << j + 1 << ": " << centroids[j][i] << std::endl;
		}
		std::cout << std::endl;
	}

	//allocate and update clusters until no points are changed
	bool changed = true;
	while (changed) {
		changed = false;

		//allocate each data point to the nearest centroid
		for (size_t i = 0; i < numEntries; i++) {
			const std::vector<int>& entry = data[i];
			std::vector<float> dist;
			
			for (size_t j = 0; j < k; j++) {
				const std::vector<int>& centroid = centroids[j];

				float currentDist = 0;
				for (size_t l = 0; l < entrySize; l++) {
					float diff = entry[l] - centroid[l];
					currentDist += diff * diff;
				}

				dist.push_back(currentDist);
			}

			int newCluster = std::distance(dist.begin(), std::min_element(dist.begin(), dist.end()));
			if (newCluster != clusters[i]) changed = true;
			clusters[i] = newCluster;
		}

		//update centroids to the mean of all their data points
		for (size_t i = 0; i < k; i++) {
			std::vector<int> positions(entrySize);
			int entryCount = 0;

			for (size_t j = 0; j < numEntries; j++) {
				if (clusters[j] != i) continue;

				entryCount++;
				for (size_t l = 0; l < entrySize; l++) {
					positions[l] += data[j][l];
				}
			}
			if (entryCount == 0) continue;

			for (int& num : positions) {
				num = num / entryCount;
			}

			centroids[i] = positions;
		}
	}

	return clusters;
}

int main(int argc, char* argv[]) {
	//check arguments are correct
	if (argc != 2) {
		std::string name = std::filesystem::path(argv[0]).filename().string();
		std::cerr << "Usage: " << name << " <filename>\n";
		return 1;
	}

	std::string filename = argv[1];
	int nameLen = filename.size();
	if (nameLen <= 4 || filename.substr(nameLen - 4) != ".csv") {
		std::cerr << "Invalid filename (must be .csv)\n";
		return 1;
	}

	std::ifstream csvin(filename);
	if (!csvin.is_open()) {
		std::cerr << "Could not open file: " << filename << "\n";
		return 1;
	}
	
	//read csv
	std::vector<std::vector<int>> entries;
	std::vector<std::string> fieldNames;
	std::string line;

	std::getline(csvin, line);
	if (line.empty()) {
		std::cerr << "Empty csv file: " << filename << "\n";
		return 1;
	}
	fieldNames = parseCSVLine(line);

	std::vector<std::string> parsedLine;
	std::vector<int> entry;
	while (std::getline(csvin, line)) {
		try {
			parsedLine = parseCSVLine(line);
			entry.clear();

			for (std::string& item : parsedLine) {
				//trim whitespace
				size_t start = item.find_first_not_of(" \t\r\n");
				size_t end = item.find_last_not_of(" \t\r\n");

				if (start == std::string::npos)
					throw std::invalid_argument("Empty CSV item");

				item = item.substr(start, end - start + 1);

				//allow only numbers
				for (char c : item) {
					if (!isdigit(c))
						throw std::invalid_argument("Invalid character in input data");
				}

				entry.push_back(std::stoi(item));
			}

			entries.push_back(entry);
		}
		catch (const std::invalid_argument& e) {
			std::cerr << "Error reading CSV: " << e.what() << std::endl;
			return 1;
		}
	}

	//check shape of entries
	if (entries.empty()) {
		std::cerr << "No input data: " << filename << "\n";
		return 1;
	}
	else {
		int entrySize = entries[0].size();

		for (std::vector<int>& entry : entries) {
			if (entry.size() != entrySize) {
				std::cerr << "Inconsistent entry size in input data\n";
				return 1;
			}
		}
	}

	std::vector<int> out = cluster(entries, 2);

	//todo: create new csv containing the input data and the cluster each row belongs to
}