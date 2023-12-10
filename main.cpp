using namespace std;

#include <iostream>
#include <vector>
#include <queue>

#include "imgui.h"
#include "imgui-SFML.h"
#include "SFML/Graphics.hpp"
#include "Economy.h"

class Voxel {
public:
	int x;
	int y;

	Voxel(int x, int y) {
		this->x = x;
		this->y = y;
	}
};

class Blob {
public:
	sf::Color color;
	vector<Voxel> voxels;
	
	Blob(vector<Voxel> voxels, sf::Color color) {
		this->voxels = voxels;
		this->color = color;
	}
};

class Province {

};

class WorldMeshGenerator {
public:
	sf::Image* image = nullptr;
	int width = -1;
	int height = -1;
	vector<Blob> blobs;
	vector<Voxel> current_voxels;
	sf::RenderWindow* window = nullptr;

	WorldMeshGenerator(string file_name, sf::RenderWindow* window) {
		this->window = window;
		createMesh(file_name);
	}

	void createMesh(string file_name) {
		image = new sf::Image;
		image->loadFromFile(file_name);
		width = image->getSize().x;
		height = image->getSize().y;

		vector<Blob> blobs;
		for (int x = 0; x < width; x++) {
			for (int y = 0; y < height; y++) {
				//cout << "Considering: " << x << ", " << y << " ";
				sf::Color color = image->getPixel(x, y);
				if (color == sf::Color(255, 255, 255)) {
					//cout << "white\n";
					continue;
				}
				//cout << "not white, flooding:\n";
				current_voxels.clear();
				flood(x, y, color);
				cout << "flooding complete " << current_voxels.size() << "\n\n";
				Blob new_blob(current_voxels, color);
				blobs.push_back(new_blob);
			}
		}

		cout << "Num Blobs: " << blobs.size() << "\n";

		// loop through the image
		// if a pixel is found that is not white, perform a flood-filling to create a new blob, mark all these pixels as visited (by changing them to white?).
		// do all the complicated stuff to make a mesh for each blob
		// package the mesh data from the blobs into provinces of common color.
	}

	//void flood(int x, int y, sf::Color color) {
	//	//cout << x << ", " << y << "\n";
	//	if (x < 0 || x >= width) { return; }
	//	if (y < 0 || y >= height) { return; }
	//	if (image->getPixel(x, y) != color) { return; }
	//	current_voxels.push_back(Voxel(x, y));
	//	image->setPixel(x, y, sf::Color(255, 255, 255));

	//	if (image->getPixel(x + 1, y) != sf::Color(255, 255, 255))
	//		flood(x + 1, y, color);

	//	if (image->getPixel(x - 1, y) != sf::Color(255, 255, 255))
	//		flood(x - 1, y, color);

	//	if (image->getPixel(x, y + 1) != sf::Color(255, 255, 255))
	//		flood(x, y + 1, color);

	//	if (image->getPixel(x, y - 1) != sf::Color(255, 255, 255))
	//		flood(x, y - 1, color);
	//}

	void flood(int start_x, int start_y, sf::Color color) {
		std::queue<std::pair<int, int>> pixels_to_process;
		pixels_to_process.push(std::make_pair(start_x, start_y));

		while (!pixels_to_process.empty()) {
			int x = pixels_to_process.front().first;
			int y = pixels_to_process.front().second;
			pixels_to_process.pop();

			if (x < 0 || x >= width || y < 0 || y >= height || image->getPixel(x, y) != color) {
				continue;
			}

			current_voxels.push_back(Voxel(x, y));
			image->setPixel(x, y, sf::Color(255, 255, 255));

			// Add neighboring pixels to the queue
			pixels_to_process.push(std::make_pair(x + 1, y));
			pixels_to_process.push(std::make_pair(x - 1, y));
			pixels_to_process.push(std::make_pair(x, y + 1));
			pixels_to_process.push(std::make_pair(x, y - 1));
		}
	}

	void draw() {
		sf::Texture texture;
		texture.loadFromImage(*image);
		sf::Sprite sprite;
		sprite.setTexture(texture);
		window->draw(sprite);
	}
};

int main() {
	sf::RenderWindow window(sf::VideoMode(1455, 850), "Hello SFML", sf::Style::Close);
	sf::Event event;
	ImGui::SFML::Init(window);
	//ImGui::GetIO().IniFilename = NULL; // Resets Window Position

	EconomicSimulation econSim(3, 3);
	econSim.agents[0].prod_functions[0].a *= 2.0f;
	econSim.agents[1].prod_functions[0].a *= 2.0f;
	econSim.agents[2].prod_functions[0].a *= 2.0f;

	WorldMeshGenerator meshGenerator("images/America.png", &window);

	sf::Clock deltaClock;
	while (window.isOpen()) {
		while (window.pollEvent(event)) {
			ImGui::SFML::ProcessEvent(event);
			if (event.type == sf::Event::Closed) {
				window.close();
			}
		}
		ImGui::SFML::Update(window, deltaClock.restart());
		
		econSim.simulationStep();

		for (Agent& agent : econSim.agents) {
			string agent_label = "Agent " + to_string(agent.index);
			ImGui::Begin(agent_label.c_str());
			
			// Stats
			ImGui::Dummy({ 0.0f, 15.0f });
			ImGui::Text("Wage Rate:                         $%.3f", agent.wage_rate);
			ImGui::Text("Income:                            $%.3f", agent.income);
			ImGui::Text("Labour Committment:                %.3f hours", agent.labour_commitment);

			// Supply, Demand, Price
			ImGui::Dummy({ 0.0f, 15.0f });
			for (int good = 0; good < econSim.numGoods; good++) {
				ImGui::Text("Supply: %.3f units     Demand: $%.3f     Price: $%.3f", agent.supplies[good], 69.0f, agent.prices[good]);
			}

			// Industry Reorganisation
			ImGui::Dummy({ 0.0f, 15.0f });
			for (int good = 0; good < econSim.numGoods; good++) {
				ImGui::Text("Marginal Value: $%.3f / unitLabour", agent.dVdL[good]);
			}
			ImGui::Text("Most Labour-Productive Good: %i", agent.most_labour_productive_good);

			// Budget Introspection
			ImGui::Dummy({ 0.0f, 15.0f });
			for (int good = 0; good < econSim.numGoods; good++) {
				ImGui::Text("Consumption: %.3f", agent.consumptions[good]);
			}

			ImGui::End();
		}

		window.clear(sf::Color(0, 0, 0));
		ImGui::SFML::Render(window);
		window.display();
	}

	ImGui::SFML::Shutdown();
	return 0;
}