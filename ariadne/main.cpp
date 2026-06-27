/*

	ARIADNE
	Web-Based Escape Room Helper


*/

// standard template library
#include <iostream>
#include <vector>
#include <print>

// libraries
#include <raylib.h>
#include <pugixml.hpp>


struct Step {
	std::string name;
	std::string url;
	std::string description;
	std::string username, password;
	
	std::vector<Step> children;
};


Step parseStep(pugi::xml_node step) {

	Step thisStep;

	thisStep.name = step.attribute("name").as_string();
	thisStep.url = step.attribute("url").as_string();
	thisStep.description = step.attribute("description").as_string();
	thisStep.username = step.attribute("username").as_string();
	thisStep.password = step.attribute("password").as_string();

	for (pugi::xml_node s : step.children("step")) {
		thisStep.children.push_back(parseStep(s));
	}
	return thisStep;
}

void loadIntoRoot(Step* root, std::string path) {

	pugi::xml_document doc;
	pugi::xml_parse_result result = doc.load_file(path.c_str());

	if (!result) throw std::out_of_range("file not xml or doesn't exist");
	
	*root = parseStep(doc.child("step"));


}

int main(int argc, char** argv) {
	
	Step root;
	root.name = "Test";
	root.url = "https://example.com";
	root.description = "Test description";
	root.username = "";
	root.password = "";
	loadIntoRoot(&root, "example.xml");

	std::vector<int> objectPath;
	objectPath.push_back(0);
	objectPath.push_back(0);

	SetConfigFlags(FLAG_WINDOW_RESIZABLE);
	InitWindow(1280, 720, "Ariadne");
	SetTargetFPS(60);
	SetExitKey(0);



	while (!WindowShouldClose()) {
		BeginDrawing();
		ClearBackground(BLACK);

		Step* currentStep = &root;

		for (int i = 0; i < objectPath.size(); i++) {
			currentStep = &(currentStep->children.at(objectPath[i]));
		}
		
		int height = 0;
		for (const Step& step : currentStep->children) {
			DrawText(step.name.c_str(), 0, height, 20, WHITE);
			height += 20;
		}


		EndDrawing();
	}

}

// for /subsystem:windows
int WinMain(int argc, char** argv) {
	main(argc, argv);
}